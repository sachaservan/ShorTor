import pytest

from ipaddress import IPv4Address

from latencies.ripelatencies import _prefix_match_len

_testdata = [
    ("192.158.0.255", "192.158.0.255", 32),
    ("192.158.0.255", "192.158.0.254", 31),
    ("0.0.0.0", "255.255.255.255", 0),
    ("0.255.255.255", "255.255.255.255", 0),
    ("10.0.0.1", "10.255.0.1", 8),
    ("10.0.0.1", "10.0.255.1", 16),
    ("10.0.0.1", "10.0.0.255", 24),
]


@pytest.mark.parametrize("ip1,ip2,expected", _testdata)
def test_prefix_match_len(ip1: str, ip2: str, expected: int):
    assert _prefix_match_len(IPv4Address(ip1), IPv4Address(ip2)) == expected
