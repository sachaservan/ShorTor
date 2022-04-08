"""The code for doing the latency measurments.

This is (will be) mostly an interface to Ting."""
from typing import Optional

from latencies.generate_fake_data import random_latency
from latencies.models import Relay


def measure_tor(relay1: Optional[Relay] = None, relay2: Optional[Relay] = None) -> int:
    """'Measure' latency between two Tor nodes.

    Right now this just makes fake data, and fails some of the time.
    """
    del relay1, relay2  # unused
    return random_latency()
