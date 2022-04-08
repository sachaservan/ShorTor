-- How many samples do we need to take of each circuit?
--
-- To answer that, measure convergence to the "true minimum" (estimated by the
-- min of 100 samples). If we take 1 sample, how close do we get? 2? etc.
--
-- Specifically, this query asks: if we take 10 samples, what percent of our
-- circuits are within 5% or 5ms of the true minimum?
--
-- Use with the data from num-measurements.gz in the data drive

WITH circuit_true_mins AS ( -- get the true minimum latency for each circuit
    SELECT
      circuit_id,
      MIN(latency_out) + MIN(latency_back) AS true_min
    FROM measurements
    GROUP BY circuit_id
  ), good_circuits AS ( -- get IDs of successful circuits with 100 measurements
    SELECT circuits.id
    FROM
      measurements
      JOIN circuits ON circuits.id = measurements.circuit_id
    WHERE
      circuits.status = 'SUCCEEDED'
    GROUP BY circuits.id
    HAVING count(*) = 100
  ), mins AS ( -- get the minimum latency among the first N measurements for each circuit
  SELECT
    numbered.circuit_id AS circuit_id,
    MIN(latency_out) + MIN(latency_back) AS curr_min,
    MIN(circuit_true_mins.true_min) AS true_min
  FROM
    (SELECT ROW_NUMBER() OVER (PARTITION BY circuit_id) AS r, t.* FROM measurements t) numbered
    JOIN circuit_true_mins ON numbered.circuit_id = circuit_true_mins.circuit_id
    JOIN good_circuits ON numbered.circuit_id = good_circuits.id
  WHERE numbered.r <= 10 -- take the first 10 rows
  GROUP BY numbered.circuit_id
  ), latency_diffs AS ( -- get absolute and % differences between N-measurements and true mins
    SELECT
      circuit_id,
      curr_min - true_min AS diff_abs,
      (curr_min - true_min)::float / true_min AS diff_pct
    FROM mins
  )
SELECT
  count(*)::float / (select count(*) from good_circuits) AS pct_converged
FROM latency_diffs
WHERE
  diff_pct < 0.05 OR -- within 5%
  diff_abs < 5;  -- or within 5ms
