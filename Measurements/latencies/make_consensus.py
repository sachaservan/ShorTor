import argparse
import base64
import binascii
import hashlib
import itertools
import logging
import os
import shutil
import sys

from typing import List, Any, Dict
from pathlib import Path

from latencies.db import make_session_from_url
from latencies.models import Relay

log = logging.getLogger(__name__)
TRACE = logging.DEBUG - 5
logging.addLevelName(TRACE, "TRACE")

_CONSENSUS_PREAMBLE = """\
network-status-version 3
vote-status consensus
consensus-method 30
valid-after 2021-07-01 00:00:00
fresh-until 2021-08-01 00:00:00
valid-until 2021-08-01 00:00:00
voting-delay 300 300
client-versions 0.3.5.10,0.3.5.11,0.3.5.12,0.3.5.13,0.3.5.14,0.3.5.15,0.4.4.1-alpha,0.4.4.2-alpha,0.4.4.3-alpha,0.4.4.4-rc,0.4.4.5,0.4.4.6,0.4.4.7,0.4.4.8,0.4.4.9,0.4.5.1-alpha,0.4.5.2-alpha,0.4.5.3-rc,0.4.5.4-rc,0.4.5.5-rc,0.4.5.6,0.4.5.7,0.4.5.8,0.4.5.9,0.4.6.1-alpha,0.4.6.2-alpha,0.4.6.3-rc,0.4.6.4-rc,0.4.6.5,0.4.6.6
server-versions 0.3.5.10,0.3.5.11,0.3.5.12,0.3.5.13,0.3.5.14,0.3.5.15,0.4.4.1-alpha,0.4.4.2-alpha,0.4.4.3-alpha,0.4.4.4-rc,0.4.4.5,0.4.4.6,0.4.4.7,0.4.4.8,0.4.4.9,0.4.5.1-alpha,0.4.5.2-alpha,0.4.5.3-rc,0.4.5.4-rc,0.4.5.5-rc,0.4.5.6,0.4.5.7,0.4.5.8,0.4.5.9,0.4.6.1-alpha,0.4.6.2-alpha,0.4.6.3-rc,0.4.6.4-rc,0.4.6.5,0.4.6.6
known-flags Authority BadExit Exit Fast Guard HSDir NoEdConsensus Running Stable StaleDesc Sybil V2Dir Valid
recommended-client-protocols Cons=2 Desc=2 DirCache=2 HSDir=2 HSIntro=4 HSRend=2 Link=4-5 Microdesc=2 Relay=2
recommended-relay-protocols Cons=2 Desc=2 DirCache=2 HSDir=2 HSIntro=4 HSRend=2 Link=4-5 LinkAuth=3 Microdesc=2 Relay=2
required-client-protocols Cons=2 Desc=2 Link=4 Microdesc=2 Relay=2
required-relay-protocols Cons=2 Desc=2 DirCache=2 HSDir=2 HSIntro=4 HSRend=2 Link=4-5 LinkAuth=3 Microdesc=2 Relay=2
params CircuitPriorityHalflifeMsec=30000 DoSCircuitCreationEnabled=1 DoSConnectionEnabled=1 DoSConnectionMaxConcurrentCount=50 DoSRefuseSingleHopClientRendezvous=1 ExtendByEd25519ID=1 KISTSchedRunInterval=2 NumDirectoryGuards=3 NumEntryGuards=1 NumNTorsPerTAP=100 UseOptimisticData=1 bwauthpid=1 cbttestfreq=10 hs_service_max_rdv_failures=1 hsdir_spread_store=4 pb_disablepct=0 sendme_emit_min_version=1 usecreatefast=0
shared-rand-previous-value 9 5kNriWFlC90z1tembcnEFuuQzDInujOTtoUMd1gu+JY=
shared-rand-current-value 9 S/KwI6CMw4qjqxNCUkdgh/7ukwPj9av9jG9F1SB2wbQ=
dir-source dannenberg 0232AF901C31A04EE9848595AF9BB7620D4C5B2E dannenberg.torauth.de 193.23.244.244 80 443
contact Andreas Lehner
vote-digest 6D59B85FFA2B54974B64727874B141312A2E1A28
dir-source tor26 14C131DFC5C6F93646BE72FA1401C02A8DF2E8B4 86.59.21.38 86.59.21.38 80 443
contact Peter Palfrader
vote-digest DA153AC312B75A17DF43A89A10769CB28B20332E
dir-source longclaw 23D15D965BC35114467363C165C4F724B64B4F66 199.58.81.140 199.58.81.140 80 443
contact Riseup Networks <collective at riseup dot net> - 1nNzekuHGGzBYRzyjfjFEfeisNvxkn4RT
vote-digest D4654859D59804668C6A1FAA5F102C00FB139174
"""

