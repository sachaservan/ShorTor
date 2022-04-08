#ifndef RELAY_HPP
#define RELAY_HPP

/** @file */

#include <vector>
#include <string>
#include <memory>

#include "ip.hpp"

/**
 * @enum RelayFlag
 * Relay's flags positions.
 */
enum RelayFlag
{
	AUTHORITY = 1, /**< Relay is directory authority */
	BAD_EXIT = 1 << 1, /**< Relay is useless as exit node */
	EXIT = 1 << 2, /**< Relay is preferred to be used as exit */
	FAST = 1 << 3, /**< Relay is fast (relay suitable for high-bandwidth circuits) */
	GUARD = 1 << 4, /**< Relay is suitable to be used as guard */ 
	HS_DIR = 1 << 5, /**< Relay is considered a Hidden Service Directory */
	NAMED = 1 << 6, /**< Relay has a nickname */
	STABLE = 1 << 7, /**< Relay is stable (relay suitable for long-lived circuits) */
	RUNNING = 1 << 8, /**< Relay is currently usable */
	UNNAMED = 1 << 9, /**< Relay's nickname is used by another relay */
	VALID = 1 << 10, /**< Relay is validated */
	V2_DIR = 1 << 11, /**< Relay supports V2_DIR protocol */
};

/**
 * Relay in Tor network.
 * Class contains information related to the single relay
 * obtained from consensus and server descriptors. It also cantains information
 * about Autonomous System Number and Name.
 */
class Relay : public std::enable_shared_from_this<Relay>
{
	public:
		//constructors
		/**
		 * Initializes relay with values extracted from consensus file.
		 * Parses IP address.
		 * @param reg Position in relays vector in consensus
		 * @param name name of the relay in Tor network.
		 * @param fingerprint relay's fingerprint in hexadecimal string format without leading 0x.
		 * @param publishedDate date of publishing descriptors referenced by consensus in YYYY-MM-DD hh:mm:ss format.
		 * @param stringIPAddress relay's IP address in quad-dotted notation.
		 * @param isGuard indicates whether relay got "Guard" flag in consensus file.
		 * @param isExit indicates whether relay got "Exit" flag in consensus file.
		 * @param isBadExit indicates whether relay got "BadExit" flag in consensus file.
		 * @param isFast indicates whether relay got "Fast" flag in consensus file.
		 * @param isValid indicates whether relay got "Valid" flag in consensus file.
		 * @param isStable indicates whether relay got "Stable" flag in consensus file.
		 * @param isRunning indicates whether relay got "Running" flag in consensus file.
		 * @param bandwidth relay's bandwidth assigned in consensus file.
		 * @param version Tor version used by the ralay.
		 */
		Relay(size_t reg, const std::string& name, const std::string& fingerprint,
			const std::string& publishedDate, const std::string& stringIPAddress,
			bool isGuard, bool isExit, bool isBadExit, bool isFast,
			bool isValid, bool isStable, bool isRunning, int bandwidth,
			const std::string& version);
			
		/**
		 * Initializes relay with values extracted from consensus file and compressed flags.
		 * Parses IP address.
		 * @param reg Position in relays vector in consensus
		 * @param name name of the relay in Tor network.
		 * @param fingerprint relay's fingerprint in hexadecimal string format without leading 0x.
		 * @param publishedDate date of publishing descriptors referenced by consensus in YYYY-MM-DD hh:mm:ss format.
		 * @param stringIPAddress relay's IP address in quad-dotted notation.
		 * @param flags compressed flags.
		 * @param bandwidth relay's bandwidth assigned in consensus file.
		 * @param version Tor version used by the ralay.
		 */
		Relay(size_t reg, const std::string& name, const std::string& fingerprint,
			const std::string& publishedDate, const std::string& stringIPAddress,
			int flags, int bandwidth, const std::string& version)
			: name(name), fingerprint(fingerprint), publishedDate(publishedDate),
			ipAddress(stringIPAddress), version(version), bandwidth(bandwidth),
			flags(flags), registeredAt(reg) { }

		// subclasses
		/**
		 * Auxulliary class for policy entry.
		 */
		struct PolicyDescriptor
		{
			bool isAccept; /**< Is this entry accept entry (false for reject entry)? */
			IP address; /**< Address mentioned in policy. */
			uint16_t portBegin; /**< The smallest port number this entry applies to. */
			uint16_t portEnd; /**< The biggest port number this entry applies to. */
			
