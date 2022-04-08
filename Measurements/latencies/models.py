# pylint: disable=too-few-public-methods
"""Define the data model for latency measurements.
"""
import enum

from typing import Optional

import sqlalchemy as sa
import sqlalchemy_repr

from sqlalchemy.schema import Index
from sqlalchemy.sql.expression import case
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, aliased
from sqlalchemy import (
    Column,
    Integer,
    Float,
    String,
    DateTime,
    CheckConstraint,
    Boolean,
    ForeignKey,
    Enum,
    Text,
)
from sqlalchemy_utils import create_materialized_view

from ting.utils import TingLeg


class _SqlalchemyEquals:
    """Mixin to give __hash__ and __eq__ to SQLAlchemy model classes."""

    def _get_dict(self):
        return {k: v for k, v in self.__dict__.items() if k != "_sa_instance_state"}

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            raise NotImplementedError
        return self._get_dict() == other._get_dict()

    def __hash__(self):
        return hash(tuple(sorted(self._get_dict().items())))


_convention = {
    "ix": "ix_%(column_0_label)s",
    "uq": "uq_%(table_name)s_%(column_0_name)s",
    "ck": "ck_%(table_name)s_%(constraint_name)s",
    "fk": "fk_%(table_name)s_%(column_0_name)s_%(referred_table_name)s",
    "pk": "pk_%(table_name)s",
}
Base = declarative_base(
    cls=(sqlalchemy_repr.RepresentableBase, _SqlalchemyEquals),
    metadata=sa.MetaData(_convention),
)


class Relay(Base, _SqlalchemyEquals):
    """A Tor relay.

    Parsed from a Tor consensus file.
    """

    __tablename__ = "relays"

    id = Column(Integer, primary_key=True)
    fingerprint = Column(String(length=40), unique=True, index=True)
    nickname = Column(String)
    address = Column(String)
    guard = Column(Boolean)
    exit = Column(Boolean)
    # Location data (from GeoLite2 IP database)
    latitude = Column(Float)
    longitude = Column(Float)
    country = Column(String(length=2))
    # Autonomous system number
    autonomous_system = Column(Integer)
    # Tor info
    bandwidth_average = Column(Integer)  # bytes/sec
    bandwidth_burst = Column(Integer)  # bytes/sec
    bandwidth_observed = Column(Integer)  # bytes/sec
    consensus_weight = Column(Integer)  # arbitrary unit
    #
    stable = Column(Boolean)
    consensuses_seen = Column(Integer)
    consensuses_total = Column(Integer)  # since first seen
    top1000 = Column(Boolean)
    tor_version = Column(String)

    @property
    def ip(self):
        return self.address.partition(":")[0]

    @property
    def orport(self):
        return self.address.partition(":")[2]

    # all batches that reference us, regardless of whether we're relay1 or relay2
    batches = relationship(
        "Batch",
        primaryjoin=(
            "or_(" "    Relay.id==Batch.relay1_id, " "    Relay.id==Batch.relay2_id" ")"
        ),
        cascade="all, delete-orphan",
    )


class BatchStatus(enum.Enum):
    PENDING = 0
    FAILED = 1
    SUCCEEDED = 2
    INPROGRESS = 3


class Batch(Base):
    """A "batch" of measurements sharing the same circuit.

    We take individual measurements to smooth out variations.
    """

    __tablename__ = "batches"
    __table_args__ = (
        CheckConstraint("start_time < end_time"),
        CheckConstraint("(error::text IS NULL) OR status = 'FAILED'::batchstatus"),
        Index("batches_relay_ids_idx", "relay1_id", "relay2_id"),
    )

    id = Column(Integer, primary_key=True)

    # hostname of the observer machine
    observer = Column(String)

    # When the batch was enqueued.
    created_time = Column(DateTime)
    # The time range the circuit was live.
    # start_time < end_time (see __table_args__)
    # Maybe represent as a time range?
    # https://stackoverflow.com/questions/43334186/sqlalchemy-representation-for-custom-postgres-range-type
    # Need to figure out how to deal with SQLite though. Maybe we just drop it
    # altogether.
    start_time = Column(DateTime)
    end_time = Column(DateTime)

    relay1_id = Column(Integer, ForeignKey("relays.id"))
    relay1 = relationship("Relay", foreign_keys=[relay1_id], back_populates="batches")
    relay2_id = Column(Integer, ForeignKey("relays.id"))
    relay2 = relationship("Relay", foreign_keys=[relay2_id], back_populates="batches")

    num_measurements = Column(Integer)

    Status = BatchStatus

    status = Column(Enum(Status), default=Status.PENDING)
    error = Column(Text)
    # For getting more info about failures: the Celery task ID.
    # You can use celery.result.AsyncResult to inspect the failed task.
    celery_request_id = Column(String)
    # Was this initiated by a spot check?
    spot_check = Column(Boolean, default=False)

    circuits = relationship(
        "Circuit",
        order_by="Circuit.id",
        back_populates="batch",
        cascade="all, delete-orphan",
    )