# NOTE: no dirport = 0
# NOTE: bandwidth = KBps
_CONSENSUS_ENTRY = """\
r {nickname} {ident} {digest} 2021-06-30 23:00:00 {ip} {orport} 0
s Fast Running Stable V2Dir Valid {flags}
v Tor 0.4.5.6
pr Cons=1-2 Desc=1-2 DirCache=2 FlowCtrl=1 HSDir=1-2 HSIntro=3-5 HSRend=1-2 Link=1-5 LinkAuth=1,3 Microdesc=1-2 Padding=2 Relay=1-3
w Bandwidth={bandwidth}
p {exit_policy}
"""

_CONSENSUS_POSTAMBLE = """\
directory-footer
bandwidth-weights Wbd={Wbd} Wbe={Wbe} Wbg={Wbg} Wbm={Wbm} Wdb={Wdb} Web={Web} Wed={Wed} Wee={Wee} Weg={Weg} Wem={Wem} Wgb={Wgb} Wgd={Wgd} Wgg={Wgg} Wgm={Wgm} Wmb={Wmb} Wmd={Wmd} Wme={Wme} Wmg={Wmg} Wmm={Wmm}
directory-signature 0232AF901C31A04EE9848595AF9BB7620D4C5B2E 0AF48E6865839B2529BFB19DB8F97AFB3AAD2FFD
-----BEGIN SIGNATURE-----
hMl2alz75FzjSCrPaugNUvI1GFu4AhknL7CESFM5+CzoUjVdDVCnJofXqzRsdXTW
8ZVomgDvf5hidU99bpihjx6n4PGIM12GoaOgw7OmKMx9pe6odh3XkTezwB+0cWBU
GdpFKh5eQb4bAb7mTNs67uv25c/OZMqbuN1sc6Uw6xchIJ/0sr4c7YNY/o85hfe9
ORjvzfYlTAdXoc037IFAzlebFuphudn12JNVzLBhArs3552tqTbf1qhyO+urxiEg
oWkXJxCJnM7NzInuUcJM0r3gyWYgoTykv0fva+X7ZHTNHqEdAjyzYg/T5Z0kJ19Y
uuJusIuqj1NQVLV7JYA/xg==
-----END SIGNATURE-----
directory-signature 14C131DFC5C6F93646BE72FA1401C02A8DF2E8B4 08C51820AE7A7457CD5C994E5D70F7153368520B
-----BEGIN SIGNATURE-----
CctuO0L4eLjHTzclWcuKuhV4WfQZtDeE2ZStRPD00VxvrmliMgF090YvtV3JORgY
0T6FDhSaBW+zU+NjvMkAQ0padOD3+GpS9M+StvmZm0kd3fR4T+eVx2qzfFOh4O/Q
wXscgz8whhWe2GrOCvif5B7Tc9mQwQx6MyGSSgeFSuRfEN/WTQO5l1vX8DrFgYMt
HnTXanmVT9me4hScMLjIhQrAeG7PlYUDzPGSJB9cGofFEDhsmg/Rw9M/bTVw0PT9
Z+/4sG1USGZxMs2FONzU3R2gDdrOM7shg9IRm0k8Qr+UfSAAB+RScii1V4v7LPMP
+Ko4SS4vT4vA6uDcYhdzcJA7V4MZaCEPkVyll5x16ihvySRO8Do1iPGwVYpGOwMS
84RB06NDm2cbBy5msFxRWbQtbg0PFEHIK3jb9E2iBby2qH4w+ZwCoEbrGMoTx0cn
rjClrCnkeOJdmD2XCXHwxA+n++A0mBX44yLeS8EMCpXpXBRQtv/AUl6Jxqp/EHac
-----END SIGNATURE-----
"""


