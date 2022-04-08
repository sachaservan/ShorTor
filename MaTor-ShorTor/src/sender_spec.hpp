#ifndef SENDER_SPEC_HPP
#define SENDER_SPEC_HPP

/** @file */

#include "ip.hpp"

/**
 * Specification of Tor network user (sender).
 */ 
class SenderSpec : public std::enable_shared_from_this<SenderSpec>
{
	public:
		/**
		 * Initializes sender specification without valid coordinates initialization.
		 * Sets IP address to default.
		 */ 
		SenderSpec() : latitude(-1000), longitude(-1000) { }
		/**
		 * Initializes sender specification without valid coordinates initialization.
		 * Sets specified ip address.
		 * @param ip sender's IP address
		 */
		SenderSpec(const IP& ip) : latitude(-1000), longitude(-1000), address(ip) { }
		/**
		 * Initializes sender's variables.
		 * @param ip sender's IP address
		 * @param lat sender's latitude (in degrees)
		 * @param lon sender's longitude (in degrees)
		 */
		SenderSpec(const IP& ip, double lat, double lon) : latitude(lat), longitude(lon), address(ip) { }


		SenderSpec(const std::string& ip, double lat = -1000, double lon = -1000) : latitude(lat), longitude(lon), address(ip) { }
		
		IP address; /**< Sender's IP address. */
		double latitude; /**< Sender's latitude based on IP address (in degrees). */
		double longitude; /**< Sender's longitude based on IP address (in degrees). */
};

#endif
