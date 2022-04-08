#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>

#include "mator.hpp"
#include "utils.hpp"
#include "consensus.hpp"
#include "asmap.hpp"
#include "pcf.hpp"
#include "types/const_vector.hpp"

using namespace std;


MATor::MATor(
	std::shared_ptr<SenderSpec> senderSpec1,
	std::shared_ptr<SenderSpec> senderSpec2,
	std::shared_ptr<RecipientSpec> recipientSpec1,
	std::shared_ptr<RecipientSpec> recipientSpec2,
	std::shared_ptr<PathSelectionSpec> pathSelectionSpec1,
	std::shared_ptr<PathSelectionSpec> pathSelectionSpec2,
	Config config)
	: senderSpec1(senderSpec1), senderSpec2(senderSpec2),
	recipientSpec1(recipientSpec1), recipientSpec2(recipientSpec2),
	pathSelectionSpec1(pathSelectionSpec1), pathSelectionSpec2(pathSelectionSpec2)
{
	epsilon = config.epsilon;
	consensus = make_shared<Consensus>(config.consensusFile, config.databaseFile, config.viaAllPairsFile, config.useVias);
	clogsn("Recipientspecs: #ports for R1: " << recipientSpec1->ports.size() << ", R2: " << recipientSpec2->ports.size());
	clogsn(recipientSpec1->address.address);
}

MATor::MATor(
	std::shared_ptr<SenderSpec> senderSpec1,
	std::shared_ptr<SenderSpec> senderSpec2,
	std::shared_ptr<RecipientSpec> recipientSpec1,
	std::shared_ptr<RecipientSpec> recipientSpec2,
	std::shared_ptr<PathSelectionSpec> pathSelectionSpec1,
	std::shared_ptr<PathSelectionSpec> pathSelectionSpec2,
	std::shared_ptr<Consensus> consensus, bool fast)
	: senderSpec1(senderSpec1), senderSpec2(senderSpec2),
	recipientSpec1(recipientSpec1), recipientSpec2(recipientSpec2),
	pathSelectionSpec1(pathSelectionSpec1), pathSelectionSpec2(pathSelectionSpec2),
	consensus(consensus) {
	clogsn("Recipientspecs: #ports for R1: " << recipientSpec1->ports.size() << ", R2: " << recipientSpec2->ports.size());
	clogsn("R0 addr: " << recipientSpec1->address.address << ", R1 addr: " << recipientSpec2->address.address);
}

MATor::MATor(const std::string& configJSONfile) {
	NOT_IMPLEMENTED;
}


MATor::~MATor() {}


void MATor::prepareCalculation() {
	commitSpecification();
	if (gwca == nullptr) {
		std::cout << "\n Preparing calculation..." << std::endl;
		gwca = unique_ptr<GenericWorstCaseAnonymity>(new GenericWorstCaseAnonymity(*consensus, *pathSelectionA1, *pathSelectionA2, *pathSelectionB1, *pathSelectionB2, epsilon));
		std::cout << "done preparing calculation." << std::endl;
	}
	if (!adversary.getCostmap().isInitialized(consensus->getRelays().size())) {
		clogn(LOG_STANDARD, "You did not commit your PCF functions!");
		adversary.getCostmap().commit(consensus->getRelays());
	}
}

void MATor::preparePreciseCalculation(
	std::vector<size_t> compromisedNodes) {
	size_t size = consensus->getRelays().size();
	const_vector<const_vector<bool>> observedNodes(size,size,false);
	const_vector<bool> observedSenderA (size,false);
	const_vector<bool> observedSenderB (size, false);
	const_vector<bool> observedRecipient1 (size, false);
	const_vector<bool> observedRecipient2 (size, false);

	// Computes all pairs of nodes and sets pair to `true' if at least 
	// one node is compromised 
	for (size_t k = 0; k < compromisedNodes.size(); k++)
	{
		size_t i = compromisedNodes[k];
		for (size_t j = 0; j < size; j++)
		{
			observedNodes[i][j] = true;
			observedNodes[j][i] = true;
		}
		observedSenderA[i] = true;
		observedSenderB[i] = true;
		observedRecipient1[i] = true;
		observedRecipient2[i] = true;
	}

	std::cout << "Preparing precise calculation..." << std::endl;
	gpra = unique_ptr<GenericPreciseAnonymity>(new GenericPreciseAnonymity(*consensus, *pathSelectionA1, *pathSelectionA2, *pathSelectionB1, *pathSelectionB2,
			observedNodes, observedSenderA, observedSenderB, observedRecipient1, observedRecipient2, epsilon));
	std::cout << "done preparing precise calculation." << std::endl;
}

