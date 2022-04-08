#ifndef CONSENSUS_HPP
#define CONSENSUS_HPP

/** @file */

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

#include "relay.hpp"
#include "db_connection.hpp"
#include "utils.hpp"

#include "types/general_exception.hpp"
#include "types/symmetric_matrix.hpp"


/**
 * Consensus creating exception.
 */
class consensus_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			OPEN_FILE, /**< Opening consensus file failure. */
			INSUFFICIENT_FILE, /**< File has insufficient content. */
			INVALID_FORMAT /**< File is not valid consensus file. */
		};
		
		// constructors
		/**
		 * Constructs exception instance related with consensus file.
		 * @param why reason of throwing exception
		 * @param fileName consensus file name.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		consensus_exception(reason why, const std::string& fileName, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case OPEN_FILE:
					this->message = "could not open file: \"" + fileName + "\".";
					break;
				case INSUFFICIENT_FILE:
					this->message = "file \"" + fileName + "\" is not a valid consensus file (insufficient content).";
					break;
				default:
					this->message = "unknown reason for failing \"" + fileName + "\" parsing.";
			}
			commit_message();
		}

		/**
		 * Constructs exception instance related with consensus parsing error.
		 * @param why reason of throwing exception
		 * @param fileName consensus file name.
		 * @param fileLine line in file at which parsing error has occured.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		consensus_exception(reason why, const std::string& fileName, const std::string& fileLine, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case INVALID_FORMAT:
					this->message = "could not parse file: \"" + fileName + "\" at \"" + fileLine + "\".";
					break;
				default:
					this->message = "unknown reason for failing \"" + fileName + "\" parsing.";
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~consensus_exception() throw () { }
		
		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
	
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "consensus_exception";
			return tag;
		}
};

/**
 * @enum RelayRole
 * Possible roles of a relay in a circuit.
 */
enum class RelayRole
{
	ENTRY_ROLE, /**< Entry node role. */
	EXIT_ROLE, /**< Exit node role. */
	MIDDLE_ROLE /**< Middle node role. */
};

/**
 * Consensus class contains information about Tor network status based on
 * consensus file used and its date. It also takes into account server descriptors
 * and takes care of family relations between relays.
 */ 
class Consensus : public std::enable_shared_from_this<Consensus>
{
	public:
		// constructors
		/**
		 * Creates consensus from file specified and load additional relays information from given database.
		 * @param consensusFileName name of consensus file.
		 * @param DBFileName name of .sqlite database file.
		 * @param viaAllPairsFileName map of valid circuits (csv file).
		 */
		Consensus(const std::string& consensusFileName, const std::string& DBFileName, const std::string& viaAllPairsFileName, bool useViaRelays);
		
		/**
		 * Creates consensus from saved to binary file state.
		 * @param binaryFileName name of file containing saved binary consensus state.
		 */
		Consensus(const std::string& binaryFileName);

		Consensus(const Consensus& consensus)
			: relations(new SymmetricMatrix<bool>(*consensus.relations)), weightMods(consensus.weightMods), 
			maxModifier(consensus.maxModifier), validAfter(consensus.validAfter), 
			fingerprintMap(consensus.fingerprintMap), viaPairs(consensus.viaPairs), useViaRelays(consensus.useViaRelays), relays(consensus.relays) {}
		
		
		const std::vector<std::tuple<size_t,size_t>>& getPairsForVia(size_t via) const 
		{
			return viaPairs[via];
		}


		/**
		 * Returns relay at specified position in relays vector.
		 * @param relayPos position of a relay in relays vector.
		 */
		const Relay& getRelay(size_t relayPos) const
		{
			return relays[relayPos];
		}

		/**
		 * Returns relay at specified position in relays vector.
		 * @param relayPos position of a relay in relays vector.
		 */
		void forceSetRelayFlag(size_t relayPos, RelayFlag flag, bool v)
		{
			relays[relayPos].setFlag(flag, v);
		}

		/**
		 * Returns a vector of all relays in this consensus, in the correct order.
		 */
		const std::vector<Relay>& getRelays() const
		{
			return relays;
		}
		
		/**
		 * Returns weight modifier specified in consensus file
		 * for chosen relay role and flags.
		 * @param role relay's role.
		 * @param flags relay's flags set.
		 * @return weight modifier.
		 */
		weight_t getWeightModifier(RelayRole role, int flags) const
		{
			return weightMods.getModifier(role, flags);
		}
		
		/**
		 * @return maximal modifier that appeared in consensus file.
		 */
		weight_t getMaxModifier() const
		{
			return maxModifier;
		}
		
		/**
		 * Saves current consensus state to binary file.
		 * It can be loaded later with one of constructors.
		 * @param output output binary file.
		 */
		void saveBinary(const std::string& output) const;
		
