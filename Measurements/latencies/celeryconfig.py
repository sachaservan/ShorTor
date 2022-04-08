"""Configuration for Celery (worker and client)."""
# pylint: disable=invalid-name
import os

result_backend = os.getenv("CELERY_BROKER_URL")
imports = ("latencies.tasks",)
broker_heartbeat = 0
broker_transport_options = {"queue_order_strategy": "priority"}
task_default_priority = 5