bool vector_in_set(std::vector<std::string>& myvec, std::set<std::string>& myset)
{
	//myassert(myvec.size()==3 && myvec[0]=="AS0" && myvec[1]=="AS1" && myvec[2]=="AS2");
	//myassert(myset.size()==3);
	for(size_t i=0; i < myvec.size(); i++)
	{
		if(myset.find(myvec[i]) != myset.end())
			return true;
	}
	return false;
}

void MATor::prepareNetworkCalculation(std::vector<std::string> compromisedASes) {
	std::ofstream debugfile;
	debugfile.open("debug_mator.txt");

	debugfile << "Creating AS map.." << std::endl;
	asmap = unique_ptr<ASMap>(new ASMap(*consensus, senderSpec1,senderSpec2, recipientSpec1, recipientSpec2, "../data/networkfile.txt"));

	debugfile << "done." << std::endl;


	if(!asmap->endpoint_exists((std::string)senderSpec1->address))
		debugfile << "Missing entry for Sender 1" << std::endl;
	if(!asmap->endpoint_exists((std::string)senderSpec2->address))
		debugfile << "Missing entry for Sender 2" << std::endl;
	if(!asmap->endpoint_exists((std::string)recipientSpec1->address))
		debugfile << "Missing entry for Recipient 1" << std::endl;
	if(!asmap->endpoint_exists((std::string)recipientSpec2->address))
		debugfile << "Missing entry for Recipient 2" << std::endl;

	size_t size = consensus->getRelays().size();
	const_vector<const_vector<bool>> observedNodes(size,size,false);
	const_vector<bool> observedSenderA (size,false);
	const_vector<bool> observedSenderB (size, false);
	const_vector<bool> observedRecipient1 (size, false);
	const_vector<bool> observedRecipient2 (size, false);

	for (size_t i = 0; i < size; i++)
	{
		for (size_t j = 0; j < size; j++)
		{
			//myassert(i==j || vector_in_set(compromisedASes,asmap->aspath_nodes(i,j)));
			if(vector_in_set(compromisedASes,asmap->aspath_nodes(i,j)))
			{
				observedNodes[i][j] = true;
				observedNodes[j][i] = true;
			}else{
				if(i != j)
				{
					debugfile << "nodes " << i << " and " << j << " are not observed!" << std::endl;
					BOOST_FOREACH (std::string t, asmap->aspath_nodes(i,j))
						debugfile << "<" << t << ">" << std::flush;
					debugfile << std::endl;
				}
			}
		}
		//myassert(vector_in_set(compromisedASes,asmap->aspath_endpoint(i,(std::string)senderSpec1->address)));

		if(vector_in_set(compromisedASes,asmap->aspath_endpoint(i,(std::string)senderSpec1->address)))
			observedSenderA[i] = true;
		if(vector_in_set(compromisedASes,asmap->aspath_endpoint(i,(std::string)senderSpec2->address)))
			observedSenderB[i] = true;
		if(vector_in_set(compromisedASes,asmap->aspath_endpoint(i,(std::string)recipientSpec1->address)))
			observedRecipient1[i] = true;
		if(vector_in_set(compromisedASes,asmap->aspath_endpoint(i,(std::string)recipientSpec2->address)))
			observedRecipient2[i] = true;

	}
	gpra = unique_ptr<GenericPreciseAnonymity>(new GenericPreciseAnonymity(*consensus, *pathSelectionA1, *pathSelectionA2, *pathSelectionB1, *pathSelectionB2,
			observedNodes, observedSenderA, observedSenderB, observedRecipient1, observedRecipient2, epsilon));


}

double MATor::getSenderAnonymity() {
	prepareCalculation();
	return gwca->senderAnonymity(adversary);
}

double MATor::getRecipientAnonymity() {
	prepareCalculation();
	return gwca->recipientAnonymity(adversary);
}

double MATor::getRelationshipAnonymity() {
	prepareCalculation();
	return gwca->relationshipAnonymity(adversary);
}



void MATor::getGreedyListForSenderAnonymity(std::vector<size_t>& output) {
	prepareCalculation();
	return gwca->greedySenderAnonymity(output,adversary);
}

void MATor::getGreedyListForRecipientAnonymity(std::vector<size_t>& output) {
	prepareCalculation();
	return gwca->greedyRecipientAnonymity(output,adversary);
}

void MATor::getGreedyListForRelationshipAnonymity(std::vector<size_t>& output) {
	prepareCalculation();
	return gwca->greedyRelationshipAnonymity(output,adversary);
}


