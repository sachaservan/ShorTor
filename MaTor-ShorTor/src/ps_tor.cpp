#include "ps_tor.hpp"

#include <algorithm>
#include <math.h>   

PSTor::PSTor(std::shared_ptr<PSTorSpec> pathSelectionSpec, 
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
		exitWeights[i] = getExitWeight(relay);
		entryWeights[i] = getEntryWeight(relay);
		middleWeights[i] = getMiddleWeight(relay);
		
		exitWeightSum +=  exitWeights[i];
		entryWeightSum += entryWeights[i];
		middleWeightSum += middleWeights[i];
	}



	// [ShorTor]: bias the weights of relays being selected as middle 
	// nodes based on the probability of the relay being selected as a via relay. 

	weight_t middleSumInv = (weight_t) 1.0 / middleWeightSum;
	weight_t entrySumInv = (weight_t) 1.0 / entryWeightSum;
	if (consensus.useVias()) {
		size_t size = consensus.getSize();

		exitSumInv = (weight_t) 1.0 / exitWeightSum;

		middlePossibleBecauseVia.resize(size);

		// Step 1: recompute weights based on whether or not each relay is ever 
		// used as a via for some pair
		for(size_t i = 0; i < size; ++i)
		{
			std::vector<std::tuple<size_t, size_t>> allPairs = consensus.getPairsForVia(i);

			// IMPORTANT: set middlePossible to true *before* getting the relay weight
			// because otherwise the weight is zero! 
			if (allPairs.size() > 0) {
				middlePossible[i] = true;
				middlePossibleBecauseVia[i] = true;
			} else {
				middlePossibleBecauseVia[i] = false;
			}

			// NOTE: have to recompute weight *after* setting middlePossible flag
			weight_t newWeight = getMiddleWeight(consensus.getRelay(i)); 
			middleWeightSum += (newWeight - middleWeights[i]);
			middleWeights[i] = newWeight; 
		}

		weight_t middleSumInv = (weight_t) 1.0 / middleWeightSum;
		weight_t entrySumInv = (weight_t) 1.0 / entryWeightSum;

		std::cout << "[ShorTor]: starting to update middle weights based on via probability... " << std::endl;

		std::vector<weight_t> newMiddleWeights = std::vector<weight_t>(size);
		weight_t newMiddleWeightSum = 0;
		for(size_t i = 0; i < size; ++i)
		{
			std::vector<std::tuple<size_t, size_t>> allPairs = consensus.getPairsForVia(i);

			probability_t middleProb = (probability_t) middleWeights[i] * middleSumInv;
			newMiddleWeights[i] = middleWeights[i];
			
			if (middleProb == 0)
				continue; // avoid division by zero 
			
			probability_t newMiddleProb = middleProb; // to be computed in the loop below
			for(size_t j = 0; j < allPairs.size(); ++j) {
				size_t r1 = std::get<0>(allPairs[j]);
				size_t r2 = std::get<1>(allPairs[j]);
			
				probability_t entryProbR1 = (probability_t) entryWeights[r1] * entrySumInv;
				probability_t entryProbR2 = (probability_t) entryWeights[r2] * entrySumInv;
				probability_t exitProbR1 = (probability_t) exitWeights[r1] * exitSumInv;
				probability_t exitProbR2 = (probability_t) exitWeights[r2] * exitSumInv;
				probability_t middleProbR1 = (probability_t) middleWeights[r1] * middleSumInv;
				probability_t middleProbR2 = (probability_t) middleWeights[r2] * middleSumInv;

				probability_t gm = (middleProbR2 * entryProbR1) + (middleProbR1 * entryProbR2); 
				probability_t mx = (middleProbR2 * exitProbR1) + (middleProbR1 * exitProbR2);

				newMiddleProb += (gm + mx);
			}

			// change in probability ==> how much to increase this relay's weight
			probability_t change = newMiddleProb / middleProb;

			// increase middle weight by probability change
			newMiddleWeights[i] *= change;
			newMiddleWeightSum += newMiddleWeights[i];
		}

		std::cout << "[ShorTor]: finished updating middle weights. Weights inflated " <<  newMiddleWeightSum / middleWeightSum << "x in total." << std::endl;

		middleWeightSum = newMiddleWeightSum;
	}

	exitSumInv = (weight_t) 1 / exitWeightSum;

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

