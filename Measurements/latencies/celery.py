"""Celery worker.

Nothing interesting to see here; configuration is in `latencies.celeryconfig`.
"""
from celery import Celery

app = Celery("latencies")
app.config_from_object("latencies.celeryconfig")

if __name__ == "__main__":
    app.start()
