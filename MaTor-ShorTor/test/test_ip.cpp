#define TEST_NAME "IP"

#include "stdafx.h"

#include <ip.hpp>

bool isInvalidAddressException(ip_parse_exception const& ex) { return ex.why() == ip_parse_exception::reason::INVALID_ADDRESS; }
bool isInvalidMaskException(ip_parse_exception const& ex) { return ex.why() == ip_parse_exception::reason::INVALID_MASK; }



BOOST_AUTO_TEST_SUITE(IPSuite)

BOOST_AUTO_TEST_CASE(IP_Parsing)
{	
	// Standard IPv4
	IP stdIP ("134.96.225.122");
	BOOST_CHECK_EQUAL(stdIP.address, 2254496122); // 134.96.225.122

	// Default mask /24
	BOOST_CHECK_EQUAL(stdIP.mask, 4294967040); // 255.255.255.0

	// Empty IP
	BOOST_CHECK_EXCEPTION(IP(""), ip_parse_exception, isInvalidAddressException);

	// Wrong IP
	BOOST_CHECK_EXCEPTION(IP("1.1.1.1.1"), ip_parse_exception, isInvalidAddressException);
}

BOOST_AUTO_TEST_CASE(IP_CIDR)
{
	IP cidrIP ("134.96.225.122/20");
	BOOST_CHECK_EQUAL(cidrIP.address, 2254496122); // 134.96.225.122
	BOOST_CHECK_EQUAL(cidrIP.mask, 4294963200); // 255.255.240.0
	BOOST_CHECK_EQUAL(cidrIP.getPrefix(), 2254495744); // 134.96.224.0

	// Empty IP
	BOOST_CHECK_EXCEPTION(IP("/20"), ip_parse_exception, isInvalidAddressException);

	// Wrong IP
	BOOST_CHECK_EXCEPTION(IP("1.1.1.1.1/20"), ip_parse_exception, isInvalidAddressException);

	// Mask out of range
	BOOST_CHECK_EXCEPTION(IP("1.1.1.1/33"), ip_parse_exception, isInvalidMaskException);
}

BOOST_AUTO_TEST_CASE(IP_QuadDottedMask)
{
	IP cidrIP ("134.96.225.122/255.255.240.0");
	BOOST_CHECK_EQUAL(cidrIP.address, 2254496122); // 134.96.225.122
	BOOST_CHECK_EQUAL(cidrIP.mask, 4294963200); // 255.255.240.0
	BOOST_CHECK_EQUAL(cidrIP.getPrefix(), 2254495744); // 134.96.224.0

	// Empty IP
	BOOST_CHECK_EXCEPTION(IP("/255.255.240.0"), ip_parse_exception, isInvalidAddressException);

	// Wrong IP
	BOOST_CHECK_EXCEPTION(IP("1.1.1.1.1/255.255.240.0"), ip_parse_exception, isInvalidAddressException);

	// Mask out of range
	BOOST_CHECK_EXCEPTION(IP("1.1.1.1/255.0.255.0"), ip_parse_exception, isInvalidMaskException);
}

BOOST_AUTO_TEST_CASE(IP_Conversion)
{
	IP stdIP ("134.96.225.122");
	BOOST_CHECK_EQUAL(std::string(stdIP), "134.96.225.122");
}

BOOST_AUTO_TEST_CASE(IP_Subnets)
{
	{
		// Positive check
		IP firstIP ("134.96.225.122/24");
		IP secondIP ("134.96.225.12/24");

		BOOST_CHECK(firstIP.isSubnet(secondIP) == true);
		BOOST_CHECK(secondIP.isSubnet(firstIP) == true);
	}

	{
		// Negative check
		IP firstIP ("134.96.225.122/30");
		IP secondIP ("134.96.225.12/30");

		BOOST_CHECK(firstIP.isSubnet(secondIP) == false);
		BOOST_CHECK(secondIP.isSubnet(firstIP) == false);
	}
}

BOOST_AUTO_TEST_SUITE_END()