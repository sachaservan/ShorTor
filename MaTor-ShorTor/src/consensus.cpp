#include "consensus.hpp"
#include "utils.hpp"
#include <chrono>
#include <fstream>

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}   

Consensus::Consensus(const std::string& consensusFileName, const std::string& DBFileName, const std::string& viaAllPairsFileName, bool useVias)
{
	initMeasure(start, stop);

	useViaRelays = useVias;
	
	clogsn("Reading and parsing consensus file " << consensusFileName << "...");
	makeMeasure(start);
	loadConsensusFile(consensusFileName);
	makeMeasure(stop);
	clogsn("\tDone in " << measureTime(start, stop) << " ms.");

	relations = std::unique_ptr<SymmetricMatrix<bool>>(new SymmetricMatrix<bool>(relays.size(), true));
	
	if (!DBFileName.empty()) {
		clogsn("Opening / creating database. If this is the first time this consensus file is used, this step may take a while...");
		makeMeasure(start);
		DBConnection dbConnection(DBFileName, validAfter);
		makeMeasure(stop);
		clogsn("\tDone in " << measureTime(start, stop) << " ms.");

		clogsn("Loading node information...");
		makeMeasure(start);
		loadRelaysDBInformation(dbConnection);
		makeMeasure(stop);
		clogsn("\tDone in " << measureTime(start, stop) << " ms.");
	}


	viaPairs = std::vector<std::vector<std::tuple<size_t, size_t>>>(getSize()); // all pairs of relays that use this relay as a via relay
	size_t totalNumPairs = 0;
	size_t minNumPairs = 1 << 64;
	size_t maxNumPairs = 0;

	if (useVias && !viaAllPairsFileName.empty()) {

		std::cout << "[ShorTor]: via pairs file is: " << viaAllPairsFileName << std::endl;

		std::ifstream       file(viaAllPairsFileName);
		CSVRow              row;
		while(file >> row)
		{
			std::string relayFP = row[0]; // first element of the row is the via fingerprint 
			size_t relay = fingerprintMap[relayFP];

			for (size_t i = 1; i < row.size() - 1; i += 2) {
				std::string fpr1 = row[i]; // fingerprint of first relay in the pair
				std::string fpr2 = row[i+1]; // fingerprint of second relay in the pair

				// convert fingerprints to indexes
				size_t r1 = fingerprintMap[fpr1];
				size_t r2 = fingerprintMap[fpr2];

				// add the pair to the list
				std::tuple<size_t, size_t> pair = std::make_tuple(r1, r2);
				viaPairs[relay].push_back(pair);
				
				totalNumPairs ++;
			}

			// bookkeeping
			if (((row.size() - 1)/2) > maxNumPairs)
				maxNumPairs = ((row.size() - 1)/2);
			
			if (((row.size() - 1)/2) < minNumPairs)
				minNumPairs = ((row.size() - 1)/2);
		}

		std::cout << "[ShorTor]: finished constructing via pairs: " << viaPairs.size() << " in total." << std::endl;
		std::cout << "[ShorTor]: average number of pairs per via relay: " << totalNumPairs / viaPairs.size() << "." << std::endl;
		std::cout << "[ShorTor]: average number of pairs per via relay: " << totalNumPairs / viaPairs.size() << "." << std::endl;
		std::cout << "[ShorTor]: min number of pairs per via relay: " << minNumPairs << "." << std::endl;
		std::cout << "[ShorTor]: max number of pairs per via relay: " << maxNumPairs << "." << std::endl;

	}

	clogsn("Consensus initialized successfully.");	
}

Consensus::Consensus(const std::string& binaryFileName)
{
	NOT_IMPLEMENTED;
}

