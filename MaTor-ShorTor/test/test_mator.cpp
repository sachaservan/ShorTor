#define TEST_NAME "MATor"

#include "stdafx.h"
#include "mator.hpp"

BOOST_AUTO_TEST_CASE(MATor_DevTest) {
	// calculate some simple sender anonymity
	shared_ptr<SenderSpec> sender1 = make_shared<SenderSpec>(IP("144.118.66.83"), 39.9597, -75.1968);
	shared_ptr<SenderSpec> sender2 = make_shared<SenderSpec>(IP("129.79.78.192"), 39.174729, -86.507890);
	shared_ptr<RecipientSpec> recipient1 = make_shared<RecipientSpec>(IP("130.83.47.181"), 49.8719, 8.6484);
	shared_ptr<RecipientSpec> recipient2 = make_shared<RecipientSpec>(IP("134.58.64.12"), 50.8796, 4.7009);
	recipient1->ports.insert(443);
	recipient2->ports.insert(443);
	recipient2->ports.insert(1);
	shared_ptr<PathSelectionSpec> ps(new PSTorSpec());
	shared_ptr<PathSelectionSpec> psU(new PSUniformSpec());
	shared_ptr<PathSelectionSpec> psS(new PSSelektorSpec());
	shared_ptr<PathSelectionSpec> psL(new PSLASTorSpec());

	Config config;
	config.epsilon = 1;
	

	//config.consensusFile = DATAPATH "2014-10-04-05-00-00-consensus-filtered";
	config.consensusFile = DATAPATH "../../Release/data/consensuses-2014-08/04/2014-08-04-05-00-00-consensus";
	config.databaseFile = DATAPATH "../../../mator-db/server-descriptors-2014-08.db"; // database given
	config.viaAllPairsFile = DATAPATH "../../Release/data/viaAllPairs.csv";

	std::shared_ptr<Consensus> consensus = make_shared<Consensus>(config.consensusFile, config.databaseFile, config.viaAllPairsFile, config.useVias);
	std::shared_ptr<PathSelection> checkUniform = Scenario::makePathSelection(psU, sender1, recipient1, *consensus);

	std::cout << "Checking path selection algorithms UNIFORM and SELEKTOR" << endl;
	unsigned int exitnodes = 0;
	unsigned int guards = 0;
	unsigned int middles = 0;

	for (unsigned int i = 0; i < consensus->getSize(); i++)
	{
		if (consensus->getRelay(i).hasFlags(RelayFlag::VALID) && consensus->getRelay(i).hasFlags(RelayFlag::RUNNING))
			middles += 1;
		if (consensus->getRelay(i).hasFlags(RelayFlag::EXIT) && consensus->getRelay(i).supportsConnection(IP("129.132.0.0") ,443) && !(consensus->getRelay(i).hasFlags(RelayFlag::BAD_EXIT)))
			exitnodes += 1;
		if (consensus->getRelay(i).hasFlags(RelayFlag::GUARD))
			guards += 1;
	}
	std::cout << "There are " << exitnodes << " exit nodes" << endl;
	vector<bool> possibleflags = vector<bool>(12,true);
	vector<bool> beenTrue = vector<bool>(12, false);
	vector<bool> beenFalse = vector<bool>(12, false);
	for (unsigned int i = 0; i < consensus->getSize(); i++)
	{
		if (consensus->getRelay(i).hasFlags(RelayFlag::EXIT) && consensus->getRelay(i).supportsConnection(IP("129.132.0.0"), 443) && !(consensus->getRelay(i).hasFlags(RelayFlag::BAD_EXIT)))
		{
			BOOST_CHECK_EQUAL(checkUniform->exitProb(i), 1.0 / exitnodes);
		}
		else {
			BOOST_CHECK_EQUAL(checkUniform->exitProb(i), 0);
			if (checkUniform->exitProb(i) > 0)
				cout << consensus->getRelay(i).getFlags();
		}
		if (consensus->getRelay(i).hasFlags(RelayFlag::GUARD) && i != 0)
		{
			BOOST_CHECK_EQUAL(checkUniform->entryProb(i,0), 1.0 / guards);
		}
		else {
			BOOST_CHECK_EQUAL(checkUniform->entryProb(i,0), 0);
		}
		if ((i != 10) && (i != 11) && (consensus->getRelay(i).hasFlags(RelayFlag::VALID)) && (consensus->getRelay(i).hasFlags(RelayFlag::RUNNING)))
		{
			BOOST_CHECK_EQUAL(checkUniform->middleProb(i, 11, 10), 1.0 / (middles - 2));
		}
	}

	std::shared_ptr<PathSelection> checkSelektor = Scenario::makePathSelection(psS, sender1, recipient1, *consensus);

	for (unsigned int i = 0; i < consensus->getSize(); i++)
	{
		if (consensus->getRelay(i).hasFlags(RelayFlag::EXIT) && consensus->getRelay(i).supportsConnection(IP("129.132.0.0"), 443) && (consensus->getRelay(i).getCountry() == "US") && !(consensus->getRelay(i).hasFlags(RelayFlag::BAD_EXIT)))
		{
			BOOST_CHECK_GT(checkSelektor->exitProb(i), 0);
		}
		else {
			BOOST_CHECK_EQUAL(checkSelektor->exitProb(i), 0);
			if (checkSelektor->exitProb(i) > 0)
				cout << consensus->getRelay(i).getFlags();
		}
	}

	std::shared_ptr<PathSelection> checkLASTor = Scenario::makePathSelection(psL, sender1, recipient1, *consensus);

	cout << "Checking MATor" << endl;

	// initialize mator
	MATor mator(sender1, sender2, recipient1, recipient2, psU, ps, config);
	mator.commitSpecification();

	// Calculate SA
	mator.setAdversaryBudget(100); // 10-of-n
	mator.commitPCFs();
	double result = mator.getSenderAnonymity();
	cout << "SA Result: " << result << endl;

	/*
	cout << "Running precise MATor for SA (this may take a while)." << endl;
	//result = mator.lowerBoundSenderAnonymity();
	cout << "Precise SA Result: " << result << endl;
	*/
	result = mator.getRecipientAnonymity();
	cout << "RA Result: " << result << endl;
	/*
	cout << "Running precise MATor for RA (this may take a while)." << endl;
	//result = mator.lowerBoundRecipientAnonymity();
	cout << "Precise RA Result: " << result << endl;
	*/
	result = mator.getRelationshipAnonymity();
	cout << "REL Result: " << result << endl;
	/*
	cout << "Running precise MATor for REL (this may take a while)." << endl;
	//result = mator.lowerBoundRelationshipAnonymity();
	cout << "Precise REL Result: " << result << endl;
	*/






	BOOST_CHECK_GE(result, 0.0);
	BOOST_CHECK_LE(result, 1.0);
}
