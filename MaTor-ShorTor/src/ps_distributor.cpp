#include "ps_distributor.hpp"
#include "utils.hpp"
#include <math.h>   

#include <algorithm>

bool PSDistribuTor::compBWExit::operator() (size_t i, size_t j) const
{
	int bwI = (exitWeights[i] > 0) ? consensus.getRelay(i).getBandwidth() : 0;
	int bwJ = (exitWeights[j] > 0) ? consensus.getRelay(j).getBandwidth() : 0;
	return (bwI < bwJ);
}

bool PSDistribuTor::compBWEntry::operator() (size_t i, size_t j) const
{
	int bwI = (entryWeights[i] > 0) ? consensus.getRelay(i).getBandwidth() : 0;
	if(exitWeights[i] > 0)
		bwI = (bwI > maxExitBW) ? (bwI - maxExitBW) : 0;
	
	int bwJ = (entryWeights[j] > 0) ? consensus.getRelay(j).getBandwidth() : 0;
	if(exitWeights[j] > 0)
		bwJ = (bwJ > maxExitBW) ? (bwJ - maxExitBW) : 0;
	
	return (bwI < bwJ);
}

PSDistribuTor::PSDistribuTor(std::shared_ptr<PSDistribuTorSpec> pathSelectionSpec, 
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus &consensus) :
				TorLike(senderSpec, recipientSpec, pathSelectionSpec, relationshipManager, consensus),
				maxEntryWeight(0), maxExitWeight(0), modifier(consensus.getMaxModifier())
{
	if(pathSelectionSpec->bandwidthPerc < 0 || pathSelectionSpec->bandwidthPerc > 1)
		throw_exception(distributor_init_exception, distributor_init_exception::PERCENTAGE_OOR);
	
	initMeasure(start, stop);
	
	size_t size = consensus.getSize();
	
	clogsn("Preinitializing weights... ");
	makeMeasure(start);
	computeRelaysPossibilities(pathSelectionSpec, false);
	
	weight_t exitWeightSum = 0;
	weight_t entryWeightSum = 0;
	weight_t middleWeightSum = 0;
	
	for(size_t i = 0; i < size; ++i)
	{
		const Relay& relay = consensus.getRelay(i);
		exitWeights[i] = TorLike::getExitWeight(relay);
		entryWeights[i] = TorLike::getEntryWeight(relay);
		exitWeightSum += exitWeights[i];
		entryWeightSum += entryWeights[i];
	}
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Computing maximal exit bandwidth...");
	makeMeasure(start);
	
	// sorting by exit bandwidth
	compBWExit compEx(exitWeights, consensus);
	std::vector<size_t> indices(size);
	for(size_t i = 0; i < size; ++i)
		indices[i] = i;
			
	std::sort(indices.begin(), indices.end(), compEx);
	
	weight_t accumW = 0;
	
	weight_t goalExit = exitWeightSum * pathSelectionSpec->bandwidthPerc;
	clogvn("Exit goal: " << goalExit);
	
	for(size_t i = 0; i < size; ++i)
	{
		size_t relaysLeft = size - i;
		weight_t currentW = (exitWeights[indices[i]] > 0) ? ((weight_t)consensus.getRelay(indices[i]).getBandwidth() * modifier) : 0;
		
		if(accumW + relaysLeft * currentW < goalExit)
			accumW += currentW;
		else
		{
			maxExitWeight = (goalExit - accumW) / relaysLeft;
			if(maxExitWeight < exitWeights[indices[size - 1]])
			{
				clogvn("Limit found for exit weight.");
				break;
			}
		}
	}
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Computing maximal entry bandwidth...");
	makeMeasure(start);
	
	compBWEntry compEn(exitWeights, entryWeights, consensus, (maxExitWeight / modifier));
	std::sort(indices.begin(), indices.end(), compEn);
	
	accumW = 0;
	
	weight_t goalEntry = entryWeightSum * pathSelectionSpec->bandwidthPerc;
	clogvn("Entry goal: " << goalEntry);
	
	for(size_t i = 0; i < size; ++i)
	{
		size_t relaysLeft = size - i;
		weight_t currentW = (entryWeights[indices[i]] > 0) ? ((weight_t)consensus.getRelay(i).getBandwidth() * modifier) : 0;
		if(exitWeights[i] > 0)
			currentW = (currentW > maxExitWeight) ? (currentW - maxExitWeight) : 0;
		
		if(accumW + relaysLeft * currentW < goalEntry)
			accumW += currentW;
		else
		{
			maxEntryWeight = (goalEntry - accumW) / relaysLeft;
			weight_t testW = ((weight_t)consensus.getRelay(indices[size - 1]).getBandwidth() * modifier);
			
			if(exitWeights[indices[size - 1]] > 0)
				testW -= maxExitWeight;
			
			if(maxEntryWeight < testW)
			{
				clogvn("Limit found for entry weight.");
				break;
			}
		}
	}
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogvn("Maximal exit weight: " << maxExitWeight);
	clogvn("Maximal entry weight: " << maxEntryWeight);
	
	exitWeightSum = 0;
	entryWeightSum = 0;
	
	clogsn("Assigning distributed weights...");
	makeMeasure(start);
	for(size_t i = 0; i < size; ++i)
	{
		const Relay& relay = consensus.getRelay(i);
		exitWeights[i] = getExitWeight(relay);
		entryWeights[i] = getEntryWeight(relay);
		middleWeights[i] = getMiddleWeight(relay);
		
		exitWeightSum +=  exitWeights[i];
		entryWeightSum += entryWeights[i];
		middleWeightSum += middleWeights[i];
	}	
	if(entryWeightSum < 0)
		throw_exception(distributor_init_exception, distributor_init_exception::ZERO_ENTRY_SUM);
	exitSumInv = (weight_t) 1.0 / exitWeightSum;
	
	makeMeasure(stop);	
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Assigning additional relations constraints...");
	makeMeasure(start);
	relations->assignConstraints(entryPossible, exitPossible);
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Computing related weights...");
	makeMeasure(start);
	assignRelatedWeight(entryWeightSum, middleWeightSum);
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
}

weight_t PSDistribuTor::getExitWeight(const Relay& relay)const
{
	weight_t relayWeight = (weight_t)relay.getBandwidth() * modifier;
	return (!exitPossible[relay.position()]) ? 0 : ((relayWeight > maxExitWeight) ? maxExitWeight : relayWeight);
}

weight_t PSDistribuTor::getEntryWeight(const Relay& relay) const
{
	int index = relay.position();
	if(!entryPossible[index])
		return 0;
	
	weight_t relayWeight = (weight_t)relay.getBandwidth() * modifier;
	if(exitPossible[index])
		relayWeight = (relayWeight > maxExitWeight) ? relayWeight - maxExitWeight : 0;
	
	return (relayWeight < maxEntryWeight) ? relayWeight : maxEntryWeight;
}

weight_t PSDistribuTor::getMiddleWeight(const Relay& relay) const
{
	int index = relay.position();
	if(!middlePossible[index])
		return 0;
	
	weight_t relayWeight = ((weight_t)relay.getBandwidth() * modifier) - exitWeights[index] - entryWeights[index];
	return (relayWeight > 0) ? relayWeight : 0; 
}
