#define TEST_NAME "DBConnection"

#include "stdafx.h"

#include <db_connection.hpp>

#define DB_PATH DATAPATH "test_database.sqlite" // Made To Measure(TM)
#define CONSENSUS_DATE "2014-10-21 02:00:00"

bool isDBOpenException(db_exception const& ex) { return ex.why() == db_exception::reason::DB_OPEN; }

struct TestConnection
{
	DBConnection db;
	std::vector<std::string> fingerprints;

	TestConnection() : db(DB_PATH, CONSENSUS_DATE), fingerprints(15)
	{
		fingerprints = {
			"30427B9A4A5A98367829BF19AD1DCADD7E0F541C",
			"2A6E08AE74372282F80B56CAB0B350E03B13DA7F",
			"1AFD9C8FF9AB802AEA89487AE8DF36EC736A4EEE",
			"BFEEBAC4BC06E0CC12F600E5B77A1B08A687CCB3",
			"15D5A0C2695603C8EFB49AE012D271034AAAD3F8",
			"A05C6749F2338F7ABBC8169E6D4EA37A2EE95E37",
			"993E5DD06573565D0112F4CC77D9B4CD016AE0FB",
			"964CF449DD79CC6AFFAE89AF11E221E72045DF75",
			"CF01EB12EE0D8EF198F55AB530C6783E8E6BBB8B",
			"81D8BDAEB86E16AAEC2EB4A395A791E43A73C351",
			"92DDD496C7A1AC758FC12FD2D9931AC6560400F0",
			"10FDA609624925659622F89107C8494B6033A7FA",
			"F985EAB7BC0C7C2BB0872614C98738F111FB4F45",
			"112B4C7FAEA2D3DA1C0483E09144A8A59B610BCB",
			"C0ADF883382BD423D5678082A7971714FFFA314F"
		};
	}

	bool TestAreRelated(const std::string& nodeA, const std::string& nodeB, const QueryResult& families)
	{
		for (auto family : families) {
			if ((family[0] == nodeA && family[1] == nodeB) || (family[1] == nodeA && family[0] == nodeB)) {
				return true;
			}
		}

		return false;
	}
};

BOOST_FIXTURE_TEST_SUITE(DBConnectionSuite, TestConnection)

BOOST_AUTO_TEST_CASE(DBConnection_Initialization)
{
	// Should connect
	BOOST_REQUIRE_NO_THROW(DBConnection(DB_PATH, CONSENSUS_DATE));

	// TODO: add tests for mator-db db creation
}

BOOST_AUTO_TEST_CASE(ReadRelaysInfo)
{
	QueryResult relays;
	relays = db.readRelaysInfo(fingerprints.begin(), fingerprints.end());

	BOOST_REQUIRE_EQUAL(relays.size(), 13);

	Row get = relays[0];
	Row expected = {
		"30427B9A4A5A98367829BF19AD1DCADD7E0F541C",
		"13631488",
		"Linux",
		"reject *:*\n",
		"US",
		"36.1554",
		"-95.9939",
		"54290",
		"HOSTWINDS - Hostwinds LLC.,US"
	};

	for (int i = 0; i < expected.size(); ++i)
	{
		BOOST_REQUIRE_EQUAL(expected[i], get[i]);
	}
}

BOOST_AUTO_TEST_CASE(ReadFamilies)
{
	QueryResult families;
	families = db.readFamilies();

	std::cout << "Families size: " << families.size() << std::endl;

	BOOST_REQUIRE_EQUAL(families.size(), 4);

	BOOST_REQUIRE_EQUAL(TestAreRelated("30427B9A4A5A98367829BF19AD1DCADD7E0F541C", "2A6E08AE74372282F80B56CAB0B350E03B13DA7F", families), true);
	BOOST_REQUIRE_EQUAL(TestAreRelated("30427B9A4A5A98367829BF19AD1DCADD7E0F541C", "1AFD9C8FF9AB802AEA89487AE8DF36EC736A4EEE", families), true);
	BOOST_REQUIRE_EQUAL(TestAreRelated("30427B9A4A5A98367829BF19AD1DCADD7E0F541C", "BFEEBAC4BC06E0CC12F600E5B77A1B08A687CCB3", families), true);
	BOOST_REQUIRE_EQUAL(TestAreRelated("CF01EB12EE0D8EF198F55AB530C6783E8E6BBB8B", "81D8BDAEB86E16AAEC2EB4A395A791E43A73C351", families), true);
	
	BOOST_REQUIRE_EQUAL(TestAreRelated("993E5DD06573565D0112F4CC77D9B4CD016AE0FB", "81D8BDAEB86E16AAEC2EB4A395A791E43A73C351", families), false);
}

BOOST_AUTO_TEST_SUITE_END()