void Consensus::loadConsensusFile(const std::string& fileName)
{
	std::ifstream consensusStream(fileName);
	if (!consensusStream.is_open())
		throw_exception(consensus_exception, consensus_exception::OPEN_FILE, fileName);
	
	std::string line;
	
	// auxiliary variables;
	std::string name;
	std::string ipAddr;
	std::string published;
	std::string fingerprint;
	std::string version;
	int flags, bandwidth;
	
	// initialize max modifier
	maxModifier = 0;
	
	// relay's position in relays vector
	size_t registerNumber = 0;
	
	// State machine parsing consensus file.
	int state = 0;
	while(getline(consensusStream, line))
	{
		// should we use right trim on line?
		try
		{
			std::vector<std::string> tokens = split(line);
			if(!tokens.size())
				continue;
			
			switch(state)
			{
				case 0: // parsing header of the consensus file
					if(tokens[0] == "valid-after")
					{
						if(tokens.size() < 3)
							throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
						
						validAfter = tokens[1] + " " + tokens[2];
						state = 1;
					}						
					break;
				case 1: // looking for the first relay description
					if(tokens[0] == "r") // beginning relays description
					{
						if(tokens.size() < 7)
							throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
						
						// parse the first relay's r line
						name = tokens[1];
						fingerprint = b64toHex(tokens[2]);
						published = tokens[4] + " " + tokens[5];
						ipAddr = tokens[6];
						
						state = 2;
						version = "unknown";
						flags = 0;
						bandwidth = 0;
					}
					break;
				case 2: // parsing relays description
					switch(tokens[0][0])
					{
						case 'r': // relay's main description
							relays.emplace_back(registerNumber, name, fingerprint, published, ipAddr, flags, bandwidth, version);
							++registerNumber;
							
							if(tokens.size() < 7)
								throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
							
							name = tokens[1];
							fingerprint = b64toHex(tokens[2]);
							published = tokens[4] + " " + tokens[5];
							ipAddr = tokens[6];
							version = "unknown";
							flags = 0;
							bandwidth = 0;
							break;
						case 's': // relay's flags
							flags |= (findToken(tokens, "Authority") ? RelayFlag::AUTHORITY : 0);
							flags |= (findToken(tokens, "BadExit") ? RelayFlag::BAD_EXIT : 0);
							flags |= (findToken(tokens, "Exit") ? RelayFlag::EXIT : 0);
							flags |= (findToken(tokens, "Fast") ? RelayFlag::FAST : 0);
							flags |= (findToken(tokens, "Guard") ? RelayFlag::GUARD : 0);
							flags |= (findToken(tokens, "HSDir") ? RelayFlag::HS_DIR : 0);
							flags |= (findToken(tokens, "Named") ? RelayFlag::NAMED : 0);
							flags |= (findToken(tokens, "Stable") ? RelayFlag::STABLE : 0);
							flags |= (findToken(tokens, "Running") ? RelayFlag::RUNNING : 0);
							flags |= (findToken(tokens, "Unnamed") ? RelayFlag::UNNAMED : 0);
							flags |= (findToken(tokens, "Valid") ? RelayFlag::VALID : 0);
							flags |= (findToken(tokens, "V2Dir") ? RelayFlag::V2_DIR : 0);
							break;
						case 'w': // relay's bandwidth
							{
								if(tokens.size() < 2)
									throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
								
								std::vector<std::string> parts = split(tokens[1], '=');
								
								if(parts.size() < 2)
									throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
								
								bandwidth = std::stoi(parts[1]);
							}
							break;
						case 'v': // relay's Tor version
							if(tokens.size() < 3)
								throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
							version = tokens[2];
							break;
						case 'd': // directory footer
							// save the last relay
							relays.emplace_back(registerNumber, name, fingerprint, published, ipAddr, flags, bandwidth, version);
							++registerNumber;
							state = 3;
							break;
						default:
							break;
					}
					break;
				case 3: // parsing footer
					{
						if(tokens[0] == "bandwidth-weights")
						{
							for(int i = 1; i < tokens.size(); ++i)
							{
								std::vector<std::string> mods = split(tokens[i], '=');
								if(mods.size() < 2)
									throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
								
								weight_t weight = (weight_t)std::stoi(mods[1]);
								if(mods[0] == "Wed")
									weightMods.wed = weight;
								else if(mods[0] == "Weg")
									weightMods.weg = weight;
								else if(mods[0] == "Wee")
									weightMods.wee = weight;
								else if(mods[0] == "Wem")
									weightMods.wem = weight;
								else if(mods[0] == "Wgd")
									weightMods.wgd = weight;
								else if(mods[0] == "Wgg")
									weightMods.wgg = weight;
								else if(mods[0] == "Wgm")
									weightMods.wgm = weight;
								else if(mods[0] == "Wmd")
									weightMods.wmd = weight;
								else if(mods[0] == "Wmg")
									weightMods.wmg = weight;
								else if(mods[0] == "Wme")
									weightMods.wme = weight;
								else if(mods[0] == "Wmm")
									weightMods.wmm = weight;
								
								if(weight > maxModifier)
									maxModifier = weight;
							}
							state = 4;
						}
					}
					break;
				default:
					break;
			}
		}
		catch(std::exception)
		{
			throw_exception(consensus_exception, consensus_exception::INVALID_FORMAT, fileName, line);
		}
	}
	if(state != 4)
		throw_exception(consensus_exception, consensus_exception::INSUFFICIENT_FILE, fileName);
		
	clogvn("There are " << relays.size() << " relays declared.");
	// parsing done.
}

void Consensus::loadRelaysDBInformation(const DBConnection& dbConnection)
{
	size_t size = relays.size();
	
	// mapping relays' fingerprints to position in relays vector...
	std::vector<std::string> fingerprintVector = std::vector<std::string>(size);
	for(size_t i = 0; i < size; ++i)
	{
		fingerprintVector[i] = relays[i].getFingerprint();
		fingerprintMap[relays[i].getFingerprint()] = i;
	}
	
	// reading single relay information from DB in chunks...
	int chunkSize = 500;
	clogsn("reading single relay information from DB in chunks...");
	for(size_t i=0; i < size; i+=chunkSize)
	{
		auto resultVector = dbConnection.readRelaysInfo(fingerprintVector.begin() + i, i + chunkSize >= size ? fingerprintVector.end() : fingerprintVector.begin() + i + chunkSize);
		for(const auto& row : resultVector)
		{
			Relay &filledRelay = relays[fingerprintMap[row[0]]];
			filledRelay.setAveragedBandwidth(std::stoi(row[1]));
			filledRelay.setPlatform(row[2]);
			filledRelay.addPolicy(row[3]);
			clogsn("Found policy: " << row[3] << " for relay " << row[8]);
			filledRelay.setCountry(row[4]);
			filledRelay.setLatitude(std::stof(row[5]));
			filledRelay.setLongitude(std::stof(row[6]));
			filledRelay.setASNumber("AS" + row[7]);
			filledRelay.setASName(row[8]);
		}
	}

	// assigning families...
	clogsn("Assigning families");
	auto resultVector = dbConnection.readFamilies();
	for(const auto& row : resultVector)
	{
		if(!fingerprintMap.count(row[0]) || !fingerprintMap.count(row[1]))
			continue;
		
		int nodeA = fingerprintMap[row[0]];
		int nodeB = fingerprintMap[row[1]];
		(*relations)[nodeA][nodeB] = true;
	}
}

void Consensus::saveBinary(const std::string& output) const
{
	NOT_IMPLEMENTED;
}


