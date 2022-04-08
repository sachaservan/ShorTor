#define TEST_NAME "PCF_PARSER"

#include "stdafx.h"

#include <pcf_parser.hpp>
#include <relay.hpp>
#include <pcf.hpp>

BOOST_AUTO_TEST_SUITE( Lexing )

BOOST_AUTO_TEST_CASE( Keywords )
{
    for (std::string s : pcfparser::keywords)
    {
		int prefix = pcfparser::keywordPrefix(s);
		BOOST_CHECK_GE(prefix, 0);
        BOOST_CHECK_EQUAL(pcfparser::keywords[prefix], s);
    }
}

BOOST_AUTO_TEST_SUITE_END()

// TODO: Add more tests
BOOST_AUTO_TEST_SUITE( Parsing )

BOOST_AUTO_TEST_CASE(Parse_Simple_PCF) {
	Relay r = Relay(0, "name", "fp", "2015-01-01", "1.2.3.4", true, true, false, true, true, true, true, 10000, "v1.0b");
	shared_ptr<ProgrammableCostFunction> pcf;

	pcf = make_shared<ProgrammableCostFunction>("ANY ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 4);
	
	pcf = make_shared<ProgrammableCostFunction>("NOT ANY ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 1);
	
	pcf = make_shared<ProgrammableCostFunction>("NAME == \'name\' ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 4);
	pcf = make_shared<ProgrammableCostFunction>("NAME == \'name2\' ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 1);
}

BOOST_AUTO_TEST_CASE(Parse_ADD_PCF) {
	Relay r = Relay(0, "name", "fp", "2015-01-01", "1.2.3.4", true, true, false, true, true, true, true, 10000, "v1.0b");
	shared_ptr<ProgrammableCostFunction> pcf;

	pcf = make_shared<ProgrammableCostFunction>("ANY ? ADD 1");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 2);

	pcf = make_shared<ProgrammableCostFunction>("NOT ANY ? ADD 1");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 1);

	pcf = make_shared<ProgrammableCostFunction>("NAME == \'name\' ? ADD 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 5);
}

BOOST_AUTO_TEST_CASE(Parse_Bandwidth_PCF) {
	Relay r = Relay(0, "name", "fp", "2015-01-01", "1.2.3.4", true, true, false, true, true, true, true, 10000, "v1.0b");
	r.setAveragedBandwidth(21000);
	shared_ptr<ProgrammableCostFunction> pcf;

	pcf = make_shared<ProgrammableCostFunction>("ANY ? SET BANDWIDTH");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 10000);
	pcf = make_shared<ProgrammableCostFunction>("ANY ? SET AVGBANDWIDTH");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 21000);
	pcf = make_shared<ProgrammableCostFunction>("ANY ? ADD BANDWIDTH");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 10001);
	pcf = make_shared<ProgrammableCostFunction>("ANY ? SET 3; ANY ? MUL BANDWIDTH");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 30000);
	pcf = make_shared<ProgrammableCostFunction>("ANY ? SET BANDWIDTH; ANY ? ADD AVGBANDWIDTH; ANY ? MUL 0.5");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 15500);
}

BOOST_AUTO_TEST_CASE(Parse_BandwidthCond_PCF) {
	Relay r = Relay(0, "name", "fp", "2015-01-01", "1.2.3.4", true, true, false, true, true, true, true, 10000, "v1.0b");
	r.setAveragedBandwidth(21000);
	shared_ptr<ProgrammableCostFunction> pcf;

	pcf = make_shared<ProgrammableCostFunction>("BANDWIDTH < 12000 ? SET 5");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 5);
	pcf = make_shared<ProgrammableCostFunction>("BANDWIDTH > 12000 ? SET 5");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 1);

	pcf = make_shared<ProgrammableCostFunction>("AVGBANDWIDTH < 12000 ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 1);
	pcf = make_shared<ProgrammableCostFunction>("AVGBANDWIDTH > 12000 ? SET 4");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 4);
}

BOOST_AUTO_TEST_CASE(Parse_Cond_PCF) {
	Relay r = Relay(0, "name", "fp", "2015-01-01", "1.2.3.4", true, true, false, true, true, true, true, 10000, "v1.0b");
	r.setAveragedBandwidth(21000);
	shared_ptr<ProgrammableCostFunction> pcf;

	pcf = make_shared<ProgrammableCostFunction>("ANY AND ANY ? SET 5");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 5);
	pcf = make_shared<ProgrammableCostFunction>("ANY OR NOT ANY ? SET 6");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 6);
	pcf = make_shared<ProgrammableCostFunction>("NOT ANY OR NOT BANDWIDTH < 5000 ? SET 7");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 7);

	// Order testing
	pcf = make_shared<ProgrammableCostFunction>("NOT ANY AND ANY OR ANY ? SET 8");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 8);
	pcf = make_shared<ProgrammableCostFunction>("ANY OR (NOT ANY) AND ANY ? SET 9");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 9);
	pcf = make_shared<ProgrammableCostFunction>("(ANY OR (NOT ANY)) AND ANY ? SET 10");
	BOOST_CHECK_EQUAL(pcf->apply(r, 1), 10);
}


BOOST_AUTO_TEST_SUITE_END()

//BOOST_AUTO_TEST_SUITE( Predicate )
//BOOST_AUTO_TEST_SUITE_END()
//
//BOOST_AUTO_TEST_SUITE( Effect )
//BOOST_AUTO_TEST_SUITE_END()
//
//BOOST_AUTO_TEST_SUITE( toJSON )
//BOOST_AUTO_TEST_SUITE_END()
//
//BOOST_AUTO_TEST_SUITE( fromJSON )
//BOOST_AUTO_TEST_SUITE_END()
