import argparse
import os
import sys
import functools
import logging
from typing import List, Iterable, Tuple
from sqlalchemy.sql import text
from sqlalchemy.orm.session import Session
from latencies.db import make_session_from_url
from ipaddress import IPV4LENGTH, IPv4Address


def _parse_args(argv: List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")
    args = parser.parse_args(argv[1:])
    if not args.db_url:
        args.db_url = os.getenv("SHORTOR_DB_URL")
    return args


def _connect_to_db(db_url: str) -> Session:
    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")

    print(f"Connecting to database: [{db_url}]")
    return make_session_from_url(db_url)


def _create_tables(db: Session):
    db.execute(text("DROP MATERIALIZED VIEW IF EXISTS shortor_vs_ripe"))
    db.execute(text("DROP TABLE IF EXISTS relays_ripenodes"))
    db.execute(text("DROP MATERIALIZED VIEW IF EXISTS ripenodes"))
    db.execute(text("DROP TABLE IF EXISTS ripe_allpairs"))
    db.execute(
        text(
            """
        CREATE TABLE ripe_allpairs (
            id SERIAL,
            src inet,
            src_city int,
            src_country VARCHAR(2),
            dst inet,
            dst_city int,
            dst_country VARCHAR(2),
            latency float
        )"""
        )
    )
    db.execute("CREATE INDEX ripe_allpairs_src_idx ON ripe_allpairs (src)")
    db.execute("CREATE INDEX ripe_allpairs_dst_idx ON ripe_allpairs (dst)")
    db.execute(
        text(
            """
                COPY ripe_allpairs (id, src, src_city, src_country, dst, dst_city, dst_country, latency)
                FROM :file
           DELIMITER ','
                 CSV HEADER
        """
        ),
        {"file": "/tmp/2018-all-pairs.csv"},
    )
    db.execute(
        text(
            """
        CREATE MATERIALIZED VIEW ripenodes AS (
            (  SELECT src AS ip,
                      MAX(src_city) AS city,
                      MAX(src_country) AS country
                 FROM ripe_allpairs
             GROUP BY src
            )
            UNION
            (  SELECT dst AS ip,
                      MAX(dst_city) AS city,
                      MAX(dst_country) AS country
                 FROM ripe_allpairs
             GROUP BY dst
            )
        ) WITH DATA"""
        )
    )
    db.execute("CREATE INDEX ripenodes_ip_idx ON ripenodes (ip)")
    db.execute(
        text(
            """CREATE TABLE relays_ripenodes (
            relay_ip inet PRIMARY KEY,
            ripe_ip inet
        )"""
        )
    )


def _relay_ips(db: Session) -> Iterable[Tuple[IPv4Address, str]]:
    return [
        (IPv4Address(ip), country)
        for (ip, country) in db.execute(
            text(
                """
        SELECT SPLIT_PART(address, ':', 1)::inet AS ip,
               country
          FROM relays r
        """
            )
        )
    ]


@functools.lru_cache()
def _all_ripenodes(db: Session) -> List[IPv4Address]:
    return [
        IPv4Address(ip_str)
        for (ip_str,) in db.execute(
            text("SELECT ip FROM ripenodes"),
        )
    ]


@functools.lru_cache()
def _ripenodes_for_country(db: Session, country: str) -> List[IPv4Address]:
    return [
        IPv4Address(ip_str)
        for (ip_str,) in db.execute(
            text("SELECT ip FROM ripenodes WHERE country = :country"),
            dict(country=country),
        )
    ]


def _prefix_match_len(ip1: IPv4Address, ip2: IPv4Address) -> int:
    return IPV4LENGTH - (int(ip1) ^ int(ip2)).bit_length()


def _closest_ip(ip: IPv4Address, ips: List[IPv4Address]) -> IPv4Address:
    return max(ips, key=functools.partial(_prefix_match_len, ip))


def _map_to_ripe_ip(
    db: Session, ip: IPv4Address, city: int, country: str
) -> IPv4Address:
    del city  # unused
    city_ips = []  # TODO: we don't have city data for relays
    if city_ips:
        return _closest_ip(ip, city_ips)
    country_ips = _ripenodes_for_country(db, country)
    if country_ips:
        return _closest_ip(ip, country_ips)
    all_ips = _all_ripenodes(db)
    return _closest_ip(ip, all_ips)


def main(argv: List[str]):
    args = _parse_args(argv)
    db = _connect_to_db(args.db_url)
    logging.basicConfig()
    logging.getLogger("sqlalchemy.engine").setLevel(logging.INFO)
    logging.getLogger("sqlalchemy.pool").setLevel(logging.DEBUG)
    done_relays = set()
    with db.begin():
        _create_tables(db)
        for (ip, country) in _relay_ips(db):
            if ip in done_relays:  # we can get >1 relay with the same IP
                continue
            done_relays.add(ip)
            closest_ip = _map_to_ripe_ip(db, ip, None, country)
            db.execute(
                text(
                    "INSERT INTO relays_ripenodes (relay_ip, ripe_ip) VALUES (:relay, :ripe)"
                ),
                dict(relay=str(ip), ripe=str(closest_ip)),
            )

        # TODO: average / 95%ile difference
        db.execute(
            text(
                """
                CREATE MATERIALIZED VIEW shortor_vs_ripe AS (
                    SELECT rc1.ripe_ip AS ip1,
                           rc2.ripe_ip AS ip2,
                           p.latency_rtt AS latency_measured,
                           c.latency AS latency_ripe
                      FROM pairwise_latency p
                      JOIN relays r1 ON p.relay1_id = r1.id
                      JOIN relays r2 ON p.relay2_id = r2.id
                      JOIN relays_ripenodes rc1
                        ON SPLIT_PART(r1.address, ':', 1)::inet = rc1.relay_ip
                      JOIN relays_ripenodes rc2
                        ON SPLIT_PART(r2.address, ':', 1)::inet = rc2.relay_ip
                      JOIN ripe_allpairs c
                        ON (c.src = rc1.ripe_ip AND c.dst = rc2.ripe_ip) OR
                           (c.src = rc2.ripe_ip AND c.dst = rc1.ripe_ip)
                ) WITH DATA
            """
            )
        )


if __name__ == "__main__":
    main(sys.argv)