class CircuitStatus(enum.Enum):
    SUCCEEDED = 1
    MEASUREMENTS_FAILED = 2
    SETUP_FAILED = 3
    TEARDOWN_FAILED = 4


class Circuit(Base):
    """An individual circuit that was constructed.

    A batch comprises three legs: X, Y, and XY.

    A circuit represents one of those legs.
    """

    __tablename__ = "circuits"
    __table_args__ = (
        CheckConstraint("start_time < end_time"),
        CheckConstraint(
            "(error::text IS NULL) OR status != 'SUCCEEDED'::circuitstatus"
        ),
    )

    id = Column(Integer, primary_key=True)

    Status = CircuitStatus
    leg = Column(Enum(TingLeg))
    start_time = Column(DateTime)
    end_time = Column(DateTime)
    status = Column(Enum(Status))
    error = Column(Text)

    batch_id = Column(Integer, ForeignKey("batches.id"), index=True)
    batch = relationship("Batch", back_populates="circuits")

    measurements = relationship(
        "Measurement",
        order_by="Measurement.id",
        back_populates="circuit",
        cascade="all, delete-orphan",
    )


class Measurement(Base):
    """An individual latency measurement.

    Latency measurements may fail, in which case we have `latency=None`.
    """

    __tablename__ = "measurements"
    __table_args__ = (CheckConstraint("(error::text IS NULL) OR NOT success"),)

    id = Column(Integer, primary_key=True)
    # One-way latency (outgoing) in ms. If failed, null
    latency_out = Column(Integer)
    # One-way latency (inbound) in ms. If failed, null
    latency_back = Column(Integer)
    timestamp = Column(DateTime)
    circuit_id = Column(Integer, ForeignKey("circuits.id"), index=True)
    circuit = relationship("Circuit", back_populates="measurements")
    success = Column(Boolean)
    error = Column(Text)


# The pseudo-views. All computed from normalized data, but we keep up-to-date
# incrementally (TODO: update_views.py).


class IncrementalHighWaterMark(Base):
    __tablename__ = "incremental_high_water_mark"

    id = Column(Integer, primary_key=True)
    last_batch_end_time = Column(DateTime)


class PairwiseLatency(Base):
    __tablename__ = "pairwise_latency"
    __tableargs__ = (Index("relay1_id", "relay2_id"),)

    id = Column(Integer, primary_key=True)
    relay1_id = Column(Integer, ForeignKey("relays.id"))
    relay1 = relationship("Relay", primaryjoin=relay1_id == Relay.id)
    relay2_id = Column(Integer, ForeignKey("relays.id"))
    relay2 = relationship("Relay", primaryjoin=relay2_id == Relay.id)
    latency_rtt = Column(Integer)


class TripletLatency(Base):
    __tablename__ = "triplet_latency"
    __tableargs__ = (Index("relay_from_id", "relay_to_id", "relay_via_id"),)

    id = Column(Integer, primary_key=True)
    relay_from_id = Column(Integer, ForeignKey("relays.id"))
    relay_from = relationship("Relay", primaryjoin=relay_from_id == Relay.id)
    relay_to_id = Column(Integer, ForeignKey("relays.id"))
    relay_to = relationship("Relay", primaryjoin=relay_to_id == Relay.id)
    relay_via_id = Column(Integer, ForeignKey("relays.id"))
    relay_via = relationship("Relay", primaryjoin=relay_via_id == Relay.id)
    orig_latency_rtt = Column(Integer)
    via_latency_rtt = Column(Integer)
    speedup = Column(Integer, sa.Computed("orig_latency_rtt - via_latency_rtt"))


