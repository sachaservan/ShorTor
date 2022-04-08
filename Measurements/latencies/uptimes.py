from pathlib import Path
import os
import sys
import argparse
import pickle
from sqlalchemy.orm.session import Session
from typing import Dict, Union, List, cast, Tuple
from stem.descriptor.router_status_entry import RouterStatusEntry
from latencies.db import make_session_from_url
from latencies.models import Relay
import stem.descriptor
from stem.descriptor.server_descriptor import RelayDescriptor
from stem.descriptor.router_status_entry import RouterStatusEntry
import datetime as dt
import functools
from operator import attrgetter


@functools.lru_cache()
def _read_all_descriptors(archive_path: Path) -> Dict[str, RelayDescriptor]:
    relays = {}
    for subdir in archive_path.iterdir():
        for subsubdir in subdir.iterdir():
            for descriptor_file in subsubdir.iterdir():
                if descriptor_file.stat().st_size == 0:
                    continue
                with open(descriptor_file, "rb") as f:
                    relay = cast(RelayDescriptor, next(stem.descriptor.parse_file(f)))
                    old_relay = relays.get(relay.fingerprint)
                    if old_relay:
                        relays[relay.fingerprint] = min(
                            (relay, old_relay), key=attrgetter("published")
                        )
                    else:
                        relays[relay.fingerprint] = relay
    return relays


def _read_relay_info(
    time: dt.datetime,
) -> List[Tuple[RouterStatusEntry, RelayDescriptor]]:
    cache_path = Path("/data/collector")

    consensus_path = (
        cache_path
        / time.strftime("consensuses-%Y-%m")
        / time.strftime("%d")
        / time.strftime("%Y-%m-%d-%H-%M-%S-consensus")
    )
    with open(consensus_path, "rb") as f:
        consensus = stem.descriptor.parse_file(
            f, descriptor_type="network-status-consensus-3 1.0"
        )

    descriptor_path = cache_path / time.strftime("server-descriptors-%Y-%m")
    descriptors = _read_all_descriptors(descriptor_path)

    return [
        (c, descriptors[c.fingerprint])
        for c in consensus
        if c.fingerprint in descriptors
    ]


def _get() -> Dict[str, Union[str, bool, int]]:
    consensus_cache_dir = Path("/mnt/consensus-cache")

    relays = {}
    files = sorted(consensus_cache_dir.iterdir())
    for idx, consensus_file in enumerate(files):
        print(f"{idx}/{len(files)}")
        if consensus_file.name == "last":
            continue

        with open(consensus_file, "rb") as f:
            try:
                descriptors = pickle.load(f)
                raise EOFError()
            except (pickle.UnpicklingError, EOFError):
                print(f"{consensus_file} couldn't be unpickled")
                time = dt.datetime.fromisoformat(consensus_file.name)
                descriptors = _read_relay_info(time)

        for entry in descriptors:
            (router_status_entry, descriptor) = entry
            fingerprint = descriptor.fingerprint
            relay = relays.get(fingerprint, None)
            if not relay:
                relay = {
                    "stable": "Stable" in router_status_entry.flags,
                    "tor_version": str(descriptor.tor_version),
                    "consensuses_since_first_seen": 0,
                    "consensuses_seen": 0,
                }
                relays[fingerprint] = relay
            relay["consensuses_seen"] += 1
        for relay in relays.values():
            relay["consensuses_since_first_seen"] += 1
    return relays


def _connect_to_db(db_url) -> Session:
    db_url = db_url or os.getenv("SHORTOR_DB_URL")
    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")

    print(f"Connecting to database: [{db_url}]")
    return make_session_from_url(db_url)


def _parse_args(argv: List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")
    return parser.parse_args(argv[1:])


def main(argv):
    args = _parse_args(argv)
    db = _connect_to_db(args.db_url)
    cache_path = Path("/tmp/uptimescache")
    if cache_path.exists():
        with cache_path.open("rb") as f:
            relays = pickle.load(f)
    else:
        relays = _get()
        with cache_path.open("wb") as f:
            pickle.dump(relays, f)
    with db.begin():
        for fp, relay_dict in relays.items():
            relay = db.query(Relay).where(Relay.fingerprint == fp).first()
            if not relay:
                continue
            relay.stable = relay_dict["stable"]
            relay.tor_version = relay_dict["tor_version"]
            relay.consensuses_seen = relay_dict["consensuses_seen"]
            relay.consensuses_total = relay_dict["consensuses_since_first_seen"]
            db.add(relay)
    breakpoint()
    print("YOO")
    # TODO: write to DB


if __name__ == "__main__":
    main(sys.argv)
