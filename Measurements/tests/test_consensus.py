"""Tests for latencies.consensus."""
import string

from typing import List, Set, Tuple
from pathlib import Path
from itertools import tee, combinations

from hypothesis import given, strategies as st

from latencies.consensus import generate_pairs


# In practice they're bigger but this makes debugging tests easier
fingerprints = st.text(sorted(set(string.hexdigits.upper())), min_size=6, max_size=6)


def _num_combinations(items: int):
    return (items * (items - 1)) / 2


@st.composite
def sets_with_pair_subsets(draw, sets):
    set_ = draw(sets)
    if len(set_) > 1:
        pairs = sorted(map(tuple, map(sorted, combinations(set_, 2))))
        subset = draw(st.sets(st.sampled_from(pairs), max_size=len(pairs)))
    else:
        subset = set()
    return (set(set_), subset)


@given(sets_with_pair_subsets(st.sets(fingerprints, max_size=20)))
def test_subsets_strategy(set_subset: Tuple[Set[str], Set[str]]):
    (set_, subset) = set_subset
    for (a, b) in subset:
        assert a in set_
        assert b in set_
        assert a < b


sets_subsets = sets_with_pair_subsets(st.sets(fingerprints, max_size=20))
counts = st.integers(min_value=0)


@given(sets_subsets, counts)
def test_generate_pairs_actual_pairs(set_subset, count: int):
    set_, subset = set_subset
    pairs = generate_pairs(set_, subset, count)
    for (a, b) in pairs:
        assert a in set_
        assert b in set_


@given(sets_subsets, counts)
def test_generate_pairs_no_repeats(set_subset, count: int):
    set_, subset = set_subset
    pairs = generate_pairs(set_, subset, count)
    assert len(pairs) == len(set(pairs))
    assert set(pairs).isdisjoint(subset)


@given(sets_subsets, counts)
def test_generate_pairs_sorted(set_subset, count: int):
    set_, subset = set_subset
    pairs = generate_pairs(set_, subset, count)
    for (a, b) in pairs:
        assert a < b


@given(sets_subsets, counts)
def test_generate_pairs_correct_count(set_subset, count: int):
    set_, subset = set_subset
    pairs = generate_pairs(set_, subset, count)
    # expected number of pairs:
    expected_count = min(count, _num_combinations(len(set_)) - len(subset))
    assert len(pairs) <= count  # redundant but more obvious condition
    assert len(pairs) == expected_count


@given(
    sets_with_pair_subsets(st.sets(fingerprints, min_size=3, max_size=20)),
    st.integers(min_value=2),
)
def test_generate_pairs_random_order(set_subset, count: int):
    set_, subset = set_subset
    while _num_combinations(len(set_)) - len(subset) < 2:
        # we need at least 2 pairs for random order to be meaningful
        subset.pop()
    pairses = set()
    for _ in range(10):
        pairs = generate_pairs(set_, subset, count)
        pairses.add(tuple(pairs))  # tuple makes it hashable
    assert len(pairses) > 1


@given(sets_subsets)
def test_generate_pairs_gives_all_pairs(set_subset):
    set_, subset = set_subset
    all_pairs = {tuple(sorted(p)) for p in combinations(set_, 2)}
    pairs = generate_pairs(set_, subset, len(all_pairs))
    assert set(pairs) | subset == all_pairs
