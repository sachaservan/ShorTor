"""Consensus taker.

This module (runnable) is responsible for:

1. downloading (and parsing) the Tor consensus
2. enqueueing pairs to measure

(2) hides a lot of thorny details, which we will explain when we figure them
out.
"""
import argparse
import datetime as dt
import functools
import os
import json
import sys
import pickle
import logging
import ipaddress
import socket

from typing import List, Optional, cast, Set, Tuple, Generator
from importlib.resources import open_text as open_text_resource
from operator import attrgetter
from pathlib import Path

import geoip2.database
import stem
import stem.descriptor
import stem.descriptor.remote
import stem.descriptor.collector
import sqlalchemy as sa

from stem.descriptor.router_status_entry import RouterStatusEntryV3
from stem.descriptor.server_descriptor import RelayDescriptor
from sqlalchemy.orm.session import Session

import latencies

from latencies import tasks
from latencies.db import make_session_from_url
from latencies.consensus import generate_pairs, DescriptorConverter
from latencies.models import Relay, Batch

log = logging.getLogger(__name__)
TRACE = logging.DEBUG - 5
logging.addLevelName(TRACE, "TRACE")


NUM_MEASUREMENTS = int(os.getenv("SHORTOR_NUM_MEASUREMENTS") or 5)
MAX_ATTEMPTS = int(os.getenv("SHORTOR_MAX_ATTEMPTS", 5))
OLD_BATCH_FAILURE_TIME = dt.timedelta(hours=2)

_CSAIL_RELAY_FINGERPRINTS = {
    "A53C46F5B157DD83366D45A8E99A244934A14C46",
    "9715C81BA8C5B0C698882035F75C67D6D643DBE3",
}


def _upsert_relays(relays: List[Relay], db: Session):
    """Process relays from Tor consensus."""
    log.debug("Adding relays that don't already exist to database.")
    relays_by_fingerprint = {r.fingerprint: r for r in relays}
    fingerprints = list(relays_by_fingerprint)

    # Upsert
    for existing in db.query(Relay).filter(Relay.fingerprint.in_(fingerprints)):
        relay = relays_by_fingerprint.pop(existing.fingerprint)
        relay.id = existing.id
        db.merge(relay)
    log.info(f"Found {len(relays_by_fingerprint)} /new/ relays to add to database.")
    for relay in relays_by_fingerprint:
        log.debug(f"New relay: {relay}")
    db.add_all(relays_by_fingerprint.values())
    db.commit()
    return db.query(Relay).filter(Relay.fingerprint.in_(fingerprints))


def _mark_old_batches_failed(db: Session):
    num_updated = (
        db.query(Batch)
        .where(
            sa.or_(
                Batch.status == Batch.Status.PENDING,
                Batch.status == Batch.Status.INPROGRESS,
            )
        )
        .where(Batch.created_time < dt.datetime.utcnow() - OLD_BATCH_FAILURE_TIME)
        .update({Batch.status: Batch.Status.FAILED, Batch.error: "In queue too long"})
    )
    log.info(
        f"Marked {num_updated} old batches as failed (they were languishing in the queue)."
    )
    db.commit()


def _delete_blacklisted_relays(db: Session, relays: List[Relay], blacklist: List[str]):
    log.info("Deleting (possibly) blacklisted relays.")
    for fp in blacklist:
        matching = [r for r in relays if r.fingerprint == fp]
        if matching:
            relays.remove(matching[0])
        else:
            log.debug("Blacklisted relay not in consensus")
        relay = db.query(Relay).filter(Relay.fingerprint == fp).first()
        if relay:
            log.info(f"Deleting relay {fp} from DB")
            db.delete(relay)
        else:
            log.debug("Blacklisted relay not in database")
    db.commit()


def _current_queue_length(db: Session) -> int:
    """Returns the length of the current Celery job queue."""
    # We use the DB rather than Celery, as Celery introspection is a little
    # tough without a long-lived monitoring service.
    length = db.query(Batch).filter(Batch.status == Batch.Status.PENDING).count()
    log.info(f"Getting the number of pending measurements: {length}.")
    return length


def _already_measured_pairs(db: Session) -> Set[Tuple[id, id]]:
    log.info("Getting the set of pending/completed measurements.")
    completed = (
        db.query(Batch.relay1_id, Batch.relay2_id)
        .filter(
            (Batch.status == Batch.Status.PENDING)
            | (Batch.status == Batch.Status.SUCCEEDED)
        )
        .all()
    )
    log.debug(f"Found {len(completed)} pending/completed measurements.")
    failed = (
        db.query(Batch.relay1_id, Batch.relay2_id)
        .filter(Batch.status == Batch.Status.FAILED)
        .group_by(Batch.relay1_id, Batch.relay2_id)
        .having(sa.func.count(Batch.id) >= MAX_ATTEMPTS)
        .all()
    )
    log.debug(
        f"Found {len(failed)} measurements that have failed the maximum times ({MAX_ATTEMPTS})."
    )
    return set(completed + failed)