def _get_bandwidth_weights(relays: Relay) -> Dict[str, int]:
    G = sum((r.consensus_weight for r in relays if r.guard))
    M = sum((r.consensus_weight for r in relays if (not r.guard) and (not r.exit)))
    E = sum((r.consensus_weight for r in relays if r.exit))
    D = sum((r.consensus_weight for r in relays if r.guard and r.exit))
    T = G + M + E + D

    weight_scale = 10000

    if E >= T // 3 and G >= T // 3:
        # case 1: neither exit not guard scarce
        Wgd = weight_scale // 3
        Wed = weight_scale // 3
        Wmd = weight_scale // 3
        Wee = (weight_scale * (E + G + M)) // (3 * E)
        Wme = weight_scale - Wee
        Wmg = (weight_scale * (2 * G - E - M)) // (3 * G)
        Wgg = weight_scale - Wmg
    elif E < T // 3 and G < T // 3:
        R = min(G, E)
        S = max(G, E)
        if R + D < S:
            Wgg = Wee = weight_scale
            Wmg = Wme = Wmd = 0
            if E < G:
                Wed = weight_scale
                Wgd = 0
            else:
                Wed = 0
                Wgd = weight_scale
        else:
            Wee = (weight_scale * (E - G + M)) // E
            Wed = (weight_scale * (D - 2 * E + 4 * G - 2 * M)) // (3 * D)
            Wme = (weight_scale * (G - M)) // E
            Wmg = 0
            Wgg = weight_scale
            Wmd = (weight_scale - Wed) // 2
            Wgd = (weight_scale - Wed) // 2

            _invalid = lambda num: num not in range(0, weight_scale)
            if any(map(_invalid, [Wee, Wed, Wme, Wmg, Wgg, Wmd, Wgd])):
                Wgg = weight_scale
                Wee = weight_scale
                Wed = (weight_scale * (D - 2 * E + G + M)) // (3 * D)
                Wmd = (weight_scale * (D - 2 * M + G + E)) // (3 * D)
                Wme = 0
                Wmg = 0
                Wgd = weight_scale - Wed - Wmd

            if Wmd < 0:
                assert M > T // 3
                Wmd = 0
                Wgd = weight_scale - Wed
    else:
        S = min(E, G)
        if S + D < T // 3:
            if G == S:
                Wgg = Wgd = weight_scale
                Wmd = Wed = Wmg = 0
                if E < M:
                    Wme = 0
                else:
                    Wme = (weight_scale * (E - M)) // (2 * E)
                Wee = weight_scale - Wme
            else:
                Wee = Wed = weight_scale
                Wmd = Wgd = Wme = 0
                if G < M:
                    Wmg = 0
                else:
                    Wmg = (weight_scale * (G - M)) // (2 * G)
                Wgg = weight_scale - Wmg
        else:
            if G == S:
                Wgg = weight_scale
                Wgd = (weight_scale * (D - 2 * G + E + M)) // (3 * D)
                Wmg = 0
                Wee = (weight_scale * (E + M)) // (2 * E)
                Wme = weight_scale - Wee
                Wmd = (weight_scale - Wgd) // 2
                Wed = (weight_scale - Wgd) // 2
            else:
                Wee = weight_scale
                Wed = (weight_scale * (D - 2 * E + G + M)) // (3 * D)
                Wme = 0
                Wgg = (weight_scale * (G + M)) // (2 * G)
                Wmg = weight_scale - Wgg
                Wmd = (weight_scale - Wed) // 2
                Wgd = (weight_scale - Wed) // 2

    Wdb = weight_scale
    Web = weight_scale
    Wem = weight_scale
    Wgb = weight_scale
    Wmb = weight_scale
    Wmm = weight_scale
    Wbd = Wmd
    Wbg = Wmg
    Wbe = Wme
    Wbm = Wmm
    Wgm = Wgg
    Wem = Wee
    Weg = Wed

    return {k: v for k, v in locals().items() if k.startswith("W")}


def _make_consensus(db: Any, path: Path, descriptors: Dict[int, str]):
    with path.open("w") as f:
        f.write(_CONSENSUS_PREAMBLE)
        relays = list(db.query(Relay).all())
        for relay in relays:
            flags = []
            if relay.guard:
                flags.append("Guard")
            if relay.exit:
                flags.append("Exit")
            ident = (
                base64.encodebytes(binascii.unhexlify(relay.fingerprint))
                .rstrip(b"\n=")
                .decode("ascii")
            )
            if relay.exit:
                exit_policy = "accept"
            else:
                exit_policy = "reject 1-65535"
            digest = (
                base64.encodebytes(_digest_descriptor(descriptors[relay.id]))
                .rstrip(b"\n=")
                .decode("ascii")
            )
            f.write(
                _CONSENSUS_ENTRY.format(
                    nickname=relay.nickname,
                    ident=ident,
                    digest=digest,
                    ip=relay.ip,
                    orport=relay.orport,
                    bandwidth=relay.consensus_weight,
                    flags=" ".join(flags),
                    exit_policy=exit_policy,
                )
            )
        bandwidth_weights = _get_bandwidth_weights(relays)
        f.write(_CONSENSUS_POSTAMBLE.format(**bandwidth_weights))


