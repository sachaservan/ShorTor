import sys
import datetime
import logging
import os
import argparse

from sqlalchemy.orm.session import Session

from latencies import tasks
from latencies.db import make_session_from_url
from latencies.models import Relay, Batch

log = logging.getLogger(__name__)
TRACE = logging.DEBUG - 5
logging.addLevelName(TRACE, "TRACE")


def _resolve_fingerprint(db: Session, fingerprint: str) -> Relay:
    try:
        id_ = int(fingerprint)
    except ValueError:
        pass
    else:
        return db.query(Relay).get(id_)
    relays = (
        db.query(Relay).filter(Relay.fingerprint.like(fingerprint.upper() + "%")).all()
    )
    if len(relays) != 1:
        raise RuntimeError(
            f"Fingerprint {fingerprint} matched {len(relays)} !=1 relays: {relays}"
        )
    return relays[0]


def _handle_message(body):
    try:
        print(body["result"]["message"])
    except TypeError:
        print(body)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")
    parser.add_argument(
        "--measurements", type=int, help="Number measurements to take", default=5
    )
    parser.add_argument("fingerprint", nargs=2)

    args = parser.parse_args(argv[1:])

    db_url = args.db_url or os.getenv("SHORTOR_DB_URL")

    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")
    log.info(f"Connecting to database: [{db_url}]")
    logging.basicConfig(
        level=getattr(logging, os.getenv("SHORTOR_LOG_LEVEL", "DEBUG").upper())
    )
    db = make_session_from_url(db_url)

    relay1 = _resolve_fingerprint(db, args.fingerprint[0])
    relay2 = _resolve_fingerprint(db, args.fingerprint[1])
    print(f"Resolved relays: {relay1.fingerprint} {relay2.fingerprint}")
    batch = Batch(
        relay1_id=relay1.id,
        relay2_id=relay2.id,
        status=Batch.Status.PENDING,
        created_time=datetime.datetime.utcnow(),
        spot_check=True,
        num_measurements=args.measurements,
    )
    db.add(batch)
    db.commit()
    print(f"Starting batch {batch.id}...")
    tasks.measure.apply_async(args=(batch.id,), priority=9).get(
        on_message=_handle_message
    )
    print("Done.")
    db.refresh(batch)
    print(f"Results {batch}")
    for circuit in batch.circuits:
        print(f"- {circuit.leg}")
        for measurement in circuit.measurements:
            print(f"  - {measurement}")


if __name__ == "__main__":
    main(sys.argv)
