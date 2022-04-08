"""Define celery tasks.

Think of this as containing the MVC "controller."

The only task right now is "measure": given two relay fingerprints, record (in
the DB) a batch of measurements between them.
"""
import datetime
import os
import logging
import socket
import traceback
import functools
import contextlib


from contextlib import contextmanager
from typing import Any

from throttle_client import lock
from celery.utils.log import get_task_logger
from celery.signals import celeryd_after_setup
from sqlalchemy.orm import joinedload
from sqlalchemy.orm.session import Session

import stem
import ting
import sqlalchemy as sa

from ting.echo_server import echo_server_background
from ting.client import TingClient
from ting.circuit import TingCircuit
from ting.utils import TingLeg
from stem.control import Controller
import stem.descriptor.remote


from latencies.celery import app
from latencies.models import Measurement, Batch, Relay, Circuit
from latencies.db import SqlAlchemyTask

log = get_task_logger(__name__)
logging.getLogger("ting").setLevel(logging.INFO)

WORKER_NAME = None
THROTTLE_URL = os.getenv("SHORTOR_THROTTLE_URL")
CIRCUIT_LOCK = "TOR_CIRCUITS"
_MS_PER_S = 1000
_MAX_RELAY_MEASUREMENT_COUNT = 10

_BRIDGE_ORPORT = int(os.getenv("SHORTOR_BRIDGE_ORPORT", 5100))
_EXIT_ORPORT = int(os.getenv("SHORTOR_EXIT_ORPORT", 5101))
_SOCKS_PORT = int(os.getenv("SHORTOR_SOCKS_PORT", 9101))


@celeryd_after_setup.connect
def capture_worker_name(sender, *args, **kwargs):
    """Grab the worker name so we can include measurement metadata in the database."""
    # This is the easiest way to do this, AFAICT...
    del args, kwargs  # unused
    global WORKER_NAME  # pylint: disable=global-statement
    WORKER_NAME = str(sender)


@contextmanager
def lock_circuit_count(throttle_url: str):
    """Obtain a semaphore lock on Tor circuit count.

    To manage load, we want no more than X circuits open at a time.
    We use a service called Throttle to do this ($SHORTOR_THROTTLE_URL).

    This blocks until we have the lock.

    If $SHORTOR_THROTTLE_URL=0, then don't block.
    """
    if not throttle_url:
        raise RuntimeError("Throttle URL not set")
    if throttle_url == "0":
        # Special value indicating not to use Throttle (for local testing etc.)
        yield
    else:
        with lock(throttle_url, CIRCUIT_LOCK):
            yield


@functools.lru_cache(maxsize=32)
def _get_local_fingerprint(port: int):
    query = stem.descriptor.remote.their_server_descriptor(
        endpoints=[stem.ORPort("127.0.0.1", port)]
    )
    descriptors = list(query)
    if query.error is not None:
        raise RuntimeError(f"Could not get fingerprint: {query.error}")
    if len(descriptors) != 1:
        raise RuntimeError(
            f"Bad descriptor list: expected list of length 1; got {descriptors}."
        )
    return descriptors[0].fingerprint


def _take_measurement(sampler: Any, circuit_record: Circuit, db: Session, message: Any):
    result = Measurement(timestamp=datetime.datetime.utcnow(), circuit=circuit_record)
    db.add(result)
    try:
        log.debug(f"Sampling circuit {circuit_record.leg}")
        message(f"Sampling circuit {circuit_record.leg}")
        timings = sampler()
    except Exception:
        result.error = traceback.format_exc()
        result.success = False
    else:
        result.success = True
        result.latency_out = int(timings[0] * _MS_PER_S)
        result.latency_back = int(timings[1] * _MS_PER_S)
    return result


def _measure_leg(
    circuit_template: TingCircuit, batch: Batch, db: Session, message: Any
):
    log.debug("Opening circuit: %s", circuit_template)
    circuit_record = Circuit(
        leg=circuit_template.leg, start_time=datetime.datetime.utcnow(), batch=batch
    )
    db.add(circuit_record)
    circuit_created = False
    try:
        with circuit_template.build() as sampler:
            circuit_created = True
            message("Circuit opened.")
            log.debug("Circuit opened: %s", circuit_template)
            for _ in range(batch.num_measurements):
                _take_measurement(sampler, circuit_record, db, message)
            log.debug("Closing circuit: %s", circuit_template)
            message("Closing circuit.")
    except Exception:
        if circuit_created:
            circuit_record.status = Circuit.Status.TEARDOWN_FAILED
        else:
            circuit_record.status = Circuit.Status.SETUP_FAILED
        circuit_record.error = traceback.format_exc()
    else:
        if all(m.success for m in circuit_record.measurements):
            circuit_record.status = Circuit.Status.SUCCEEDED
        else:
            circuit_record.status = Circuit.Status.MEASUREMENTS_FAILED
            circuit_record.error = "Some measurement failed."
    circuit_record.end_time = datetime.datetime.utcnow()
    return circuit_record


def _get_my_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 1))  # connect() for UDP doesn't send packets
    return s.getsockname()[0]