_DESCRIPTOR_TEMPLATE = """\
@type server-descriptor 1.0
router {nickname} {ip} {orport} 0 8443
identity-ed25519
-----BEGIN ED25519 CERT-----
AQQABuZ+AftHefE7JJjwUfESUkqhzZ/XWxznmVdM5YhW6b+KSO1nAQAgBABOR/ou
jyuX41cbL+pjqv2ARgDp89N7/DhvU3GrIxeiKOozLQwvUXH+60xyuS5W0RijvTU3
GQ8Uanmmy8X1S1SYGy7ZI/a33eCcqRyOt1lyHgA5otVHcr3hef5UdUorEAk=
-----END ED25519 CERT-----
master-key-ed25519 Tkf6Lo8rl+NXGy/qY6r9gEYA6fPTe/w4b1NxqyMXoig
platform Tor 0.4.4.5 on Linux
proto Cons=1-2 Desc=1-2 DirCache=1-2 HSDir=1-2 HSIntro=3-4 HSRend=1-2 Link=1-5 LinkAuth=1,3 Microdesc=1-2 Relay=1-2
published 2021-06-30 23:00:00
fingerprint {fingerprint}
uptime 11779325
bandwidth {bandwidth_average} {bandwidth_burst} {bandwidth_observed}
extra-info-digest C3B6CFECC4E5F3B27971C4436D2F8C1878B0C66F nywt1hylzGzH6HrlCfw39WM5nILfsYiaNFTIkBb3RfQ
onion-key
-----BEGIN RSA PUBLIC KEY-----
MIGJAoGBAMusq8VfL0my1AnzbGgEdZiftpB8ZTJrTPF3+GTwNFu4HUKKVBOEIQjr
R8LvFbTs3ofXq+S5SGi3e/pIuJBU4gl0/V8FfiI8PLRlBStUenpoJWl8DvKH4rdn
9cGnk69FzXPNTShz2k2z0/TiXBR7/jDQ2U5Y5mg7k0T+4gdRkPsNAgMBAAE=
-----END RSA PUBLIC KEY-----
signing-key
-----BEGIN RSA PUBLIC KEY-----
MIGJAoGBALqxUEre1O1Cva/gq2aqHqW6ZMp6qySrNurtLsAj4KPgVmmCm2/YcO0N
e/HGsxYFNaRC7gAAGQyWsnKjxLT8ubBPHmIlGiM5E83W5totJdWX3zgfG2IlrxGS
NAjDoBR2CHMXx3Eh6dhvylSV8Xd/Khiy9YxgxsXaC5i2n2KyMP9VAgMBAAE=
-----END RSA PUBLIC KEY-----
onion-key-crosscert
-----BEGIN CROSSCERT-----
SVnQTiSeT4NIzk3/eXSjd9h4LsnCmuSp1hTrH+TIj7ZACMzBF4OTv6hT/qgwkkZd
ckNUHS/YoQ3Bd3t1NTojDsp57huyGFHH74qPnGizCj0zEd8qzv4EgQ1gxGxRZXVU
LjOwJzUfvbJHHh7XN4k6CM6bqCf+rHX1SwFBqGy/FcU=
-----END CROSSCERT-----
ntor-onion-key-crosscert 0
-----BEGIN ED25519 CERT-----
AQoABuZcAU5H+i6PK5fjVxsv6mOq/YBGAOnz03v8OG9TcasjF6IoAGUbpkpLyPU7
iZrjhq1TQiSQIE9m6njA4uekIYelHdSVRgBHRkRklTvfv7pQuO7V9nJZhT6k+5q0
mPddj1uQhgc=
-----END ED25519 CERT-----
hidden-service-dir
ntor-onion-key 0H97l/abJE5GUoBKH1KGekJsIo5P+GiJfORpXL86vj4=
{exit_policy} *:*
tunnelled-dir-server
router-sig-ed25519 4WNMxwEeYAnCvVQYZDVw8IS7+Oin77r++FCNHNLZvMKo+xwlTDzHMpeDx2cgmTGUjlsoyUVMPOkMoufEmU/UBg
router-signature
-----BEGIN SIGNATURE-----
gtGEP+9Noa5RjqN+mRU70PwGa6JeqqOjITbiY0VtK6UvKYJH6TKVe96aylupYP2y
OqkU0hgRPe7/847rO206lIwTqxbcbEV9HLV4szoTCvXT8LlUss4SqZntow1UB+ET
2YKuJQvuhdZQ8yTLB0dj4JydFRUfCuOqnuPtRv3CoLI=
-----END SIGNATURE-----

"""

