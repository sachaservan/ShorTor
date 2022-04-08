#include "ps_lastor.hpp"

#include <algorithm>
#include <tuple>
#include <cmath>




// This function returns true if the first pair is "less"
// than the second one according to some metric
// In this case, we say the first pair is "less" if the first element of the first pair
// is less than the first element of the second pair
bool pairCompare(const std::pair<probability_t, size_t>& firstElem, const std::pair<probability_t, size_t>& secondElem) {
	return firstElem.first < secondElem.first;

}


PSLASTor::PSLASTor(
	std::shared_ptr<PSLASTorSpec> pathSelectionSpec, 
	std::shared_ptr<SenderSpec> senderSpec, 
	std::shared_ptr<RecipientSpec> recipientSpec,
	std::shared_ptr<RelationshipManager> relationshipManager,
	const Consensus &consensus) :
		PathSelectionStandard(senderSpec, recipientSpec, pathSelectionSpec, relationshipManager, consensus),
		alpha(pathSelectionSpec->alpha),
		cellSize(pathSelectionSpec->cellSize)
{
	if(senderSpec->longitude < -180 || senderSpec->longitude > 180)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_SENDER_LONG, senderSpec->longitude);
	if(senderSpec->latitude < -90 || senderSpec->latitude > 90)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_SENDER_LAT, senderSpec->latitude);
	if(recipientSpec->longitude < -180 || recipientSpec->longitude > 180)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_RECIPIENT_LONG, recipientSpec->longitude);
	if(recipientSpec->latitude < -90 || recipientSpec->latitude > 90)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_RECIPIENT_LAT, recipientSpec->latitude);
	
	if(alpha > 1 || alpha < 0)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_ALPHA, alpha);
	
	if(cellSize < 1 || cellSize > 50 || 900 % cellSize != 0)
		throw_exception(lastor_init_exception, lastor_init_exception::INVALID_CELL_SIZE, cellSize);
		
	size_t size = consensus.getSize();
	
	std::vector<Cluster> invClusters; // Vector of unique clusters.
	std::map<uint32_t, size_t> clustersLookup; // Map from cluster's ID to cluster's record number in invClusters vector.
	
	initMeasure(start, stop);
	
	clogsn("Computing single relay's roles possibilities..."); 
	makeMeasure(start);
	computeRelaysPossibilities(pathSelectionSpec);
	relations->assignConstraints(entryPossible, exitPossible);
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Assigning relays to clusters...");
	makeMeasure(start);
	
	clusterOf.resize(size);
	for(size_t i = 0; i < size; ++i)
	{
		const Relay& relay = consensus.getRelay(i);
		
		// computing in which cluster relay is located
		uint32_t cluster = getClusterID(relay.getLatitude(), relay.getLongitude());
		
		size_t clusterIndex;
		if(clustersLookup.count(cluster)) // cluster already registered
			clusterIndex = clustersLookup[cluster];
		else // create new cluster
		{
			clusterIndex = invClusters.size();
			invClusters.emplace_back(cluster, cellSize);
			clustersLookup[cluster] = clusterIndex;
		}
		if(invClusters[clusterIndex].centerLat > 1800 || invClusters[clusterIndex].centerLong > 3600)
		{
			std::cout << "Cluster got weird lat/long: " << invClusters[clusterIndex].centerLat << "/" << invClusters[clusterIndex].centerLong << std::endl;
		}
		
		// register relay in it's cluster
		clusterOf[i] = clusterIndex;
		invClusters[clusterIndex].relays.push_back(i);
		invClusters[clusterIndex].id = cluster;
		invClusters[clusterIndex].index = clusterIndex;
		
		// summing possible roles
		if(exitPossible[i])
			++invClusters[clusterIndex].exitCount; 
		if(entryPossible[i])
			++invClusters[clusterIndex].entryCount; 
		if(middlePossible[i])
			++invClusters[clusterIndex].middleCount; 
	}
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	size_t clustersNumber = invClusters.size();
	clogvn("There are " << clustersNumber << " clusters.");
	
	clogsn("Computing cluster relations...");
	makeMeasure(start);
	clusterASRelations.resize(clustersNumber);
	clusterRelations.resize(clustersNumber);
	for(size_t i = 0; i < clustersNumber; ++i)
	{
		clusterRelations[i].resize(clustersNumber, false);
		clusterASRelations[i].resize(clustersNumber, false);
		for(size_t j = 0; j < clustersNumber; ++j)
		{
			bool related = false;
			bool asRelated = false;
			for(size_t a : invClusters[i].relays)
			{
				for(size_t b : invClusters[j].relays)
				{
					if(relations->exitEntryRelated(a, b))
						asRelated = true;
					if(relations->entryMiddleRelated(a, b))
						related = true;
					if(related && asRelated)
						break;
				}
				if(related && asRelated)
					break;
			}
			clusterRelations[i][j] = related;
			clusterASRelations[i][j] = asRelated;
		}
	}
	
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Disabling distant entry clusters...");
	makeMeasure(start);
	
	std::vector<std::pair<probability_t, int>> entryDistances(clustersNumber);
	for(size_t i = 0; i < clustersNumber; ++i)
		entryDistances[i] = std::make_pair(distance(sender->latitude, sender->longitude, invClusters[i].centerLat, invClusters[i].centerLong), i);
	
	// sorting clusters by distance
	std::sort(entryDistances.begin(),entryDistances.end(), pairCompare);
	
	size_t totalClosest = static_cast<float>(clustersNumber) * (.2 + alpha * 0.8);  // only (20 + alpha * 80)% closest clusters may become entries
	clogsn(totalClosest << " closest clusters out of " << clustersNumber << " are allowed for Entry.");
	
	// checking if there are any valid entry clusters in range
	bool entryFound = false;
	for(size_t i = 0; i < totalClosest; ++i)
		if(invClusters[entryDistances[i].second].entryCount > 0)
		{
			entryFound = true;
			break;
		}
	
	if(!entryFound)
		throw_exception(lastor_init_exception, lastor_init_exception::NO_ENTRY, entryDistances[totalClosest].first);
	
	for(size_t i = totalClosest; i < clustersNumber; ++i)
	{
		// excluding relays
		for(auto& r : invClusters[entryDistances[i].second].relays)
			entryPossible[r] = false;
		invClusters[entryDistances[i].second].entryCount = 0;
	}
	
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Computing circuit distances...");
	makeMeasure(start);
	
	probability_t maxDistance = 0;
	exitClusters.resize(clustersNumber);
	for(auto &exit : invClusters)
		if(exit.exitCount)
		{
			exitClusters[exit.index].entries.resize(clustersNumber);
			probability_t recipientExitDist = distance(recipient->latitude, recipient->longitude, exit.centerLat, exit.centerLong);
			for(size_t i = 0; i < totalClosest; ++i) // take entries from previously sorted distances vector
			{
				auto& entry = invClusters[entryDistances[i].second];
				if(clusterASRelations[exit.index][entry.index])
					continue; // skip related cluster
				if(entry.entryCount)
				{
					exitClusters[exit.index].entries[entry.index].middleProbs.resize(clustersNumber, 0);
					for(auto &middle : invClusters)
					{
						if(clusterRelations[exit.index][middle.index] || clusterRelations[entry.index][middle.index])
							continue;
						if(middle.middleCount)
						{
							probability_t exitMiddleDist = distance(exit.centerLat, exit.centerLong, middle.centerLat, middle.centerLong);
							probability_t middleEntryDist = distance(entry.centerLat, entry.centerLong, middle.centerLat, middle.centerLong);
							
							probability_t circuitDistance = recipientExitDist + exitMiddleDist + middleEntryDist + entryDistances[i].first;
							exitClusters[exit.index].entries[entry.index].middleProbs[middle.index] = circuitDistance;
							if(circuitDistance > maxDistance)
								maxDistance = circuitDistance;
						}
					}
				}
			}
		}
	
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	
	clogsn("Computing weights and probabilities...");
	makeMeasure(start);
	probability_t exitWeightSum = 0;
	
	for(size_t exit = 0; exit < clustersNumber; ++exit)
		if(exitClusters[exit].entries.size())
			for(size_t entry = 0; entry < clustersNumber; ++entry)
				if(exitClusters[exit].entries[entry].middleProbs.size())
					for(size_t middle = 0; middle < clustersNumber; ++middle)
						if(exitClusters[exit].entries[entry].middleProbs[middle])
						{
							probability_t weight = pow(maxDistance - exitClusters[exit].entries[entry].middleProbs[middle], (1. - alpha));
							exitClusters[exit].exitProb += weight; // increasing exit probability
							exitClusters[exit].entries[entry].entryProb += weight;  // increasing entry probability
							exitClusters[exit].entries[entry].middleProbs[middle] = weight; // assigning middle probability
							exitWeightSum += weight; // increasing total weight	
						}
	

	// computing probabilities...
	for(size_t exit=0; exit < clustersNumber; ++exit)
		if(exitClusters[exit].exitProb > 0)
		{
			exitClusters[exit].exitProb /= exitWeightSum;
			for(size_t entry = 0; entry < clustersNumber; ++entry)
				if(exitClusters[exit].entries[entry].entryProb > 0)
				{
					exitClusters[exit].entries[entry].entryProb /= exitClusters[exit].exitProb;
					exitClusters[exit].entries[entry].entryProb /= exitWeightSum;
					for(size_t middle = 0; middle < clustersNumber; ++middle)
						if(exitClusters[exit].entries[entry].middleProbs[middle] > 0)
						{
							exitClusters[exit].entries[entry].middleProbs[middle] /= exitClusters[exit].exitProb;
							exitClusters[exit].entries[entry].middleProbs[middle] /= exitClusters[exit].entries[entry].entryProb;
							exitClusters[exit].entries[entry].middleProbs[middle] /= exitWeightSum;
							exitClusters[exit].entries[entry].middleProbs[middle] /= invClusters[middle].middleCount;
						}
					exitClusters[exit].entries[entry].entryProb /= invClusters[entry].entryCount;
				}
			exitClusters[exit].exitProb /= invClusters[exit].exitCount;
		}
	
	if (consensus.useVias()) {
		
		auto newMiddleProbs = std::vector<std::vector<std::vector<probability_t>>>(clustersNumber);

		// [ShorTor]: inflate weight of each relay
		for (size_t exit=0; exit < clustersNumber; ++exit)
		{			
			if(exitClusters[exit].exitProb > 0)
			{
				newMiddleProbs[exit].resize(clustersNumber);

				for (size_t entry = 0; entry < clustersNumber; ++entry)
				{
					if(exitClusters[exit].entries[entry].entryProb > 0)
					{
						newMiddleProbs[exit][entry].resize(clustersNumber);

						for (size_t middle = 0; middle < clustersNumber; ++middle)
						{
							probability_t entryProb = exitClusters[exit].entries[entry].entryProb;
							probability_t exitProb = exitClusters[exit].exitProb;
							probability_t middleProb = exitClusters[exit].entries[entry].middleProbs[middle];

							probability_t newMiddleProb = middleProb; // to be computed in the loop below

							for (auto &relay : invClusters[middle].relays)
							{
								std::vector<std::tuple<size_t, size_t>> allPairs = consensus.getPairsForVia(relay);

								if (allPairs.size() > 0) {
									if(!middlePossible[relay])
										++invClusters[middle].middleCount;
									middlePossible[relay] = true; 
								}
									
								for(size_t j = 0; j < allPairs.size(); ++j) {
									size_t r1 = std::get<0>(allPairs[j]);
									size_t r2 = std::get<1>(allPairs[j]);
									
									// if any of the current entry/exits are in the pairs for this relay, increase cluster probability
									if (clusterOf[r1] == entry || clusterOf[r2] == entry) {
										newMiddleProb += entryProb;	
									}

									if (clusterOf[r1] == exit || clusterOf[r2] == exit) {
										newMiddleProb += exitProb;	
									}
								}
							}

							newMiddleProbs[exit][entry][middle] = newMiddleProb;
						}
					}
				}
			}
		}
	
		// [ShorTor]: inflate weight of each relay
		for (size_t exit=0; exit < clustersNumber; ++exit)
		{			
			if(exitClusters[exit].exitProb > 0)
			{
				for (size_t entry = 0; entry < clustersNumber; ++entry)
				{
					if(exitClusters[exit].entries[entry].entryProb > 0)
					{
						for (size_t middle = 0; middle < clustersNumber; ++middle)
						{
							exitClusters[exit].entries[entry].middleProbs[middle] = newMiddleProbs[exit][entry][middle] /= invClusters[middle].middleCount;
						}
					}
				}
			}
		}
		
		std::cout << "[ShorTor]: finished updating middle weights." << std::endl;
		
	}


	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");
}

