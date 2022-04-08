#include "relay.hpp"
#include "utils.hpp"

Relay::Relay(size_t reg, const std::string& name, const std::string& fingerprint,
	const std::string& publishedDate, const std::string& stringIPAddress,
	bool isGuard, bool isExit, bool isBadExit, bool isFast,
	bool isValid, bool isStable, bool isRunning, int bandwidth,
	const std::string& version) 
		: name(name), fingerprint(fingerprint), publishedDate(publishedDate),
		ipAddress(stringIPAddress), version(version), bandwidth(bandwidth), registeredAt(reg)
{
	flags = (isGuard ? RelayFlag::GUARD : 0) | (isExit ? RelayFlag::EXIT : 0) | (isBadExit ? RelayFlag::BAD_EXIT : 0)
		| (isFast ? RelayFlag::FAST : 0) | (isValid ? RelayFlag::VALID : 0) | (isStable ? RelayFlag::STABLE : 0)
		| (isRunning ? RelayFlag::RUNNING : 0);
}

void Relay::addPolicy(const std::string& entry)
{
	std::vector<std::string> policies = split(entry, '\n');
	for(std::string line : policies)
	{
		clogv("Parsing: " << line);
		while(line[0] == ' ')
			line = line.substr(1);
		bool accept = false;
		if(line[0] == 'a')
			accept = true;
		line = line.substr(7);
		
		clogvn(" stripped to " << line);

		if (line.empty()) {
			clogvn("Skipped this line");
			continue ; 
		}
		
		size_t t = line.find(':');
		std::string addr = line.substr(0, t);
		std::string port = line.substr(t + 1);
		
		t = port.find('-');
		uint16_t from, to;
		
		if(t != std::string::npos)
		{
			from = std::stoi(port.substr(0, t));
			to = std::stoi(port.substr(t + 1));
		}
		else if(port == "*")
			from = to = 0;
		else
			from = to = std::stoi(port);
		
		clogvn(from << " - " << to << " IP " << (accept ? "accept " : "reject") << " policy with address " << addr);
		policy.push_back(PolicyDescriptor(accept, IP(addr), from, to));
	}
}

bool Relay::supportsConnection(const IP& ip, uint16_t port) const
{
	for(auto entry : policy)
	{
		if(entry.address == ip)
		{
			if(entry.portBegin && entry.portEnd) // if policy is for specified ports
			{
				if(port >= entry.portBegin && port <= entry.portEnd) // if policy includes this port
				{
					if(entry.isAccept)
						return true; // port in accepted range
					else
						return false; // port in rejected range
				}
				// else continue loop
			}
						
			else // if policy is for any port
			{
				if(entry.isAccept)
					return true; // IP address is accepted
				return false; // IP address is rejected
			}
		}
	}
	return true; // no restrictions found, accepting
}
