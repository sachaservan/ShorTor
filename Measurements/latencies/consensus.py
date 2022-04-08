"""Utilities for processing a Tor consensus."""
import itertools
import random
import logging

from dataclasses import dataclass
from typing import Tuple, Iterable, Set, List, cast
from operator import attrgetter

import geoip2.database
import geoip2.errors

from stem.descriptor.router_status_entry import RouterStatusEntryV3
from stem.descriptor.server_descriptor import RelayDescriptor

from latencies.models import Relay

log = logging.getLogger(__name__)
TRACE = logging.DEBUG - 5


@dataclass(frozen=True)
class DescriptorConverter:
    city_geoip: geoip2.database.Reader
    asn_geoip: geoip2.database.Reader

    def descriptor_to_relay(
        self, descriptor_tuple: Tuple[RouterStatusEntryV3, RelayDescriptor]
    ) -> Relay:
        """Converts stem RelayDescriptor to models.Relay."""
        log.log(TRACE, "Converting stem RelayDescriptor to models.Relay")
        (rse, descriptor) = descriptor_tuple
        relay = Relay(
            fingerprint=descriptor.fingerprint,
            nickname=descriptor.nickname,
            address=f"{descriptor.address}:{descriptor.or_port}",
            guard=("Guard" in rse.flags),
            exit=("Exit" in rse.flags),
            bandwidth_average=descriptor.average_bandwidth,
            bandwidth_burst=descriptor.burst_bandwidth,
            bandwidth_observed=descriptor.observed_bandwidth,
            consensus_weight=rse.bandwidth,
        )
        try:
            ip_info = self.city_geoip.city(descriptor.address)
        except geoip2.errors.AddressNotFoundError:
            log.error(f"Couldn't get city info for IP [{descriptor.address}]")
        else:
            relay.country = ip_info.country.iso_code
            relay.latitude = ip_info.location.latitude
            relay.longitude = ip_info.location.longitude
        try:
            ip_info = self.asn_geoip.asn(descriptor.address)
        except geoip2.errors.AddressNotFoundError:
            log.error(f"Couldn't get AS info for IP [{descriptor.address}]")
        else:
            relay.autonomous_system = ip_info.autonomous_system_number
        return relay


def generate_pairs(
    values: Set[Relay], completed_pairs: Set[Tuple[id, id]], count: int
) -> List[Tuple[Relay, Relay]]:
    """Return a list of (up to) `count` pairs from `values` in a random order.

    The values will *not* be in `completed_pairs`; the pairs will be sorted
    lexicographically.
    """
    if count < 0:
        raise ValueError(f"`count` must be >= 0; got {count}.")
    all_pairs: Iterable[Tuple[Relay, Relay]] = itertools.combinations(values, 2)

    # canonical order: lexicographic by fingerprint
    sort = lambda p: tuple(sorted(p, key=attrgetter("id")))
    all_pairs_sorted = cast(Iterable[Tuple[Relay, Relay]], map(sort, all_pairs))

    # skip pairs we've already seen
    is_unseen = lambda r: (r[0].id, r[1].id) not in completed_pairs
    new_pairs = set(filter(is_unseen, all_pairs_sorted))

    # TODO: skip pairs where they're both guard/exit nodes
    log.debug(f"Count of pairs (excluding completed ones): {len(new_pairs)}")

    log.debug(f"Returning random sample of {count} of those pairs.")
    count = min(count, len(new_pairs))
    return random.sample(new_pairs, min(len(new_pairs), count))