def _ting_data_subset(relays: Set[Relay]) -> Set[Relay]:
    log.debug("Loading Ting data")
    with open_text_resource(latencies, "ting-relays.json") as f:
        ting_relays = json.load(f)
    log.debug(f"Found {len(ting_relays)} Ting relays.")
    tingerprints = cast(Set[str], {r["fingerprint"] for r in ting_relays})
    subset = {r for r in relays if r.fingerprint in tingerprints}
    log.debug(f"Current relays intersection with Ting data (count): {len(subset)}")
    return subset


def _num_pairs(relays: Set[Relay]) -> int:
    return len(relays) * (len(relays) - 1) // 2


def _subsets(relays: Set[Relay], batch_size: int) -> Generator[Set[Relay], None, None]:
    """Yields successive sets of relays:

    1. Ting relays
    2. Add (in fingerprint-sorted order) enough relays so that we have
       $SHORTOR_BATCH_SIZE additional pairs (and repeat)
    3. Eventually this gives all pairs
    """
    log.info("Computing successive relay subsets to run")

    # Start with CSAIL relays
    csail_relays = {r for r in relays if r.fingerprint in _CSAIL_RELAY_FINGERPRINTS}
    log.debug(
        f"Current relays intersection with CSAIL relays (count): {len(csail_relays)}"
    )
    yield csail_relays

    # Then Ting relays
    ting_relays = _ting_data_subset(relays)
    yield ting_relays

    # then increment based on incremental total pair count
    relays_sorted = list(
        sorted(relays, key=attrgetter("consensus_weight"), reverse=True)
    )
    last_num_pairs = _num_pairs(ting_relays)

    for idx in range(len(relays_sorted) - 1):
        subset = csail_relays | ting_relays | set(relays_sorted[:idx])
        if subset == relays:
            # we always yield the complete set, so no need to yield it here
            break
        if _num_pairs(subset) >= last_num_pairs + batch_size:
            last_num_pairs = _num_pairs(subset)
            yield subset
    yield relays


def _enqueue_relays(
    db: Session,
    relays: Set[Relay],
    max_queue_length: int,
    batch_size: int,
    number_batches: int,
):
    # Don't enqueue pairs involving us!
    relays = {r for r in relays if "ShorTor" not in r.nickname}
    relays_to_enqueue: int = max(max_queue_length - _current_queue_length(db), 0)
    log.info(
        f"Enqueueing up to {relays_to_enqueue} relay pairs of {len(relays)} relays"
    )
    if relays_to_enqueue == 0:
        log.warn("Queue is full; returning early.")
        return
    measured_pairs: Set[Tuple[id, id]] = _already_measured_pairs(db)
    for idx, subset in enumerate(_subsets(relays, batch_size)):
        if number_batches is not None and idx >= number_batches:
            log.info(
                f"Hard stop at {number_batches} batches (not running this subset)."
            )
            return
        log.info(f"Subset {idx} (size {len(subset)})")
        pairs = generate_pairs(subset, measured_pairs, relays_to_enqueue)
        if pairs:
            batches = []
            log.debug(f"Got {len(pairs)} pairs to enqueue.")
            for pair in pairs:
                log.log(TRACE, f"Enqueueing pair: [{pair}]")
                batch = Batch(
                    relay1=pair[0],
                    relay2=pair[1],
                    status=Batch.Status.PENDING,
                    num_measurements=NUM_MEASUREMENTS,
                    created_time=dt.datetime.utcnow(),
                )
                batches.append(batch)
                db.add(batch)
            db.commit()
            log.debug(f"Batches in DB; now adding them to job queue.")
            for batch in batches:
                tasks.measure.delay(batch.id)
            log.debug(f"Added to job queue.")
            break
        log.info(f"Subset {idx} had no pairs to enqueue; trying next subset.")
    else:
        log.info("No subsets had pairs to enqueue; looks like we're done!")


def get_descriptors(
    relay: Optional[str],
) -> List[Tuple[RouterStatusEntryV3, RelayDescriptor]]:
    """Get relay descriptors from the Tor consensus using the given method.

    Uses the first argument that is truthy.
    """
    if relay:
        log.info(f"Using explicit relay for consensus: [{relay}]")
        host, _, port = relay.partition(":")
        log.debug(f"Parsed as host=[{host}], port=[{port}]")
        try:
            _ = ipaddress.ip_address(host)  # just to validate, don't need result
        except ValueError:
            ip = socket.gethostbyname(host)
            log.debug(f"Resolved [{host}] to [{ip}].")
        else:
            ip = host
        endpoints = [stem.DirPort(ip, port)]
    else:
        log.info("Using default Tor consensus.")
        endpoints = None
    consensus = map(
        functools.partial(cast, RouterStatusEntryV3),
        stem.descriptor.remote.get_consensus(endpoints=endpoints),
    )
    descriptors = map(
        functools.partial(cast, RelayDescriptor),
        stem.descriptor.remote.get_server_descriptors(endpoints=endpoints),
    )
    descriptors_by_fp = {r.fingerprint: r for r in descriptors}
    return [
        (c, descriptors_by_fp[c.fingerprint])
        for c in consensus
        if c.fingerprint in descriptors_by_fp
    ]


