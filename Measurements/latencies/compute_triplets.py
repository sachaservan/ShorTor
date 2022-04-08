import argparse
import datetime
import logging
import os
import sys

from typing import List, Any

import sqlalchemy as sa
from sqlalchemy.orm.session import Session
from ting.utils import TingLeg


from latencies.db import make_session_from_url
from latencies.models import (
    IncrementalHighWaterMark,
    PairwiseLatency,
    TripletLatency,
    Circuit,
    Measurement,
    Batch,
)

log = logging.getLogger(__name__)
TRACE = logging.DEBUG - 5
logging.addLevelName(TRACE, "TRACE")

BATCH_SIZE = 10


def _parse_args(argv: List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")

    return parser.parse_args(argv[1:])


def _configure_logging():
    logging.basicConfig(
        level=getattr(logging, os.getenv("SHORTOR_LOG_LEVEL", "DEBUG").upper())
    )


def _connect_to_db(args) -> Session:
    db_url = args.db_url or os.getenv("SHORTOR_DB_URL")
    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")

    log.info(f"Connecting to database: [{db_url}]")
    return make_session_from_url(db_url)


def _latency_for_batch(db: Session, batch: Batch):
    def _min_latency_query(where, leg):
        return (
            sa.select([sa.func.min(Measurement.latency_back + Measurement.latency_out)])
            .select_from(Batch)
            .where(Batch.end_time <= batch.end_time)
            .where(Batch.spot_check == False)
            .where(Batch.status == Batch.Status.SUCCEEDED)
            .where(where)
            .join(Circuit, Circuit.batch_id == Batch.id)
            .where(Circuit.leg == leg)
            .join(Measurement, Measurement.circuit_id == Circuit.id)
        )

    def _foo(relay):
        q1 = _min_latency_query(Batch.relay1 == relay, TingLeg.X)
        q2 = _min_latency_query(Batch.relay2 == relay, TingLeg.Y)
        u = sa.union(q1, q2)
        return db.execute(sa.select(sa.func.min(u.columns[0])).select_from(u)).first()[
            0
        ]

    x_min = _foo(batch.relay1)
    y_min = _foo(batch.relay2)
    xy_min = db.execute(
        _min_latency_query(
            sa.or_(
                sa.and_(Batch.relay1 == batch.relay1, Batch.relay2 == batch.relay2),
                sa.and_(Batch.relay2 == batch.relay1, Batch.relay1 == batch.relay2),
            ),
            TingLeg.XY,
        )
    ).first()[0]

    rtt = int(xy_min - x_min / 2 - y_min / 2)
    return rtt


def _pairs_involving(relay_id: int):
    relay1_matches = (
        sa.select(PairwiseLatency.relay2_id.label("relay"), PairwiseLatency.latency_rtt)
        .select_from(PairwiseLatency)
        .where(PairwiseLatency.relay1_id == relay_id)
    )
    relay2_matches = (
        sa.select(PairwiseLatency.relay1_id.label("relay"), PairwiseLatency.latency_rtt)
        .select_from(PairwiseLatency)
        .where(PairwiseLatency.relay2_id == relay_id)
    )
    return sa.union(relay1_matches, relay2_matches)


def _update_pair(db, relay1_id: int, relay2_id: int, rtt: int) -> bool:
    log.debug(f"Computing pairwise latency for {relay1_id}<->{relay2_id}")
    pairwise_latency = (
        db.query(PairwiseLatency)
        .where(PairwiseLatency.relay1_id == relay1_id)
        .where(PairwiseLatency.relay2_id == relay2_id)
        .first()
    )
    if pairwise_latency:
        if rtt >= pairwise_latency.latency_rtt:
            log.debug(f"No update to pairwise latency for {relay1_id}<->{relay2_id}.")
            return False
        log.info(f"Updating pairwise latency for {relay1_id}<->{relay2_id}")
        pairwise_latency.latency_rtt = rtt
        db.add(pairwise_latency)
    else:
        pair = PairwiseLatency(
            relay1_id=relay1_id, relay2_id=relay2_id, latency_rtt=rtt
        )
        db.add(pair)
    return True


def _update_triplets(db: Session, relay1_id: int, relay2_id: int, rtt_1_2: int):
    log.debug(f"Computing relevant triplets for {relay1_id}<->{relay2_id}")
    assert relay1_id < relay2_id
    # We have r1<->r2. If we also have r1<->r' and r2<->r', we're interested in the following triplets:
    # - r1<->r'<->r2
    # - r1<->r2<->r'
    # - r2<->r1<->r'
    pairs_involving1 = _pairs_involving(relay1_id)
    pairs_involving2 = _pairs_involving(relay2_id)
    relevant_relays_query = sa.select(
        pairs_involving1.columns[0].label("relay_id"),
        pairs_involving1.columns[1].label("relay1_rtt"),
        pairs_involving2.columns[1].label("relay2_rtt"),
    ).select_from(
        sa.join(
            pairs_involving1,
            pairs_involving2,
            pairs_involving1.columns[0] == pairs_involving2.columns[0],
        )
    )

    triplets_with_speedup = []
    for (other_id, rtt_1_other, rtt_2_other) in db.execute(relevant_relays_query):
        # r1<->r'<->r2: r' as via
        triplet_rtt = rtt_1_other + rtt_2_other
        if triplet_rtt < rtt_1_2:
            triplet = TripletLatency(
                relay_from_id=relay1_id,
                relay_to_id=relay2_id,
                relay_via_id=other_id,
                orig_latency_rtt=rtt_1_2,
                via_latency_rtt=triplet_rtt,
            )
            triplets_with_speedup.append(triplet)
        # r1<->r2<->r'
        triplet_rtt = rtt_1_2 + rtt_2_other
        if triplet_rtt < rtt_1_other:
            triplet = TripletLatency(
                relay_from_id=min(relay1_id, other_id),
                relay_to_id=max(relay1_id, other_id),
                relay_via_id=relay2_id,
                orig_latency_rtt=rtt_1_other,
                via_latency_rtt=triplet_rtt,
            )
            triplets_with_speedup.append(triplet)
        # r2<->r1<->r'
        triplet_rtt = rtt_1_2 + rtt_1_other
        if triplet_rtt < rtt_2_other:
            triplet = TripletLatency(
                relay_from_id=min(relay2_id, other_id),
                relay_to_id=max(relay2_id, other_id),
                relay_via_id=relay1_id,
                orig_latency_rtt=rtt_2_other,
                via_latency_rtt=triplet_rtt,
            )
            triplets_with_speedup.append(triplet)

    if triplets_with_speedup:
        # Delete existing r1<->r2 triplets; we might have invalidated them
        # if rtt_1_2 is lower.
        query = db.query(TripletLatency).where(
            sa.and_(
                TripletLatency.relay_from_id == relay1_id,
                TripletLatency.relay_to_id == relay2_id,
            )
        )
        count = query.count()
        if count:
            log.debug(f"Found {count} existing speedups to replace.")
            query.delete()
        db.add_all(triplets_with_speedup)


def _bump_high_water_mark(db: Session, end_time: datetime.datetime):
    high_water_mark = db.query(IncrementalHighWaterMark).first()
    if high_water_mark is None:
        high_water_mark = IncrementalHighWaterMark()
    high_water_mark.last_batch_end_time = end_time
    db.add(high_water_mark)


def _get_batches(db: Session):
    high_water_mark = db.query(IncrementalHighWaterMark).first()
    if high_water_mark:
        ended_since_last = Batch.end_time > high_water_mark.last_batch_end_time
    else:
        ended_since_last = True
    return list(
        db.query(Batch)
        .where(ended_since_last)
        .where(Batch.status == Batch.Status.SUCCEEDED)
        .where(Batch.spot_check == False)
        .order_by(Batch.end_time.asc())
        .limit(BATCH_SIZE)
    )


def _update_batch(db: Session, batch: Batch):
    if batch.relay1_id >= batch.relay2_id:
        log.warn(
            f"That's weird, {batch.id} had relay1={batch.relay1_id} and relay2={batch.relay2_id}."
        )
        relay1_id = batch.relay2_id
        relay2_id = batch.relay1_id
    else:
        relay1_id = batch.relay1_id
        relay2_id = batch.relay2_id
    rtt = _latency_for_batch(db, batch)  # TODO: make parallel
    if rtt <= 0:
        log.debug(f"That's weird, rtt was <= 0 for batch {batch.id}.")
        return
    is_updated = _update_pair(db, relay1_id, relay2_id, rtt)
    if is_updated:
        _update_triplets(db, relay1_id, relay2_id, rtt)


def update_view(db: Any):
    with db.begin():
        batches = _get_batches(db)
    for idx, batch in enumerate(batches):
        with db.begin():
            log.debug(
                f"Batch {batch.id} ({idx+1} / {len(batches)}): {batch.relay1_id}<->{batch.relay2_id}"
            )
            _update_batch(db, batch)
            _bump_high_water_mark(db, batch.end_time)


def main(argv: List[str]):
    args = _parse_args(argv)
    _configure_logging()
    db = _connect_to_db(args)

    breakpoint()
    while True:
        start = datetime.datetime.now()
        update_view(db)
        end = datetime.datetime.now()
        log.info(f"Processed {BATCH_SIZE} batches in time {end - start}")


if __name__ == "__main__":
    main(sys.argv)
