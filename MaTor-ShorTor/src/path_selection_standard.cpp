#include "path_selection_standard.hpp"

#include <algorithm>

bool PathSelectionStandard::LLPRequiresStable(const std::set<uint16_t>& llports) const
{
	for(uint16_t port : recipient->ports)
		if(llports.find(port) != llports.end())
			return true;
	
	return false;
}

std::vector<int> PathSelectionStandard::translateGuards(const std::set<std::string>& guards) const
{
	std::vector<int> parsed;
	
	int size = consensus.getSize();
	for(auto& guard : guards)
	{
		for(size_t i = 0; i < size; ++i)
		{
			const Relay& relay = consensus.getRelay(i);
			if(relay.getName() + "@" + std::string(relay.getAddress()) == guard)
			{
				parsed.push_back(relay.position());
				break;
			}
		}
	}
	return parsed;
}	

bool PathSelectionStandard::canBeExit(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const
{
	// disallow bad exit
	if(relay.hasFlags(RelayFlag::BAD_EXIT))
		return false;
	
	// check if relay has all required flags
	if(!relay.hasFlags(requiredFlags))
		return false;
	
	// if relay is the only guard, it should not be selected as exit
	if(guards.size() == 1)
		if(guards[0] == relay.position())
			return false;
	
	// if every possible guard is related to the relay, it should be disallowed
	// it must be precomputed because exit is chosen as first
	if(guards.size() > 0)
	{		
		bool everyGuardRelated = true;
		for(auto guard : guards)
			if(relations->exitEntryRelated(relay.position(), guards[0])) // at this point some of relations constraints may be yet uncomputed.
			{
				everyGuardRelated = false;
				break;
			}
		return !everyGuardRelated;
	}
	
	// relay allowed, when all tests are passed
	return true;
}

bool PathSelectionStandard::canBeEntry(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const
{
	// check if relay has all required flags
	if(!relay.hasFlags(requiredFlags))
		return false;
	
	if(guards.size() > 0)
		// relay allowed, if it's on guard list
		return (std::find(guards.begin(), guards.end(), relay.position()) != guards.end());
	else
		// relay allowed, if it can be guard
		return relay.hasFlags(RelayFlag::GUARD);
}

bool PathSelectionStandard::canBeMiddle(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const
{
	// check if relay has all required flags
	if(!relay.hasFlags(requiredFlags))
		return false;
	
	// if relay is the only guard, it should not be selected as middle
	if(guards.size() == 1)
		if(guards[0] == relay.position())
			return false;
	
	return true;
}

void PathSelectionStandard::computeRelaysPossibilities(std::shared_ptr<StandardSpec> psSpec, bool useGuards)
{
	size_t size = consensus.getSize();
	clogvn("Parsing guards...");
	std::vector<int> guards = translateGuards(psSpec->guards);
	if(!useGuards && !guards.empty())
	{
		clogsn("[Warning] Guards specified, but current Path Selection Algorithm doesn't use them.");
		guards.clear();
	}
	clogvn("Initializing vectors...");
	int bestSupport = 0;
		
	exitPossible.resize(size);
	entryPossible.resize(size);
	middlePossible.resize(size);
	
	std::vector<int> supportedPortsCardinalities(size, 0);
	
	// computing required flags for roles
	int exitRequiredFlags = 0 | (psSpec->requireFastFlags * RelayFlag::FAST)
		| (requireStable * RelayFlag::STABLE)
		| ((!psSpec->allowNonValidExit) * RelayFlag::VALID)
		| (psSpec->requireExitFlag * RelayFlag::EXIT
		| RelayFlag::RUNNING); // always require "running" flag
	
	int middleRequiredFlags = 0 | (psSpec->requireFastFlags * RelayFlag::FAST)
		| (requireStable * RelayFlag::STABLE)
		| ((!psSpec->allowNonValidMiddle) * RelayFlag::VALID
		| RelayFlag::RUNNING);
	
	int entryRequiredFlags = 0 | (psSpec->requireFastFlags * RelayFlag::FAST)
		| (requireStable * RelayFlag::STABLE)
		| ((!psSpec->allowNonValidEntry) * RelayFlag::VALID
		| RelayFlag::RUNNING);
	
	// compute relays possibilities
	clogvn("Assigning possibilities...");
	for(size_t i = 0; i < size; ++i)
	{
		const Relay& relay = consensus.getRelay(i);
		exitPossible[i] = canBeExit(relay, exitRequiredFlags, guards); 
		entryPossible[i] = canBeEntry(relay, entryRequiredFlags, guards); 
		middlePossible[i] = canBeMiddle(relay, middleRequiredFlags, guards);
		
		if(!exitPossible[i])
			continue;
		
		int relaySupport = 0;
		for(uint16_t port : recipient->ports)
			if(relay.supportsConnection(recipient->address, port))
				++relaySupport;
		
		supportedPortsCardinalities[i] = relaySupport;
		
		if(relaySupport > bestSupport)
			bestSupport = relaySupport;
	}
	
	// disallow relays which do not support enough ports
	clogsn("Excluding exits under best support...");
	int asdas = 123;
	for(size_t i = 0; i < size; ++i)
	{
		//clogsn("Relay " << i << " has cardinality " << supportedPortsCardinalities[i] << " and bestSupport is " << bestSupport);
		//clogsn("btw.: The policy is " << consensus.getRelay(i).getPolicy());
		if(supportedPortsCardinalities[i] < bestSupport)
			exitPossible[i] = false;
	}
}

