"""Utilities for interacting with a database.

We use SQLAlchemy for most database interaction.

Specify the database using a SQLAlchemy database URL[1] (based on RFC 1738[2])
in the `$SHORTOR_DB_URL` environment variable.

[1] https://docs.sqlalchemy.org/en/14/core/engines.html#database-urls
[2] http://rfc.net/rfc1738.html

Additionally, this module is runnable:

    $ python -m latencies.db shell
    >>> db.query(Relay).first()
    <Relay fingerprint='0006DE2E77E3C3EC5E1825B076E5257FD200ED0A', [...]
    >>> ^D
    $ python -m latencies.db destroy
    $ python -m latencies.db shell
    >>> db.query(Relay).first()
    [...]
    sqlalchemy.exc.OperationalError: [...]
    $ python -m latencies.db destroy
    $ python -m latencies.db shell
    >>> db.query(Relay).first() is None
    True
"""
import sys
import os
import code

from contextlib import contextmanager
from itertools import chain
from typing import Optional, List

import celery
import sqlalchemy as sa

from sqlalchemy import create_engine
from sqlalchemy.orm import scoped_session, sessionmaker, Session
from sqlalchemy_utils.view import DropView, CreateView

from latencies.models import Base, VIEWS


def _make_engine(db_url: Optional[str]):
    return create_engine(db_url or "sqlite:///:memory:")


def _make_session_cls(engine):
    return scoped_session(
        sessionmaker(
            autocommit=False, autoflush=False, bind=engine, expire_on_commit=False
        )
    )


def make_session_from_url(db_url: str) -> Session:
    return _make_session_cls(_make_engine(db_url))()


# pylint: disable=abstract-method
class SqlAlchemyTask(celery.Task):
    """Celery mixin to clean up SQLAlchemy session on teardown."""

    __session = None
    abstract = True

    @property
    def _session(self):
        if self.__session is not None:
            return self.__session
        self.__session = _make_session_cls(_make_engine(os.getenv("SHORTOR_DB_URL")))
        return self.__session

    @contextmanager
    def db(self):
        """Give a db handle."""
        db = self._session()
        try:
            yield db
        finally:
            self._session.remove()


# pylint: enable=abstract-method


def create(engine, *args):
    """Create the tables in our schema."""
    Base.metadata.create_all(engine)


def recreate(engine, table_names: List[str], *args):
    """Create the tables in our schema."""
    tables_recreate = dict()
    tables_create = dict()
    views_recreate = dict()
    views_create = dict()
    inspector = sa.inspect(engine)
    for table_name in table_names:
        for table_key, table in Base.metadata.tables.items():
            if table_name.lower() in table_key.lower():
                if inspector.has_table(table_key):
                    tables_recreate[table_key] = table
                else:
                    tables_create[table_key] = table
        for table_key, cls in VIEWS.items():
            if table_name.lower() in table_key.lower():
                if inspector.has_table(table_key):
                    views_recreate[table_key] = cls
                else:
                    views_create[table_key] = cls

    print(f"Tables to create: {list(tables_create.keys())}")
    print(f"View to create: {list(views_create.keys())}")
    if tables_recreate or views_recreate:
        print(
            f"Tables to recreate (WILL DROP EXISTING DATA): "
            f"{list(tables_recreate.keys())}"
        )
        print(
            f"Views to recreate (WILL DROP EXISTING DATA): "
            f"{list(views_recreate.keys())}"
        )
    elif not (tables_create or views_create):
        print("Nothing to do; exiting.")
        return
    if input("Continue? [y/N] ").lower() != "y":
        return
    for table in tables_recreate.values():
        table.drop(bind=engine)
    for table in chain(tables_recreate.values(), tables_create.values()):
        table.create(bind=engine, checkfirst=True)
    with engine.connect() as conn:
        for name in views_recreate.keys():
            conn.execute(DropView(name, materialized=True))
        for name, cls in chain(views_recreate.items(), views_create.items()):
            conn.execute(
                str(CreateView(name, cls.__selectable__, materialized=True))
                + " WITH NO DATA"
            )


def destroy(engine, table_names: List[str], *args):
    """Drop all tables in our schema.

    Does not drop tables *not* in our schema, so if we remove/rename a table, we
    need to clean it up by hand.
    """
    if not table_names:
        if input("Will drop all tables; continue? [y/N]").lower() != "y":
            return
        Base.metadata.drop_all(engine)
    inspector = sa.inspect(engine)
    tables_to_destroy = dict()
    views_to_destroy = dict()
    for table_name in table_names:
        for table_key, table in Base.metadata.tables.items():
            if table_name.lower() in table_key.lower():
                if inspector.has_table(table_key):
                    tables_to_destroy[table_key] = table
                else:
                    print("Ignoring table {table_key}...")
        for table_key, cls in VIEWS.items():
            if table_name.lower() in table_key.lower():
                if inspector.has_table(table_key):
                    views_to_destroy[table_key] = cls
                else:
                    print("Ignoring table {table_key}...")
    if not (tables_to_destroy or views_to_destroy):
        print("Nothing to do!")
        return
    print(f"Tables to drop: {tables_to_destroy.keys()}.")
    print(f"Views to drop: {views_to_destroy.keys()}.")
    if input("Continue? [y/N] ").lower() != "y":
        return
    for table in tables_to_destroy.values():
        table.drop(bind=engine)
    with engine.connect() as conn:
        for name in views_to_destroy.keys():
            conn.execute(DropView(name, materialized=True))


def shell(engine, *args):
    """Open a Python shell with a preloaded database connection."""
    # pylint: disable=possibly-unused-variable,import-outside-toplevel
    from latencies.models import (
        Batch,
        Measurement,
        Relay,
        PairwiseLatency,
        TripletLatency,
    )
    import sqlalchemy as sa

    db = _make_session_cls(engine)()
    code.interact(local=dict(locals(), **globals()))
    # pylint: enable=possibly-unused-variable,import-outside-toplevel


_COMMANDS = {"create": create, "destroy": destroy, "shell": shell, "recreate": recreate}
_DEFAULT_COMMAND = "shell"


def main(args):
    try:
        command_name = args[1]
    except IndexError:
        command_name = _DEFAULT_COMMAND

    try:
        command = _COMMANDS[command_name]
    except ValueError:
        print(
            "Should be one of {} (default {}).".format(
                list(_COMMANDS.keys()), _DEFAULT_COMMAND
            )
        )
    else:
        db = _make_engine(os.getenv("SHORTOR_DB_URL"))
        breakpoint()
        command(db, args[2:])


if __name__ == "__main__":
    main(sys.argv)