def _get_my_dest_ip():
    env_var = os.getenv("SHORTOR_ECHO_SERVER_IP")
    if env_var:
        return env_var
    return None


def _check_relays_online(controller: Controller, batch: Batch) -> bool:
    for relay in (batch.relay1, batch.relay2):
        try:
            controller.get_server_descriptor(relay.fingerprint)
        except stem.DescriptorUnavailable:
            msg = f"Relay {relay.fingerprint} not in consensus."
            log.exception(msg)
            batch.status = Batch.Status.FAILED
            batch.error = msg
            return False
    return True


def _prior_measurement_query(
    db: Session, relay_id: int, batch_relay_id_col: Any, leg: TingLeg
) -> Any:
    return (
        db.query(Batch)
        .where(Batch.status == Batch.Status.SUCCEEDED)
        .where(Batch.spot_check == False)
        .where(batch_relay_id_col == relay_id)
        .join(Circuit, Circuit.batch_id == Batch.id)
        .where(Circuit.leg == leg)
        .where(Circuit.status == Circuit.Status.SUCCEEDED)
    )


def _num_prior_measurements(db: Session, relay_id: int) -> int:
    return (
        _prior_measurement_query(db, relay_id, Batch.relay1_id, TingLeg.X).count()
        + _prior_measurement_query(db, relay_id, Batch.relay2_id, TingLeg.Y).count()
    )


def _measure_batch(batch: Batch, db: Session, message: Any):
    controller = Controller.from_socket_file("/run/tor/control")
    with contextlib.closing(controller):
        controller.authenticate()
        if not _check_relays_online(controller, batch):
            return
        with echo_server_background(host=_get_my_ip()) as echo_server:
            dest_ip = _get_my_dest_ip()
            if dest_ip:
                # Sometimes we have a different bind IP / Tor destination IP.
                # e.g., we use 10.0.0.X for binding, and some public IP for our
                # destination
                echo_server.host = dest_ip
            bridge_fp = _get_local_fingerprint(_BRIDGE_ORPORT)
            exit_fp = _get_local_fingerprint(_EXIT_ORPORT)
            ting_client = TingClient(
                bridge_fp,
                exit_fp,
                echo_server,
                controller,
                SocksPort=_SOCKS_PORT,
            )
            log.debug(f"Ting client created; Bridge=[{bridge_fp}] and Exit=[{exit_fp}]")
            relay1: Relay = batch.relay1
            relay2: Relay = batch.relay2
            circuit_templates = ting_client.generate_circuit_templates(
                relay1.fingerprint, relay2.fingerprint
            )
            for circuit_template in circuit_templates:
                for (leg, id_) in ((TingLeg.X, relay1.id), (TingLeg.Y, relay2.id)):
                    if circuit_template.leg == leg:
                        count = _num_prior_measurements(db, id_)
                        if count > _MAX_RELAY_MEASUREMENT_COUNT:
                            log.debug(
                                f"Skipping leg {circuit_template.leg}; relay {relay1.id} has been measured {count} times."
                            )
                            continue
                message(
                    f"Preparing to measure leg {circuit_template.leg}; waiting for lock"
                )
                with lock_circuit_count(THROTTLE_URL):
                    message(f"Lock acquired for leg {circuit_template.leg}.")
                    _measure_leg(circuit_template, batch, db, message)


@app.task(
    base=SqlAlchemyTask,
    bind=True,
)
def measure(self, batch_id: int):
    """Record a measurement between two Tor nodes and store in the database."""

    def _message(msg: str):
        self.update_state(state="PROGRESS", meta={"message": msg})

    _message(f"Accepted by {WORKER_NAME}")
    with self.db() as db:
        batch = (
            db.query(Batch)
            .options(joinedload(Batch.relay1))
            .options(joinedload(Batch.relay2))
            .get(batch_id)
        )
        batch.observer = WORKER_NAME
        batch.celery_reqest_id = self.request.id
        relay1: Relay = batch.relay1
        relay2: Relay = batch.relay2
        log.info(
            "Received batch: %d: %s<->%s",
            batch.id,
            relay1.fingerprint,
            relay2.fingerprint,
        )
        if batch.status is not Batch.Status.PENDING:
            log.error("Batch %d not ready for us (status %s).", batch.id, batch.status)
            return
        batch.start_time = datetime.datetime.utcnow()
        batch.status = Batch.Status.INPROGRESS
        db.add(batch)
        db.commit()
        db.add(batch)
        try:
            _measure_batch(batch, db, _message)
        except Exception as exc:
            log.exception(
                "Error measuring %s<->%s",
                relay1.fingerprint,
                relay2.fingerprint,
            )
            batch.status = Batch.Status.FAILED
            batch.error = traceback.print_exc()
            raise exc
        else:
            if batch.status == Batch.Status.FAILED:
                pass  # failed internally; let it be
            elif all(c.status == c.Status.SUCCEEDED for c in batch.circuits):
                batch.status = Batch.Status.SUCCEEDED
            else:
                batch.status = Batch.Status.FAILED
                batch.error = "Some circuit failed."
        finally:
            # store even failed runs
            batch.end_time = datetime.datetime.utcnow()
            db.commit()
