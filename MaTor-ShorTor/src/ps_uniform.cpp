#include "ps_uniform.hpp"

#include <algorithm>
#include <math.h>   

PSUniform::PSUniform(std::shared_ptr<PSUniformSpec> pathSelectionSpec,
	std::shared_ptr<SenderSpec> senderSpec, 
	std::shared_ptr<RecipientSpec> recipientSpec,
	std::shared_ptr<RelationshipManager> relationshipManager,
	const Consensus& consensus) : TorLike(senderSpec, recipientSpec, pathSelectionSpec, relationshipManager, consensus)
{
	initMeasure(start, stop);
	
	clogsn("Computing relay's roles possibilities..."); 
	makeMeasure(start);
	computeRelaysPossibilities(pathSelectionSpec);
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	size_t size = consensus.getSize();
	
	weight_t exitWeightSum = 0;
	weight_t entryWeightSum = 0;
	weight_t middleWeightSum = 0;
	
	clogsn("Assigning single weights...");
	makeMeasure(start);
	for(size_t i = 0; i < size; ++i)
	{
		const Relay& relay = consensus.getRelay(i);
		if (exitPossible[i])
			exitWeights[i] = 1;
		else
			exitWeights[i] = 0;
		if (entryPossible[i])
			entryWeights[i] = 1;
		else
			entryWeights[i] = 0;
		if (middlePossible[i])
			middleWeights[i] = 1;
		else
			middleWeights[i] = 0;

		exitWeightSum += exitWeights[i];
		entryWeightSum += entryWeights[i];
		middleWeightSum += middleWeights[i];
	}
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
