#ifndef CONFIG_HPP
#define CONFIG_HPP

/** @file */

#include <string>

static std::string emptystring("");

/**
 * Class for configuration for MATor behaviour and scenario independent configuration.
 */ 
class Config : public std::enable_shared_from_this<Config>
{
	public:
		// variables
		bool precompute = false; /**< Indicates whether generic anonymity upper bound should be computed before calling any of get*Anonymity() functions. */
		double epsilon = 1; /**< Multiplicative factor used in computations. */
		bool fast = false; /**< Indicates whether MATor should be run in fast mode (may affect result precision). */
		bool useVias = false; /**< Indicates whether MATor should select vias for circtuits (if available). */
		std::string consensusFile; /**< Consensus file name. */
		std::string databaseFile; /**< Database file name. */
		std::string viaAllPairsFile; /**< CSV file containing all valid circuits. */

		Config(std::string& consensusFile, std::string& databaseFile = emptystring, std::string& viaAllPairsFile = emptystring, bool useVias = false, bool fast = false, bool precompute = false, double epsilon = 1)
			: consensusFile(consensusFile), databaseFile(databaseFile), viaAllPairsFile(viaAllPairsFile), useVias(useVias), fast(fast), precompute(precompute), epsilon(epsilon){}
		Config(){}
};

#endif