# https://docs.python.org/3/library/itertools.htm#itertools-recipes
def _grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return itertools.zip_longest(*args, fillvalue=fillvalue)


def _digest_descriptor(descriptor: str) -> bytes:
    to_hash = descriptor[
        descriptor.index("router") : descriptor.rindex("router-signature\n")
        + len("router-signature\n")
    ].encode("ascii")
    return hashlib.sha1(to_hash).digest()


def _make_descriptors(
    db: Any, descriptor_dir: Path, force_delete=False
) -> Dict[int, str]:
    try:
        descriptor_dir.mkdir(exist_ok=False)
    except FileExistsError:
        if not force_delete:
            msg = f"Directory {descriptor_dir} exists. Delete? [y/N] "
            if input(msg).lower() != "y":
                raise
        shutil.rmtree(str(descriptor_dir))
        descriptor_dir.mkdir()

    descriptors = {}

    for relay in db.query(Relay).all():
        fingerprint = " ".join(map("".join, _grouper(relay.fingerprint.upper(), 4)))
        descriptor = _DESCRIPTOR_TEMPLATE.format(
            nickname=relay.nickname,
            ip=relay.ip,
            orport=relay.orport,
            fingerprint=fingerprint,
            bandwidth_average=relay.bandwidth_average,
            bandwidth_burst=relay.bandwidth_burst,
            bandwidth_observed=relay.bandwidth_observed,
            exit_policy="accept" if relay.exit else "reject",
        )

        digest = binascii.hexlify(_digest_descriptor(descriptor)).decode("ascii")
        fpath = descriptor_dir / digest[0].lower() / digest[1].lower() / digest.lower()
        fpath.parent.mkdir(parents=True, exist_ok=True)
        with fpath.open("w") as f:
            f.write(descriptor)

        descriptors[relay.id] = descriptor

    return descriptors


def _parse_args(argv: List[str]):
    parser = argparse.ArgumentParser()
    parser.add_argument("--db-url", type=str, help="DB URL (`sqlite:////tmp/test.db`)")

    parser.add_argument(
        "consensus_file",
        type=Path,
        nargs="?",
        default=Path("consensus.txt"),
        help="File to dump consensus to",
    )

    parser.add_argument(
        "descriptor_dir",
        type=Path,
        nargs="?",
        default=Path("server-descriptors"),
        help="Directory to dump server descriptors to",
    )

    return parser.parse_args(argv[1:])


def _configure_logging():
    logging.basicConfig(
        level=getattr(logging, os.getenv("SHORTOR_LOG_LEVEL", "DEBUG").upper())
    )


def _connect_to_db(db_url):
    db_url = db_url or os.getenv("SHORTOR_DB_URL")
    if not db_url:
        raise RuntimeError("Must provide --db-url or $SHORTOR_DB_URL.")

    log.info(f"Connecting to database: [{db_url}]")
    return make_session_from_url(db_url)


def main(argv: List[str]):
    args = _parse_args(argv)
    _configure_logging()
    db = _connect_to_db(args.db_url)
    descriptors = _make_descriptors(db, args.descriptor_dir)
    _make_consensus(db, args.consensus_file, descriptors)


# from stem.descriptor import parse_file as parse_consensus_file
# from stem.descriptor import DocumentHandler
# for entry in parse_consensus_file(
#     "/tmp/consensus.txt",
#     "network-status-consensus-3 1.0",
#     document_handler=DocumentHandler.ENTRIES,
# ):
#     print(entry.nickname)


if __name__ == "__main__":
    main(sys.argv)
