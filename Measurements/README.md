# Latency Meassurements

The code for taking end-to-end latency measurements on the live Tor network
lives in this directory.

For an overview, see the [overall plan][plan] and [technical design][tech-design].

[plan]: https://docs.google.com/document/d/1kjl8MC_EsTJGbHG478iw1boQRXMh5nCwWZHv5BmBLWk/edit
[tech-design]: https://docs.google.com/document/d/1g_-xQvTqj7p0SV4EWXr8yvzYKqudYCu5Pg8khyKDZyg/edit#heading=h.eqimm36z60ec

## Components

This is how the [technical design][tech-design] document maps to our implementation:

- The "observer" is a [Celery] worker; run as
- The "consensus taker" is `latencies/consensus_taker.py`:
  ```shell
  python -m latencies.consensus_taker PATH_TO_CONSENSUS`
  ```
- We're using [Redis] as the broker for Celery
- Results go in a SQL database; we can use [SQLite] for testing or [PostgreSQL]
  in production. We use [SQLAlchemy] to abstract over the DB so they *should* be
  easy to swap out. See `latencies/models.py`.

[celery]: https://doc.celeryproject.org/
[redis]: https://redis.io/
[sqlite]: https://www.sqlite.org/
[postgresql]: https://www.postgresql.org/
[sqlalchemy]: https://www.sqlalchemy.org/

## Ting

Our fork of Ting is included as a [submodule].

Either clone with `--recurse-submodules` or after clone run
`git submodule update --init`.


[submodule]: https://git-scm.com/book/en/v2/Git-Tools-Submodules

## Local Testing and Development

Spin up Vagrant; run `vagrant ssh --command /app/test_vagrant.sh`. You can pass
a positional argument `sqlite` (default) or `postgresql` to determine which
database to use. Right now this:

1. Clears previous state (results database, broker state).
2. Runs the workers (2 or more) for a while.
3. Runs the consensus taker, which enqueues jobs via Redis and terminates.
4. Displays the contents of the results database.

### Requirements

You need Python (we're using 3.8) with the contents of `requirements.txt`
installed; I recommend a [venv]. `requirements-test.txt` has testing
dependencies and `requirements-dev.txt` has some other nice-to-haves for working
on the code ([`black`] auto-formatter, [`ipython`] shell, [`pylint`]).

The Vagrant machine has a suitable venv (only `requirements.txt`):
```shell
source /venv/bin/activate
```

You need Redis running; use
``` shell
export CELERY_BROKER_URL="redis://localhost:6379/0"
```
([format][broker-url]) to tell the workers about it (that's the right URL in Vagrant).

To configure a database, run
```shell
export SHORTOR_DB_URL="sqlite:////tmp/database.db"
```
([format][database-url]).


[venv]: https://docs.python.org/3/tutorial/venv.html
[black]: https://github.com/psf/black
[ipython]: https://ipython.org/
[pylint]: https://www.pylint.org/
[broker-url]: https://docs.celeryproject.org/en/stable/userguide/configuration.html#std-steting-broker_url
[database-url]: https://docs.sqlalchemy.org/en/14/core/engines.html#database-urls

### Things you can run

To run the components manually:

- worker: `celery start worker -A latencies -l INFO`
- consensus taker: `python -m latencies.consensus_taker --db-url $SHORTOR_DB_URL`
- database management: `python -m latencies.db COMMAND`:
  - `create`: create database schema in `$SHORTOR_DB_URL` database
  - `destroy`: delete data in `$SHORTOR_DB_URL` database
  - `shell` (default): pop a Python REPL with a database connection and the
    SQLAlchemy models loaded:
    ```python
    >>> db.query(Measurement).filter(Measurement.latency > 50).first().timestamp
    datetime.datetime(2021, 1, 20, 17, 40, 30, 979823)
    ```
- tests (with `requirements-test.txt` installed): `pytest`
- fake data generation: `python -m latencies.generate_fake_data --database $SHORTOR_DB_URL --relays 100`

# Deployment
To generate certs:
``` shell
cd devops && ./gen_certs.sh
```
Note that this will create new Tor relays that will take ~4 hours to end up in
the consensus, thus stalling measurements for that time.

# Safety/Monitoring
- kill switch
- error rate
- rate limiting
