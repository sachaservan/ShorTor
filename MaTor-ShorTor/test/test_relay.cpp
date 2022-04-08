#define TEST_NAME "Relay"

#include "stdafx.h"

#include <ip.hpp>
#include <relay.hpp>

#define RELAY_FINGERPRINT "BC630CBBB518BE7E9F4E09712AB0269E9DC7D626"
#define RELAY_NAME "IPredator"
#define RELAY_PUBLISHED "2014-10-21 02:12:34"
#define RELAY_ADDRESS "197.231.221.211"
#define RELATY_ADDRESS2 "8.8.8.8"
#define RELAY_FLAGS RelayFlag::EXIT | RelayFlag::FAST | RelayFlag::GUARD
#define RELAY_BANDWIDTH 12345
#define RELAY_VERSION "0.2.6.10"
#define RELAY_PLATFORM "Linux"
#define RELAY_REGISTERED_AT 0

struct TestRelay
{
	Relay r;

	TestRelay() : r(RELAY_REGISTERED_AT, RELAY_NAME, RELAY_FINGERPRINT, RELAY_PUBLISHED, RELAY_ADDRESS, RELAY_FLAGS, RELAY_BANDWIDTH, RELAY_VERSION) {}

	void TestFlagHelper(void(Relay::*helper)(bool), int flag)
	{
		(r.*helper)(true);
		BOOST_CHECK_EQUAL(r.hasFlags(flag), true);

		(r.*helper)(false);
		BOOST_CHECK_EQUAL(r.hasFlags(flag), false);
	}

	void TestIPSupport(const std::string& stringIP, uint16_t port, bool shouldAllow)
	{
		IP testIP (stringIP);
		// BOOST_TEST_MESSAGE("Testing IP: " + stringIP + ":" + std::to_string(port));
		BOOST_CHECK_EQUAL(r.supportsConnection(testIP, port), shouldAllow);
	}
};

BOOST_FIXTURE_TEST_SUITE(RelaySuite, TestRelay)

BOOST_AUTO_TEST_CASE(GettingSetting)
{
	// Fingerprint
	BOOST_CHECK_EQUAL(r.getFingerprint(), RELAY_FINGERPRINT);
	r.setFingerprint("F4E09712AB0269E9DC7D626BC630CBBB518BE7E9");
	BOOST_CHECK_EQUAL(r.getFingerprint(), "F4E09712AB0269E9DC7D626BC630CBBB518BE7E9");

	// Nickname
	BOOST_CHECK_EQUAL(r.getName(), RELAY_NAME);
	r.setName("asdf");
	BOOST_CHECK_EQUAL(r.getName(), "asdf");

	// Published date
	BOOST_CHECK_EQUAL(r.getPublishedDate(), RELAY_PUBLISHED);
	r.setPublishedDate("2014-10-21 02:12:35");
	BOOST_CHECK_EQUAL(r.getPublishedDate(), "2014-10-21 02:12:35");

	// Address
	BOOST_CHECK_EQUAL(std::string(r.getAddress()), RELAY_ADDRESS);
	IP alternativeIP ("8.8.8.8");
	r.setAddress(alternativeIP);
	BOOST_CHECK_EQUAL(std::string(r.getAddress()), "8.8.8.8");

	// Bandwidth
	BOOST_CHECK_EQUAL(r.getBandwidth(), RELAY_BANDWIDTH);
	r.setBandwidth(42);
	BOOST_CHECK_EQUAL(r.getBandwidth(), 42);

	// Version
	BOOST_CHECK_EQUAL(r.getVersion(), RELAY_VERSION);
	r.setVersion("1.0.0");
	BOOST_CHECK_EQUAL(r.getVersion(), "1.0.0");

	// Platform
	r.setPlatform(RELAY_PLATFORM);
	BOOST_CHECK_EQUAL(r.getPlatform(), RELAY_PLATFORM);

	// Country
	r.setCountry("PL");
	BOOST_CHECK_EQUAL(r.getCountry(), "PL");

	// ASNumber
	r.setASNumber("1234");
	BOOST_CHECK_EQUAL(r.getASNumber(), "1234");

	// ASName
	r.setASName("XKCD");
	BOOST_CHECK_EQUAL(r.getASName(), "XKCD");

	// Latitude, Longitude
	float lat (12.34);
	float lng (56.78);
	r.setLatitude(lat);
	r.setLongitude(lng);
	BOOST_CHECK_EQUAL(r.getLatitude(), lat);
	BOOST_CHECK_EQUAL(r.getLongitude(), lng);
}

BOOST_AUTO_TEST_CASE(Flags)
{
	// Flags from constructor
	BOOST_CHECK_EQUAL(r.getFlags(), RELAY_FLAGS);
	
	// Reseting flags
	r.resetFlags();
	BOOST_CHECK_EQUAL(r.getFlags(), 0);

	// Setting flags
	r.setFlags(RELAY_FLAGS);
	BOOST_CHECK_EQUAL(r.getFlags(), RELAY_FLAGS);

	// Having flags
	BOOST_CHECK_EQUAL(r.hasFlags(RELAY_FLAGS), true);

	// Setting one flag
	r.setFlag(RelayFlag::AUTHORITY, true);
	BOOST_CHECK_EQUAL(r.hasFlags(RelayFlag::AUTHORITY), true);
	r.setFlag(RelayFlag::AUTHORITY, false);
	BOOST_CHECK_EQUAL(r.hasFlags(RelayFlag::AUTHORITY), false);
}

