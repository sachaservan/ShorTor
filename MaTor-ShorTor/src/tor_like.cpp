#include "tor_like.hpp"

#include <algorithm>

probability_t TorLike::exitProb(size_t exit) const
{
	return exitWeights[exit] * exitSumInv; // multiplying precomputed inverse instead of division
}

probability_t TorLike::entryProb(size_t entry, size_t exit) const
{
	if(entryWeights[entry] > 0 && entryExitAllowed(entry, exit))
		return entryWeights[entry] * entrySumRelatedInv[exit];
	return 0;
}

probability_t TorLike::middleProb(size_t middle, size_t entry, size_t exit) const
{
	if (consensus.useVias() && middlePossibleBecauseVia[middle])
		return middleWeights[middle] * middleSumRelatedInv[entry][exit];

	if(middleWeights[middle] > 0 && middleEntryExitAllowed(middle, entry, exit))
		return middleWeights[middle] * middleSumRelatedInv[entry][exit];
	
	return 0;
}

weight_t TorLike::getExitWeight(const Relay& relay) const
{
	int index = relay.position();
	if(!exitPossible[index])
		return 0;
	weight_t bandwidth = (weight_t) relay.getBandwidth();
	weight_t modifier = consensus.getWeightModifier(RelayRole::EXIT_ROLE, relay.getFlags());	
	return bandwidth * modifier;
}

weight_t TorLike::getEntryWeight(const Relay& relay) const
{
	int index = relay.position();
	if(!entryPossible[index])
		return 0;
	weight_t bandwidth = (weight_t) relay.getBandwidth();
	weight_t modifier = consensus.getWeightModifier(RelayRole::ENTRY_ROLE, relay.getFlags());	
	return bandwidth * modifier;
}

weight_t TorLike::getMiddleWeight(const Relay& relay) const
{
	int index = relay.position();
	if(!middlePossible[index])
		return 0;
	weight_t bandwidth = (weight_t) relay.getBandwidth();
	weight_t modifier = consensus.getWeightModifier(RelayRole::MIDDLE_ROLE, relay.getFlags());	
	return bandwidth * modifier;
}

void TorLike::assignRelatedWeightChunk(size_t start, size_t stop, weight_t entryWeightSum, weight_t middleWeightSum, 
	std::vector<std::set<size_t>> exitEntryRelatives, std::vector<std::set<size_t>> exitMiddleRelatives, std::vector<std::set<size_t>> entryMiddleRelatives)
{
	size_t size = consensus.getSize();
	for(size_t ex = start; ex < stop; ++ex) // for all relays in the interval
	{
		weight_t relatedEntryWeight = 0;
		weight_t relatedMiddleWeight = 0;
		
		// sum up exit-entry-related relays entry weights
		for(size_t relative : exitEntryRelatives[ex])
			relatedEntryWeight += entryWeights[relative];
		
		entrySumRelatedInv[ex] = (weight_t)1 / (entryWeightSum - relatedEntryWeight);
		
		// computing middle related weights
		for(size_t en = ex + 1; en < size; ++en) // ex != en and edgemap properties
		{
			relatedMiddleWeight = 0;
			
			// extract two relay's relatives without duplicating entries in case of common relatives
			std::vector<size_t> pairRelatives(exitMiddleRelatives[ex].size() + entryMiddleRelatives[en].size());
			auto end = std::set_union(exitMiddleRelatives[ex].begin(), exitMiddleRelatives[ex].end(),
				entryMiddleRelatives[en].begin(), entryMiddleRelatives[en].end(), pairRelatives.begin());
			pairRelatives.resize(end - pairRelatives.begin());
			for(size_t r : pairRelatives)
				relatedMiddleWeight += middleWeights[r];
			
			middleSumRelatedInv[ex][en] = (weight_t)1 / (middleWeightSum - relatedMiddleWeight);
		}
	}
}

void TorLike::assignRelatedWeight(weight_t entryWeightSum, weight_t middleWeightSum)
{
	size_t size = consensus.getSize();
	std::vector<std::set<size_t>> exitEntryRelatives(size);
	std::vector<std::set<size_t>> exitMiddleRelatives(size);
	std::vector<std::set<size_t>> entryMiddleRelatives(size);
	
	for(size_t i = 0; i < size; ++i)
		for(size_t j = 0; j < size; ++j)
		{
			if(entryWeights[j] > 0) // add relays with non-zero entry weight
				if(relations->exitEntryRelated(i, j))
					exitEntryRelatives[i].insert(j);
			if(middleWeights[j] > 0) // add relays with non-zero middle weight
			{
				if(relations->exitMiddleRelated(i, j))
					exitMiddleRelatives[i].insert(j);
				if(relations->entryMiddleRelated(i, j))
					entryMiddleRelatives[i].insert(j);
			}
		}
	
	WorkManager workManager;
	size_t chunkSize = (size / workManager.getHardwareConcurrency()) / 2 + 1;
	for(size_t i = 0; i < size; i += chunkSize)
	{
		workManager.addTask([&,i]() { this->assignRelatedWeightChunk(i, ((i + chunkSize < size) ? (i + chunkSize) : size),
			entryWeightSum, middleWeightSum, exitEntryRelatives, exitMiddleRelatives, entryMiddleRelatives); });
	}
	workManager.startAndJoinAll();
}