			/**
			 * Initializes parameters.
			 * @param isAccept @copydoc Relay::PolicyDescriptor::isAccept
			 * @param address @copydoc Relay::PolicyDescriptor::address
			 * @param portBegin @copydoc Relay::PolicyDescriptor::portBegin
			 * @param portEnd @copydoc Relay::PolicyDescriptor::portEnd
			 */
			PolicyDescriptor(bool isAccept, IP address, int portBegin, int portEnd)
				: isAccept(isAccept), address(address), portBegin(portBegin), portEnd(portEnd) { };
		};
		
		//getters
		/**
		 * Checks whether relay has chosen flags set.
		 * @param testFlags tested set of flags.
		 * @return true iff relay has all specified flags.
		 */ 
		bool hasFlags(int testFlags) const
		{
			return (flags & testFlags) == testFlags;
		}
		/**
		 * @return set of flags.
		 */
		int getFlags() const { return flags; }
		/**
		 * @return name of the relay.
		 */
		const std::string& getName() const { return name; }
		/**
		 * @return relay's IP address.
		 */
		const IP& getAddress() const { return ipAddress; }
		/**
		 * @return date of publishing descriptors extracted from consensus file in YYYY-MM-DD hh:mm:ss format.
		 */
		const std::string& getPublishedDate() const { return publishedDate; }
		/**
		 * @return relay's fingerprint as string in hexadecimal format without leading 0x.
		 */
		const std::string& getFingerprint() const { return fingerprint; }
		/**
		 * @return vector of parsed policy entries.
		 */
		const std::vector<PolicyDescriptor>& getPolicy() const { return policy; }
		/**
		 * @return Tor version used by the relay.
		 */
		const std::string& getVersion() const { return version; }
		/**
		 * @return relay's bandwidth assigned in consensus file (kb/s, weight).
		 */
		int getBandwidth() const { return bandwidth; }
		/**
		 * @return relay's averaged observed bandwidth (B/s).
		 */
		int getAveragedBandwidth() const { return averagedBandwidth; }
		/**
		 * @ return the country relay's IP address is located in.
		 */
		const std::string& getCountry() const { return country; }
		/**
		 * @return relay's IP Autonomous System Number.
		 */
		const std::string& getASNumber() const { return ASNumber; }
		/**
		 * @return relay's IP Autonomous System Name.
		 */
		const std::string& getASName() const { return ASName; }
		/**
		 * @return relay's platform name.
		 */
		const std::string& getPlatform() const { return platform; }
		/**
		 * @return relay's IP latitude in degrees.
		 */
		float getLatitude() const { return latitude; }
		/**
		 * @return relay's IP longitude in degrees.
		 */
		float getLongitude() const { return longitude; }
		