		/**
		 * Checks whether relays are related. Relays are related if they are in the same subnet
		 * or they declare each other as family in descriptors.
		 * @param relay1 first relay of the pair to be tested.
		 * @param relay2 second relay of the pair to be tested.
		 */
		bool isRelated(size_t relay1, size_t relay2) const
		{
			return (*relations)[relay1][relay2];
		}
		
		/**
		 * @return number of relays registered in consensus.
		 */
		size_t getSize() const
		{
			return relays.size();
		}
		
		/**
		 * @return whether to use vias (bool)
		*/
		size_t useVias() const
		{
			return useViaRelays;
		}

		/**
		 * Find relay by fingerprint (if it exists in the relay vector)
		 * @param fingerprint
		 * @return nullptr if not found...
		 */
		const Relay* findRelayByFingerprint(const std::string& fingerprint) const
		{
			auto found = fingerprintMap.find(fingerprint);
			if(found == fingerprintMap.end())
				return nullptr;
			else
				return &relays[found->second];
		}

		/**
		 * Find index in the relay vector if it exists
		 * @param fingerprint
		 * @return -1 if not found
		 */
		const size_t findRelayIndexByFingerprint(const std::string& fingerprint) const
		{
			auto found = fingerprintMap.find(fingerprint);
			if(found == fingerprintMap.end())
				return -1;
			else
				return found->second;
		}


	private:
		// functions
		/**
		 * Helper function for parsing consensus file.
		 * Retrieves date, weight modifiers and node information.
		 * @param fileName name of the consensus file.
		 */
		void loadConsensusFile(const std::string& fileName);
		
		/**
		 * Helper function for loading additional relays information from database.
		 * Database contains descriptors information (such as platform, country or family relations).
		 * If no database exists or database seems to be corrupted, this function will also (re)create it.
		 * @param dbConnection database connection handle
		 */
		void loadRelaysDBInformation(const DBConnection& dbConnection);
		
		// variables
		std::vector<std::vector<std::tuple<size_t, size_t>>> viaPairs; /** vector of all valid circuits (G M X) with (V1 V2) vias (if present)**/
		std::unique_ptr<SymmetricMatrix<bool>> relations; /**< Matrix of relays relations. if (a,b) is true, then relay a is related to relay b. */
		std::vector<Relay> relays; /**< Vector of relays in Tor network described by consensus file. */
		std::map<std::string, size_t> fingerprintMap; /**< Maps relay's fingerprints to their position in relays vector. */
		std::string validAfter; /**< Consensus date declared in consensus file. */
		weight_t maxModifier; /**< Maximal modifier specified in consensus file. */
		bool useViaRelays; 


		struct WeightMods
		{
			// d: both exit and guard
			// g: guard
			// e: exit
			// m: not guard nor exit
			
			// exit modifiers
			weight_t wed, weg, wee, wem;
			// entry modifiers
			weight_t wgd, wgg, wgm;
			// middle modifiers
			weight_t wmd, wmg, wme, wmm;
			
			weight_t getModifier(RelayRole role, int flags) const
			{
				// take into account only guard and exit flags
				flags &= (RelayFlag::GUARD | RelayFlag::EXIT);
				
				switch(role)
				{
					case RelayRole::EXIT_ROLE:
						switch(flags)
						{
							case RelayFlag::GUARD | RelayFlag::EXIT:
								return wed;
							case RelayFlag::GUARD:
								return weg;
							case RelayFlag::EXIT:
								return wee;
							default:
								return wem;
						}
					case RelayRole::ENTRY_ROLE:
						switch(flags)
						{
							case RelayFlag::GUARD | RelayFlag::EXIT:
								return wgd;
							case RelayFlag::GUARD:
								return wgg;
							case RelayFlag::EXIT:
								return (weight_t)0;
							default:
								return wgm;
						}
					case RelayRole::MIDDLE_ROLE:
						switch(flags)
						{
							case RelayFlag::GUARD | RelayFlag::EXIT:
								return wmd;
							case RelayFlag::GUARD:
								return wmg;
							case RelayFlag::EXIT:
								return wme;
							default:
								return wmm;
						}
				}
			}		
		} weightMods; /**< Structure remembers weights modifiers specified in consensus and allows easy access to these values. */
};



/**
* [ShorTor] uses this class for parsing CSV files, specifically the valid circuits csv file
*/
class CSVRow
{
    public:
        std::string operator[](std::size_t index) const
        {
            return std::string(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
        }
        std::size_t size() const
        {
            return m_data.size() - 1;
        }
        void readNextRow(std::istream& str)
        {
            std::getline(str, m_line);

            m_data.clear();
            m_data.emplace_back(-1);
            std::string::size_type pos = 0;
            while((pos = m_line.find(',', pos)) != std::string::npos)
            {
                m_data.emplace_back(pos);
                ++pos;
            }
            // This checks for a trailing comma with no data after it.
            pos   = m_line.size();
            m_data.emplace_back(pos);
        }
    private:
        std::string         m_line;
        std::vector<int>    m_data;
};

#endif