def _top_of_hour(datetime: dt.datetime) -> dt.datetime:
    return datetime - dt.timedelta(
        minutes=datetime.minute,
        seconds=datetime.second,
        microseconds=datetime.microsecond,
    )


def get_consensus_or_die(
    converter: DescriptorConverter,
    relay: Optional[str],
    cache: Path,
) -> List[Relay]:

    # Time of the current consensus (we use hour granularity)
    now = dt.datetime.utcnow()
    consensus_time = _top_of_hour(now)
    # New Tor consensus published at 5 past the hour; give a little wiggle room in case clocks are off.
    if now.minute < 5 + 1:
        consensus_time -= dt.timedelta(hours=1)
    log.debug(f"Want consensus for current hour: {consensus_time}")

    # check cache
    last_consensus_link = cache / "last"
    try:
        # 'last' is a symlink to the last consensus
        last_consensus = last_consensus_link.resolve(strict=True)
    except FileNotFoundError:
        log.debug("Empty or missing cache.")
    else:
        if dt.datetime.fromisoformat(last_consensus.name) == consensus_time:
            log.info(f"Consensus cache hit @ {last_consensus}")
            with open(last_consensus, "rb") as f:
                descriptors = pickle.load(f)
            return list(map(converter.descriptor_to_relay, descriptors))
        else:
            log.debug("Consensus cache miss.")

    try:
        descriptors = list(get_descriptors(relay))
    except ValueError:
        log.error("Must provide a consensus source.")
        sys.exit(1)
    log.info(f"Found descriptors for {len(descriptors)} relays.")
    if not descriptors:
        log.error("Fetching Tor consensus failed.")
        sys.exit(1)

    # Cache descriptors
    cache_file = cache / consensus_time.isoformat(timespec="hours")
    with open(cache_file, "wb") as f:
        pickle.dump(descriptors, f)
    last_consensus_link.unlink(missing_ok=True)
    last_consensus_link.symlink_to(cache_file)

    return list(map(converter.descriptor_to_relay, descriptors))


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")
    consensus_group = parser.add_mutually_exclusive_group()
    consensus_group.add_argument("--consensus-file", help="Use local consensus file.")
    consensus_group.add_argument(
        "--consensus-relay",
        type=str,
        help="Use the consensus from this relay (as 'host:DirPort').",
    )
    consensus_group.add_argument(
        "--consensus-tor",
        type=bool,
        help="Use the default Tor consensus (will hit Tor over the network).",
    )

    args = parser.parse_args(argv[1:])

    logging.basicConfig(
        level=getattr(logging, os.getenv("SHORTOR_LOG_LEVEL", "DEBUG").upper())
    )

    db_url = args.db_url or os.getenv("SHORTOR_DB_URL")
    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")
    max_queue_length = int(os.getenv("SHORTOR_MAX_QUEUE_LENGTH") or "5")
    batch_size = int(os.getenv("SHORTOR_BATCH_SIZE") or "10")
    consensus_relay = args.consensus_relay or os.getenv("SHORTOR_CONSENSUS_RELAY")
    consensus_tor = bool(args.consensus_tor or os.getenv("SHORTOR_CONSENSUS_TOR"))
    if not (consensus_relay or consensus_tor):
        print(
            "At least one of SHORTOR_CONSENSUS_RELAY, "
            "SHORTOR_CONSENSUS_TOR must be set."
        )
    consensus_cache = Path(os.getenv("SHORTOR_CONSENSUS_CACHE_DIR"))
    geolite_city = Path(os.getenv("SHORTOR_GEOIP_CITY"))
    geolite_asn = Path(os.getenv("SHORTOR_GEOIP_ASN"))
    relay_blacklist = os.getenv("SHORTOR_RELAY_BLACKLIST")
    try:
        number_batches = int(os.getenv("SHORTOR_NUMBER_BATCHES"))
    except (ValueError, TypeError):
        number_batches = None

    log.info(f"Connecting to database: [{db_url}]")
    db = make_session_from_url(db_url)

    with geoip2.database.Reader(geolite_city) as city_geoip, geoip2.database.Reader(
        geolite_asn,
    ) as asn_geoip:
        converter = DescriptorConverter(city_geoip=city_geoip, asn_geoip=asn_geoip)
        relays = get_consensus_or_die(
            converter,
            consensus_relay,
            consensus_cache,
        )

    if relay_blacklist:
        _delete_blacklisted_relays(db, relays, relay_blacklist.split(" "))
    relays = _upsert_relays(relays, db)
    _mark_old_batches_failed(db)
    _enqueue_relays(db, set(relays), max_queue_length, batch_size, number_batches)


if __name__ == "__main__":
    main(sys.argv)
