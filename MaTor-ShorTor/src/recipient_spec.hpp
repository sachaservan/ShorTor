#ifndef RECIPIENT_SPEC_HPP
#define RECIPIENT_SPEC_HPP

/** @file */

#include <set>

#include "ip.hpp"

/**
 * Specification of a recipient (server) that Tor client connects to.
 */ 
class RecipientSpec : public std::enable_shared_from_this<RecipientSpec>
{
	public:
		/**
		 * Initializes recipient specification without valid coordinates initialization.
		 * Sets IP address to default.
		 */ 
		RecipientSpec() : latitude(-1000), longitude(-1000) { }
		/**
		 * Initializes recipient specification without valid coordinates initialization.
		 * Sets specified ip address.
		 * @param ip recipient's IP address
		 */
		RecipientSpec(const IP& ip) : latitude(-1000), longitude(-1000), address(ip) { }
		/**
		 * Initializes recipient's variables.
		 * @param ip recipient's IP address
		 * @param lat recipient's latitude (in degrees)
		 * @param lon recipient's longitude (in degrees)
		 */
		RecipientSpec(const IP& ip, double lat, double lon) : latitude(lat), longitude(lon), address(ip) { }

		RecipientSpec(const std::string& ip, double lat=-1000, double lon = -1000) : address(ip), latitude(lat), longitude(lon){}
		
		IP address; /**< Recipient's IP address. */
		std::set<uint16_t> ports; /**< Set of port numbers used by recipient's server. */
		double latitude; /**< Recipient's latitude based on IP address (in degrees). */
		double longitude; /**< Recipient's longitude based on IP address (in degrees). */
};

#endif