double MATor::getPreciseSenderAnonymity() {
	if (gpra == nullptr) {
		clogsn("You have to first call 'preparePreciseCalculation' with a description of a (fixed) adversary");
		return -1;
	}
	return gpra->senderAnonymity();
}

double MATor::getPreciseRecipientAnonymity() {
	if (gpra == nullptr) {
		clogsn("You have to first call 'preparePreciseCalculation' with a description of a (fixed) adversary");
		return -1;
	}
	return gpra->recipientAnonymity();
}

double MATor::getPreciseRelationshipAnonymity() {
	if (gpra == nullptr) {
		clogsn("You have to first call 'preparePreciseCalculation' with a description of a (fixed) adversary");
		return -1;
	}
	return gpra->relationshipAnonymity();
}

double MATor::getNetworkSenderAnonymity() {
	if (gpra == nullptr || asmap == nullptr) {
		clogsn("You have to first call 'prepareNetworkCalculation' with a description of a (fixed) network adversary");
		return -1;
	}
	return gpra->senderAnonymity();
}

double MATor::getNetworkRecipientAnonymity() {
	if (gpra == nullptr || asmap == nullptr) {
		clogsn("You have to first call 'prepareNetworkCalculation' with a description of a (fixed) network adversary");
		return -1;
	}
	return gpra->recipientAnonymity();
}

double MATor::getNetworkRelationshipAnonymity() {
	if (gpra == nullptr || asmap == nullptr) {
		clogsn("You have to first call 'prepareNetworkCalculation' with a description of a (fixed) network adversary");
		return -1;
	}
	return gpra->relationshipAnonymity();
}


double MATor::lowerBoundSenderAnonymity() {
	prepareCalculation();
	std::vector<size_t> greedylist;
	getGreedyListForSenderAnonymity(greedylist);
	preparePreciseCalculation(greedylist);
	return gpra->senderAnonymity();
}

double MATor::lowerBoundRecipientAnonymity() {
	prepareCalculation();
	std::vector<size_t> greedylist;
	getGreedyListForRecipientAnonymity(greedylist);
	preparePreciseCalculation(greedylist);
	return gpra->recipientAnonymity();
}

double MATor::lowerBoundRelationshipAnonymity() {
	prepareCalculation();
	std::vector<size_t> greedylist;
	getGreedyListForRelationshipAnonymity(greedylist);
	preparePreciseCalculation(greedylist);
	return gpra->relationshipAnonymity();
}


void MATor::addProgrammableCostFunction(std::shared_ptr<ProgrammableCostFunction> pcf) {
	adversary.getCostmap().addPCF(pcf);
}

void MATor::commitPCFs() {
	adversary.getCostmap().commit(consensus->getRelays());
}

void MATor::resetPCFs() {
	adversary.getCostmap().reset();
}

void MATor::setPCF(std::string& pcf) {
	adversary.getCostmap().reset();
	if (!pcf.empty())
		adversary.getCostmap().addPCF(shared_ptr<ProgrammableCostFunction>(new ProgrammableCostFunction(pcf)));
	adversary.getCostmap().commit(consensus->getRelays());
}

void MATor::setPCFCallback(const std::function<double(const Relay&, double)>& pcf) {
	adversary.getCostmap().reset();
	adversary.getCostmap().addPCF(shared_ptr<ProgrammableCostFunction>(new ProgrammableCostFunction(0, pcf)));
	adversary.getCostmap().commit(consensus->getRelays());
}

void MATor::setAdversaryBudget(double budget) {
	adversary.setBudget(budget);
}

void MATor::commitSpecification() {
	if (!computeFlags) return;
	if (computeFlags & 1) {
		// Path selection computed from specification for sender A, path selection 1 and recipient 1
		pathSelectionA1 = Scenario::makePathSelection(pathSelectionSpec1, senderSpec1, recipientSpec1, *consensus);
	}
	if (computeFlags & 2) {
		// Path selection computed from specification for sender A, path selection 1 and recipient 2
		pathSelectionA2 = Scenario::makePathSelection(pathSelectionSpec1, senderSpec1, recipientSpec2, *consensus);
	}
	if (computeFlags & 4) {
		// Path selection computed from specification for sender B, path selection 2 and recipient 1
		pathSelectionB1 = Scenario::makePathSelection(pathSelectionSpec2, senderSpec2, recipientSpec1, *consensus);
	}
	if (computeFlags & 8) {
		// Path selection computed from specification for sender B, path selection 2 and recipient 2
		pathSelectionB2 = Scenario::makePathSelection(pathSelectionSpec2, senderSpec2, recipientSpec2, *consensus);
	}
	gwca = nullptr;
	computeFlags = 0;
}