uint32_t PSLASTor::getClusterID(double latitude, double longitude) const
{	
	// Coordinates are multiplied by 10 because minimal cell size is 0.1 (convertion to "integer" form).
	// Then, 10*90 and 10*180 is added to results respectively in order to avoid negative numbers
	// (shifting from [-90, 90] to [0, 180] and from [-180, 180] to [0, 360].
	// Precision shift based on cell size truncates IDs to the desired precision.
	uint32_t intLat = static_cast<uint32_t>((10 * latitude + 900) / cellSize) * cellSize;
	uint32_t intLong = static_cast<uint32_t>((10 * longitude + 1800) / cellSize) * cellSize;
	
	// finally, both values (maximal size: 12 bits) are packed together to one 32-bit variable. 
	return  (intLat << 16) | intLong;
}

probability_t PSLASTor::exitProb(size_t exit) const
{
	return exitPossible[exit] ? exitClusters[clusterOf[exit]].exitProb : 0;
}

probability_t PSLASTor::entryProb(size_t entry, size_t exit) const
{
	size_t exitIndex = clusterOf[exit], entryIndex = clusterOf[entry];
	if(exitPossible[exit] && exitClusters[exitIndex].exitProb > 0)
		return entryPossible[entry] ? exitClusters[exitIndex].entries[entryIndex].entryProb : 0;
	return 0;
}

probability_t PSLASTor::middleProb(size_t middle, size_t entry, size_t exit) const
{
	if(!middleEntryExitAllowed(middle, entry, exit)) return 0;
	
	size_t exitIndex = clusterOf[exit], entryIndex = clusterOf[entry], middleIndex = clusterOf[middle];
	if(exitPossible[exit] && entryPossible[entry] && exitClusters[exitIndex].exitProb > 0 && exitClusters[exitIndex].entries[entryIndex].entryProb > 0)
		return middlePossible[middle] ? exitClusters[exitIndex].entries[entryIndex].middleProbs[middleIndex] : 0;
	return 0;
}

bool PSLASTor::entryExitAllowed(size_t entry, size_t exit) const
{
	return !(clusterASRelations[clusterOf[exit]][clusterOf[entry]]);
}

bool PSLASTor::middleEntryExitAllowed(size_t middle, size_t entry, size_t exit) const
{
	return !(clusterRelations[clusterOf[entry]][clusterOf[middle]] || clusterRelations[clusterOf[exit]][clusterOf[middle]]); 
}
