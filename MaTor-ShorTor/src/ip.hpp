#ifndef IP_HPP
#define IP_HPP

/** @file */

#include <string>

#include "types/general_exception.hpp"

/**
 * IP parsing exception.
 */
class ip_parse_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			INVALID_ADDRESS, /**< IP address is in invalid format. */
			INVALID_MASK, /**< IP subnet mask is in invalid format. */	
		};
		
		// constructors
		/**
		 * Constructs exception instance.
		 * @param why reason of throwing exception
		 * @param addr IP address string.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		ip_parse_exception(reason why, std::string addr, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case INVALID_ADDRESS:
					this->message = "\"" + addr + "\" IP address format is invalid.";
					break;
				case INVALID_MASK:
					this->message = "\"" + addr + "\" IP subnet mask format is invalid.";
					break;
				default:
					this->message = "unknown reason for failing \"" + addr + "\" parsing.";
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~ip_parse_exception() throw () { }
		
		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
	
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "ip_parse_exception";
			return tag;
		}
};

/**
 * Class containing information about IP address.
 * Address 0 and mask 0 stand for "any" address.
 */
class IP
{
	public:
		// constructors
		/**
		 * Constructor creates instance out of IP address in quad-dotted notation (for IPv4).
		 * IPv4 addresses are supported. String address "*" stands for "any" IP.
		 * @param ipAddress IPv4 address in quad-dotted format with /CIDR or /quad-dotted subnet.
		 * @throw ip_parse_exception
		 */
		IP(const std::string& ipAddress);
		
		/**
		 * Creates "any" IP.
		 */
		IP() : address(0), mask(0) { }
		
		/**
		 * Creates IP based on specified address and subnet mask.
		 * If no mask is specified, default value is used.
		 * @param address IP address
		 * @param mask subnet mask
		 */
		IP(uint32_t address, uint32_t mask = 0xffffff00) : address(address), mask(mask) { } 
		
		// functions
		/**
		 * Applies subnet mask to the address as bitwise AND and yields network prefix.
		 * @return network prefix of the IP
		 */ 
		uint32_t getPrefix() const { return address & mask; }
		/**
		 * Checks whether given IP belongs to the same subnet.
		 * @param anotherIP tested IP address
		 * @return true, if given IP belongs to the same subnet, false otherwise.
		 */ 
		bool isSubnet(const IP& anotherIP) const
		{
			return !((address & mask) ^ (anotherIP.address & anotherIP.mask));
		}
		
		/**
		 * Checks if IP is equall to another IP (including "any" IP address)
		 * @param anotherIP another IP
		 */
		bool operator == (const IP& anotherIP) const 
		{
			return (address & mask & anotherIP.mask) == (anotherIP.address & mask & anotherIP.mask);
		}
		
		/**
		 * @return IP address in quad-dotted notation or "*", if IP address snands for "any".
		 */
		operator std::string() const
		{
			if(!address && !mask)
				return "*";
			return std::to_string((address & 0xff000000) >> 24) + "." + 
				   std::to_string((address & 0x00ff0000) >> 16) + "." + 
				   std::to_string((address & 0x0000ff00) >> 8) + "." + 
				   std::to_string((address & 0x000000ff));
		}
		
		// variables 
		uint32_t address; /**< IP address. */
		uint32_t mask; /**< Subnet mask. */
};

#endif
