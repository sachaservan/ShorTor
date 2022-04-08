#include "relationship_manager.hpp"

#include <algorithm>

SubnetRelations::SubnetRelations(const Consensus& consensus) : consensus(consensus), relations(consensus.getSize(), true)
{
	size_t size = consensus.getSize();
	for(size_t i = 0; i < size; ++i)
	{
		relations[i][i] = true;
		for(size_t j = i + 1; j < size; ++j)
			if(consensus.isRelated(i, j) || consensus.getRelay(i).getAddress().isSubnet(consensus.getRelay(j).getAddress()))
				relations[i][j] = true;
	}
}

ASRelations::ASRelations(const Consensus& consensus, const IP& senderIP, const IP& recipientIP)
	: SubnetRelations(consensus), senderIP(senderIP), recipientIP(recipientIP)
{
	size_t size = consensus.getSize();
	
	// resize all vectors
	canBeExit.resize(size, false);
	canBeEntry.resize(size, false);
	ASrelations.resize(size);
	for(size_t i = 0; i < size; ++i)
		ASrelations[i].resize(size);
	
	// assign standard relationships for exit and entry
	for(size_t i = 0; i < size; ++i)
		for(size_t j = 0; j < size; ++j)
			ASrelations[i][j] = relations[i][j];
}

void ASRelations::assignConstraints(std::vector<bool>& exitPossible, std::vector<bool>& entryPossible) 
{
	size_t size = consensus.getSize();
	
	// assigning AS relationships for exit and entry
	for(size_t i = 0; i < size; ++i)
		if(exitPossible[i])
		{
			// for each possible exit, obtain route
			std::set<std::string> exitRoute = tracert(consensus.getRelay(i).getAddress(), recipientIP);
			for(size_t j = 0; j < size; ++j)
				if(!ASrelations[i][j])
				{
					// for each possible entry (for exit specified), obtain route
					std::set<std::string> entryRoute = tracert(senderIP, consensus.getRelay(j).getAddress());
					
					// intersect both routes
					std::vector<std::string> intersection (entryRoute.size() + exitRoute.size());
					auto end = std::set_intersection(entryRoute.begin(), entryRoute.end(), exitRoute.begin(), exitRoute.end(), intersection.begin());
					
					if(end != intersection.begin()) // if routes intersection is not empty
						ASrelations[i][j] = true;					
					else
						canBeExit[i] = canBeEntry[j] = true; // entry and exit allowed
				}
		}
	
	// reassign possibilities
	for(size_t i = 0; i < size; ++i)
	{
		exitPossible[i] = exitPossible[i] & canBeExit[i];
		entryPossible[i] = entryPossible[i] & canBeEntry[i];
	}
}

std::set<std::string> ASRelations::tracert(const IP& startIP, const IP& endIP)
{
	std::set<std::string> route;
	return route;
}
