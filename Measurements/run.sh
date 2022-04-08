#!/bin/bash
# Run the e2e flow.
#
# Configuration:
# - CELERY_BROKER_URL
# - SHORTOR_DB_URL: DBAPI URL
# - CONSENSUS_FILE: path to consensus.txt file
# - RUN_TIME: how many seconds to let the workers run for
set -eo pipefail

export SHORTOR_THROTTLE_URL=0

echo "Running e2e with:"
echo "CELERY_BROKER_URL: $CELERY_BROKER_URL"
echo "SHORTOR_DB_URL: $SHORTOR_DB_URL"
echo "CONSENSUS_FILE: $CONSENSUS_FILE"

echo "Clearing state from prior runs (DB, errant workers)..."
REDIS_HOST=$(echo $CELERY_BROKER_URL | sed "s'redis://\(.*\):.*'\1'")
redis-cli -h $REDIS_HOST FLUSHALL

python -m latencies.db destroy
python -m latencies.db create

(
  celery multi start 2 worker -A latencies -l INFO

  sleep $RUN_TIME

  celery multi stopwait 2 worker -A latencies -l INFO
)&
SUBSHELL_PID=$!

sleep 0.1

for i in {1..3}; do
  python -m latencies.consensus_taker --db-url $SHORTOR_DB_URL --consensus-file $CONSENSUS_FILE
  sleep 2
done

wait $SUBSHELL_PID

if [[ $SHORTOR_DB_URL == postgresql* ]]; then
  psql --command "SELECT * FROM measurements;" | cat
  psql --command "SELECT * FROM batches;" | cat
  psql --command "SELECT * FROM relays;" | cat
fi

if [[ $SHORTOR_DB_URL == sqlite* ]]; then
  DB_PATH=$(echo $SHORTOR_DB_URL | sed "s'sqlite:///''g")
  sqlite3 $DB_PATH \
    -cmd ".headers on" \
    -cmd ".mode columns" \
    "SELECT * FROM measurements; SELECT * FROM batches; SELECT * FROM relays;"
fi
