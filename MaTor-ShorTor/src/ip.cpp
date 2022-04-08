#include "ip.hpp"
#include "utils.hpp"

#include <vector>
#include <exception>

IP::IP(const std::string& ipAddress)
{
	if(ipAddress == "*")  // "any" IP
	{
		address = 0;
		mask = 0;
	}
	else
	{
		// extracting IPv4 subnet
		size_t t = ipAddress.find("/");
		std::string strAddress;
		if(t != std::string::npos)
		{
			std::string maskSubstr = ipAddress.substr(t+1);
			if(maskSubstr.find(".") == std::string::npos) // CIDR mask
			{
				int parseMask;
				try
				{
					parseMask = std::stoul(ipAddress.substr(t+1));
				}
				catch(std::exception)
				{
					throw_exception(ip_parse_exception, ip_parse_exception::INVALID_MASK, ipAddress);
				}
				if(32 - parseMask < 0)
					throw_exception(ip_parse_exception, ip_parse_exception::INVALID_MASK, ipAddress);
				
				mask = 0xffffffff ^ ((1 << (32 - parseMask)) - 1);
			}
			else // quad-dotted mask
			{
				std::vector<std::string> parts = split(maskSubstr, '.');
				mask = (std::stoul(parts[0]) << 24);
				mask += (std::stoul(parts[1]) << 16);
				mask += (std::stoul(parts[2]) << 8);
				mask += std::stoul(parts[3]);
			}
			// validating mask
			bool oneStream = true;
			for(int i = 31; i >= 0; --i)
			{
				if((mask >> i) & 1)
				{
					if(!oneStream)			
						throw_exception(ip_parse_exception, ip_parse_exception::INVALID_MASK, ipAddress);
				}
				else
					oneStream = false;
			}
			strAddress = ipAddress.substr(0, t);
		}
		else
		{
			mask = 0xffffff00;
			strAddress = ipAddress;
		}
		// parsing to IPv4
		std::vector<std::string> parts = split(strAddress, '.');
		if(parts.size() != 4)
			throw_exception(ip_parse_exception, ip_parse_exception::INVALID_ADDRESS, ipAddress);
		try
		{
			address = (std::stoul(parts[0]) << 24);
			address += (std::stoul(parts[1]) << 16);
			address += (std::stoul(parts[2]) << 8);
			address += std::stoul(parts[3]);
		}
		catch(std::exception)
		{
			throw_exception(ip_parse_exception, ip_parse_exception::INVALID_ADDRESS, ipAddress);
		}
		if(std::string(*this) != strAddress && address)
			throw_exception(ip_parse_exception, ip_parse_exception::INVALID_ADDRESS, ipAddress);
	}
}