# def _min_latency_for_leg(leg: TingLeg):
#     return sa.func.min(
#         case((Circuit.leg == leg, Measurement.latency_out))
#     ) + sa.func.min(case((Circuit.leg == leg, Measurement.latency_back)))
#
#
# class PairwiseLatency(Base):
#     __tablename__ = "pairwise_latency_mv"
#     __selectable__ = (
#         sa.select(
#             [
#                 Batch.relay1_id.label("relay1"),
#                 Batch.relay2_id.label("relay2"),
#                 (
#                     _min_latency_for_leg(TingLeg.XY)
#                     - _min_latency_for_leg(TingLeg.X) / 2
#                     - _min_latency_for_leg(TingLeg.Y) / 2
#                 ).label("latency_roundtrip"),
#             ]
#         )
#         .group_by(Batch.relay1_id, Batch.relay2_id)
#         .where(Circuit.id == Measurement.circuit_id)
#         .where(Batch.id == Circuit.batch_id)
#         .where(Circuit.status == Circuit.Status.SUCCEEDED)
#         .where(Measurement.success == True)
#         .where(Batch.status == Batch.Status.SUCCEEDED)
#         .where(Batch.spot_check == False)
#     )
#     __table__ = create_materialized_view(
#         name=__tablename__,
#         selectable=__selectable__,
#         metadata=Base.metadata,
#     )
#
#
# SRC = aliased(Relay)
# DST = aliased(Relay)
# VIA = aliased(Relay)
# HOP1 = aliased(PairwiseLatency)
# HOP2 = aliased(PairwiseLatency)
# ORIG = aliased(PairwiseLatency)
#
#
# class TripletLatency(Base):
#     __tablename__ = "triplet_latency_mv"
#     __selectable__ = (
#         sa.select(
#             [
#                 SRC.id.label("src"),
#                 DST.id.label("dest"),
#                 VIA.id.label("via"),
#                 ORIG.latency_roundtrip.label("direct_latency_roundtrip"),
#                 (HOP1.latency_roundtrip + HOP2.latency_roundtrip).label(
#                     "via_latency_roundtrip"
#                 ),
#                 sa.func.greatest(
#                     ORIG.latency_roundtrip
#                     - (HOP1.latency_roundtrip + HOP2.latency_roundtrip),
#                     0,
#                 ).label("speedup"),
#             ]
#         )
#         .select_from(sa.join(SRC, DST, sa.true()).join(VIA, sa.true()))
#         .join(
#             HOP1,
#             sa.or_(
#                 sa.and_(HOP1.relay1 == SRC.id, HOP1.relay2 == VIA.id),
#                 sa.and_(HOP1.relay1 == VIA.id, HOP1.relay2 == SRC.id),
#             ),
#             isouter=True,
#         )
#         .join(
#             HOP2,
#             sa.or_(
#                 sa.and_(HOP2.relay1 == VIA.id, HOP2.relay2 == DST.id),
#                 sa.and_(HOP2.relay1 == DST.id, HOP2.relay2 == VIA.id),
#             ),
#             isouter=True,
#         )
#         .join(
#             ORIG,
#             sa.or_(
#                 sa.and_(ORIG.relay1 == SRC.id, ORIG.relay2 == DST.id),
#                 sa.and_(ORIG.relay1 == DST.id, ORIG.relay2 == SRC.id),
#             ),
#             isouter=True,
#         )
#         .where(SRC.id < DST.id)
#         .where(VIA.id != DST.id)
#         .where(VIA.id != SRC.id)
#     )
#     __table__ = create_materialized_view(
#         name=__tablename__,
#         selectable=__selectable__,
#         metadata=Base.metadata,
#     )
#
#
class ReliabilityRates(Base):
    __tablename__ = "pairwise_reliability"
    __selectable__ = (
        sa.select(
            [
                Batch.relay1_id.label("relay1"),
                Batch.relay2_id.label("relay2"),
                sa.func.count(Circuit.id.distinct()).label("total_circuits"),
                sa.func.count(
                    case(
                        (Circuit.status == Circuit.Status.SUCCEEDED, Circuit.id),
                    ).distinct()
                ).label("successful_circuits"),
                sa.func.count(Measurement.id.distinct()).label("total_measurements"),
                sa.func.count(
                    case(
                        (Measurement.success == True, Measurement.id),
                    ).distinct()
                ).label("successful_measurements"),
            ]
        )
        .group_by(Batch.relay1_id, Batch.relay2_id)
        .where(Circuit.id == Measurement.circuit_id)
        .where(Batch.id == Circuit.batch_id)
    )
    __table__ = create_materialized_view(
        name=__tablename__,
        selectable=__selectable__,
        metadata=Base.metadata,
    )


# # TODO: if you add non-materialized views, need to handle in db.py
VIEWS = {c.__tablename__: c for c in (ReliabilityRates,)}
