#define TEST_NAME Types

#include "stdafx.h"
#include <stdlib.h>
#include <ctime>

#include <types/symmetric_matrix.hpp>

#define MATRIX_SIZE 15 // Must be >= 2

struct Matrices
{
	SymmetricMatrix <int> m; // Non-diagonal symmetric matrix
	SymmetricMatrix <int> d; // Diagonal symmetric matrix
	SymmetricMatrix <int> g; // Graph-like matrix

	Matrices() : m(MATRIX_SIZE, false), d(MATRIX_SIZE, true), g(MATRIX_SIZE, true)
	{
		srand (time(NULL));

		// Filling matrices m & d with random values
		for (int i = 0; i < MATRIX_SIZE; ++i)
		{
			for (int j = i; j < MATRIX_SIZE; ++j)
			{
				if (i != j) {
					m[i][j] = rand() % 100;
				}

				d[i][j] = rand() % 100;
			}
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(SymmetricMatrixSuite, Matrices)

BOOST_AUTO_TEST_CASE(Integrity)
{
	// Check if symmetric values are the same
	for (int i = 0; i < MATRIX_SIZE; ++i)
	{
		for (int j = i; j < MATRIX_SIZE; ++j)
		{
			if (i != j) {
				BOOST_CHECK_EQUAL(m[i][j], m[j][i]);
			}

			BOOST_CHECK_EQUAL(d[i][j], d[j][i]);
		}
	}

	// Check sizes
	BOOST_CHECK_EQUAL(m.size(), MATRIX_SIZE);
	BOOST_CHECK_EQUAL(d.size(), MATRIX_SIZE);
	BOOST_CHECK_EQUAL(g.size(), MATRIX_SIZE);

	// Check counts
	BOOST_CHECK_EQUAL(m.count(), ((MATRIX_SIZE * MATRIX_SIZE) - MATRIX_SIZE) / 2);
	BOOST_CHECK_EQUAL(d.count(), (((MATRIX_SIZE * MATRIX_SIZE) - MATRIX_SIZE) / 2) + MATRIX_SIZE);
	BOOST_CHECK_EQUAL(g.count(), 0);
}

BOOST_AUTO_TEST_CASE(Modifications)
{
	m[0][1] = 42;
	BOOST_CHECK_EQUAL(m[0][1], 42);
	BOOST_CHECK_EQUAL(m[1][0], 42);

	d[0][0] = 42;
	BOOST_CHECK_EQUAL(d[0][0], 42);
}

BOOST_AUTO_TEST_CASE(Iteration)
{
	// Graph g should be empty
	int count (0);
	for(SymmetricMatrix<int>::iterator it = g.begin(); it != g.end(); ++it) {
		count++;
	}
	BOOST_CHECK_EQUAL(count, 0);

	// Nothing was accessed. Check again.
	count = 0;
	for(auto it = g.begin(); it != g.end(); ++it) {
		count++;
	}
	BOOST_CHECK_EQUAL(count, 0);

	// Fill some values of Graph
	int k (0);
	for (int i = 0; i <= MATRIX_SIZE / 2; ++i)
	{
		for (int j = 0; j <= i; ++j)
		{
			g[i][j] = ++k;
		}
	}

	// Check counts
	BOOST_CHECK_EQUAL(g.count(), k);

	// Check iterators
	k = 0;
	for(auto it : g) {
		BOOST_CHECK_EQUAL(it, ++k);
	}

	k = 0;
	for(auto it = g.begin(); it != g.end(); ++it) {
		BOOST_CHECK_EQUAL(*it, ++k);
	}

	// Check const ref iterators
	const SymmetricMatrix<int>& refg = g;

	k = 0;
	for(auto it : refg) {
		BOOST_CHECK_EQUAL(it, ++k);
	}

	k = 0;
	for(auto it = refg.begin(); it != refg.end(); ++it) {
		BOOST_CHECK_EQUAL(*it, ++k);
	}
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
	bool catched(false);

	try {
		d[MATRIX_SIZE][MATRIX_SIZE] = 42;
	} catch(symmetric_matrix_exception& e) {
		catched = true;
	}
	BOOST_CHECK(catched);

	catched = false;
	try {
		m[0][0] = 42;
	} catch(symmetric_matrix_exception& e) {
		catched = true;
	}
	BOOST_CHECK(catched);
}

BOOST_AUTO_TEST_SUITE_END()