		//setters
		/**
		 * @param name name for Tor network relay.
		 */
		void setName(const std::string& n) { this->name = n; }
		/**
		 * @param ipAddress IP address for Tor network relay.
		 */
		void setAddress(const IP& ip) { this->ipAddress = ip; }
		/**
		 * @param publishedDate date of publishing server descriptors for consensus file.
		 */
		void setPublishedDate(const std::string& pD) { this-> publishedDate = pD; }
		/**
		 * @param fingerprint fingerprint as string in hex format without leading 0x.
		 */
		void setFingerprint(const std::string& fingerprint) { this->fingerprint = fingerprint; }
		/**
		 * @param version Tor version used by the relay. 
		 */
		void setVersion(const std::string& version) { this->version = version; }
		/**
		 * @param bandwidth relay's bandwidth
		 */
		void setBandwidth(int bandwidth) { this->bandwidth = bandwidth; }
		/**
		 * @param bandwidth relay's averaged observed bandwidth
		 */
		void setAveragedBandwidth(int bandwidth) { this->averagedBandwidth = bandwidth; }
		/**
		 * Setting flags to 0.
		 */
		 void resetFlags() { flags = 0; }
		/**
		 * Sets relay flag.
		 * @param flag flag to be set.
		 * @param value flag value.
		 */
		void setFlag(RelayFlag flag, bool value = true)
		{
			flags = (~flag & flags) | (value ? flag : 0);
		}
		/**
		 * Set complete set of flags.
		 * @param flags new flags set
		 */
		void setFlags(int flags)
		{
			this->flags = flags;
		}
		/**
		 * @param badexit new value for "BadExit" flag.
		 */
		void setBadExit(bool badexit) { setFlag(RelayFlag::BAD_EXIT, badexit); }
		/**
		 * @param guard new value for "Guard" flag.
		 */
		void setGuard(bool guard) { setFlag(RelayFlag::GUARD, guard); }
		/**
		 * @param exit new value for "Exit" flag.
		 */
		void setExit(bool exit) { setFlag(RelayFlag::EXIT, exit); }
		/**
		 * @param valid new value for "Valid" flag.
		 */
		void setValid(bool valid) { setFlag(RelayFlag::VALID, valid); }
		/**
		 * @param stable new value for "Stable" flag.
		 */
		void setStable(bool stable) { setFlag(RelayFlag::STABLE, stable); }
		/**
		 * @param running new value for "Running" flag.
		 */
		void setRunning(bool running) { setFlag(RelayFlag::RUNNING, running); }
		/**
		 * @param fast new value for "Fast" flag.
		 */
		void setFast(bool fast) { setFlag(RelayFlag::FAST, fast); }
		/**
		 * @param country the country relay's IP address is located in.
		 */
		void setCountry(const std::string& country) { this->country = country; }
		/**
		 * @param ASNumber relay's IP Autonomous System Number.
		 */
		void setASNumber(const std::string& ASNumber) { this->ASNumber = ASNumber; }
		/**
		 * @param ASName relay's IP Autonomous System Name.
		 */
		void setASName(const std::string& ASName) { this->ASName = ASName; }
		/**
		 * @param platform Relay's platform.
		 */
		void setPlatform(const std::string& platform) { this->platform = platform; }
		/**
		 * @param latitude relay's IP latitude in degrees.
		 */
		void setLatitude(float latitude) { this->latitude = latitude; }
		/**
		 * @param longitude relay's IP longitude in degrees.
		 */
		void setLongitude(float longitude) { this->longitude = longitude; }		
		
		// functions
		/**
		 * Parses and adds new policy entry to policy entries vector
		 * @see getPolicy
		 * @param entry policy entry to be parsed and added.
		 */
		void addPolicy(const std::string& entry);
		/**
		 * @return IP subnet of the relay.
		 */
		uint32_t getSubnet() const { return ipAddress.getPrefix(); }

		/**
		 * Checks whether relay's policy allows for communication with IP address at port specified.
		 * @param ip tested IP address.
		 * @param port port number.
		 * @return true, if policy allows for communication with IP address, false otherwise.
		 */
		bool supportsConnection(const IP& ip, uint16_t port) const;
		
		/**
		 * @return relay's position in consensus. 
		 */
		size_t position() const{ return registeredAt; }
		
	private:
		// variables
		std::string name; /**< Name of the relay in Tor network. */
		IP ipAddress; /**< Relay's IP address. */
		std::string publishedDate; /**< Date of publishing descriptors referenced by consensus in YYYY-MM-DD hh:mm:ss format. */
		std::string fingerprint; /**< Relay's fingerprint in hexadecimal string format without leading 0x. */
		std::vector<PolicyDescriptor> policy; /**< Vector of relay's policy entries. */
		std::string version; /**< Version Tor version used by the relay. */	
		std::string country; /**< The country relay's IP address is located in. */
		std::string ASNumber; /**< Relay's IP Autonomous System Number. */
		std::string ASName; /**< Relay's IP Autonomous System Name. */
		std::string platform; /**< The name of the relay's platform. */
		int bandwidth; /**< Relay's bandwidth assigned in consensus file. */
		int averagedBandwidth = 0; /**< Relay's averaged observed bandwidth. */
		int flags; /**< Node's flags set, @see hasFlag(). */
		float latitude; /**< Relay's IP latitude in degrees. */
		float longitude; /**< Relay's IP longitude in degrees. */
		size_t registeredAt; /**< Position in relays vector in consensus. */ // for O(1) access

#ifdef USE_PYTHON
		friend class PythonBinder;
#endif
};

#endif