BOOST_AUTO_TEST_CASE(FlagHelpers)
{
	r.resetFlags();

	TestFlagHelper(&Relay::setBadExit, RelayFlag::BAD_EXIT);
	TestFlagHelper(&Relay::setGuard, RelayFlag::GUARD);
	TestFlagHelper(&Relay::setExit, RelayFlag::EXIT);
	TestFlagHelper(&Relay::setValid, RelayFlag::VALID);
	TestFlagHelper(&Relay::setStable, RelayFlag::STABLE);
	TestFlagHelper(&Relay::setRunning, RelayFlag::RUNNING);
	TestFlagHelper(&Relay::setFast, RelayFlag::FAST);
}

BOOST_AUTO_TEST_CASE(IPPoliciesStandard)
{
	r.addPolicy("reject 0.0.0.0/8:*");
	r.addPolicy("reject 169.254.0.0/16:*");
	r.addPolicy("reject 127.0.0.0/8:*");
	r.addPolicy("reject 192.168.0.0/16:*");
	r.addPolicy("reject 10.0.0.0/8:*");
	r.addPolicy("reject 172.16.0.0/12:*");
	r.addPolicy("reject 197.231.221.211:*");
	r.addPolicy("reject *:23");
	r.addPolicy("reject *:25");
	r.addPolicy("reject *:80");
	r.addPolicy("reject *:109");
	r.addPolicy("reject *:110");
	r.addPolicy("reject *:143");
	r.addPolicy("reject *:465");
	r.addPolicy("reject *:587");
	r.addPolicy("reject *:119");
	r.addPolicy("reject *:135-139");
	r.addPolicy("reject *:445");
	r.addPolicy("reject *:563");
	r.addPolicy("reject *:1214");
	r.addPolicy("reject *:4661-4666");
	r.addPolicy("reject *:6346-6429");
	r.addPolicy("reject *:6699");
	r.addPolicy("reject *:6881-6999");
	r.addPolicy("accept *:*");

	TestIPSupport("0.0.0.0", 443, false);
	TestIPSupport("8.8.8.8", 80, false);
	TestIPSupport("8.8.8.8", 137, false);
	TestIPSupport("169.254.12.1", 443, false);
	TestIPSupport("8.8.8.8", 443, true);
}

BOOST_AUTO_TEST_CASE(IPPoliciesReject)
{
	r.addPolicy("reject *:*");
	r.addPolicy("accept *:*");

	TestIPSupport("0.0.0.0", 443, false);
	TestIPSupport("8.8.8.8", 80, false);
	TestIPSupport("8.8.8.8", 137, false);
	TestIPSupport("169.254.12.1", 443, false);
	TestIPSupport("8.8.8.8", 443, false);
}

BOOST_AUTO_TEST_CASE(IPPoliciesAccept)
{
	r.addPolicy("accept *:*");
	r.addPolicy("reject *:*");

	TestIPSupport("0.0.0.0", 443, true);
	TestIPSupport("8.8.8.8", 80, true);
	TestIPSupport("8.8.8.8", 137, true);
	TestIPSupport("169.254.12.1", 443, true);
	TestIPSupport("8.8.8.8", 443, true);
}

BOOST_AUTO_TEST_CASE(IPPoliciesOrder)
{
	r.addPolicy("reject 0.0.0.0/8:*");
	r.addPolicy("reject 169.254.0.0/16:*");
	r.addPolicy("reject 127.0.0.0/8:*");
	r.addPolicy("reject 192.168.0.0/16:*");
	r.addPolicy("reject 10.0.0.0/8:*");
	r.addPolicy("reject 172.16.0.0/12:*");
	r.addPolicy("reject 197.231.221.211:*");
	r.addPolicy("reject *:23");
	r.addPolicy("reject *:25");
	r.addPolicy("accept 8.8.8.8:80-88"); // This one is added
	r.addPolicy("reject *:80");
	r.addPolicy("reject *:109");
	r.addPolicy("reject *:110");
	r.addPolicy("reject *:143");
	r.addPolicy("reject *:465");
	r.addPolicy("reject *:587");
	r.addPolicy("reject *:119");
	r.addPolicy("reject *:135-139");
	r.addPolicy("reject *:445");
	r.addPolicy("reject *:563");
	r.addPolicy("reject *:1214");
	r.addPolicy("reject *:4661-4666");
	r.addPolicy("reject *:6346-6429");
	r.addPolicy("reject *:6699");
	r.addPolicy("reject *:6881-6999");
	r.addPolicy("accept *:*");

	TestIPSupport("0.0.0.0", 443, false);
	TestIPSupport("8.8.8.8", 80, true);
	TestIPSupport("8.8.8.8", 137, false);
	TestIPSupport("169.254.12.1", 443, false);
	TestIPSupport("8.8.8.8", 443, true);
}

BOOST_AUTO_TEST_SUITE_END()
