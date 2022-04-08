#define TEST_NAME "Consensus"

#include "stdafx.h"

#include <consensus.hpp>

//#define DB_PATH DATAPATH "test_database.sqlite" // Made To Measure(TM)
#define CONSENSUS_PATH DATAPATH "../../Release/data/consensuses-2014-08/04/2014-08-04-05-00-00-consensus"
#define DB_PATH DATAPATH DATAPATH "../../../mator-db/server-descriptors-2014-08.db"
#define VALID_CIRCUITS_PATH DATAPATH "../../Release/data/viaAllPairs.csv"

//#define CONSENSUS_PATH DATAPATH "test_consensus.txt" // Made To Measure(TM)
#define CONSENSUS_DATE "2014-10-21 02:00:00"

//config.consensusFile = DATAPATH "../../consensuses-2014-08/04/2014-08-04-05-00-00-consensus";
//config.databaseFile = DATAPATH "../../../mator-db/server-descriptors-2014-08.db"; // database given

struct ConsensusFixture
{
	Consensus c;

	ConsensusFixture() : c(CONSENSUS_PATH, DB_PATH, VALID_CIRCUITS_PATH, false) {} // false: don't use vias
};

BOOST_FIXTURE_TEST_SUITE(ConsensusSuite, ConsensusFixture)

BOOST_AUTO_TEST_CASE(Consensus_Initialization)
{
	// Should just work
	// BOOST_REQUIRE_NO_THROW(Consensus(CONSENSUS_PATH, DB_PATH));

	// TODO: add tests for exceptions
}

BOOST_AUTO_TEST_CASE(Consensus_Relays)
{
	// There should be 5 relays in consensus
	//BOOST_CHECK_EQUAL(c.getSize(), 5);

	unsigned int num_US = 0;
	unsigned int num_UNKNOWN = 0;

	for (unsigned int i = 0; i < c.getRelays().size(); i++)
	{
		std::cout << i << " CC= " << c.getRelay(i).getCountry() << std::endl;
		std::cout << i << " long= " << c.getRelay(i).getLongitude() << std::endl;
		std::cout << i << " lat= " << c.getRelay(i).getLatitude() << std::endl;
		if (c.getRelay(i).getCountry() == "US")
			num_US += 1;

	}

	BOOST_CHECK(num_US > 500);

}

BOOST_AUTO_TEST_SUITE_END()