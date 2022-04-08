#ifndef PATH_SELECTION_SPEC_HPP
#define PATH_SELECTION_SPEC_HPP

/** @file */

#include <set>
#include <string>

/**
 * @enum PathSelectionType
 * Supported path selection algorithms.
 */
enum PathSelectionType 
{ 
	PS_TOR, /**< Vanilla Tor path selection algorithm. */
	PS_DISTRIBUTOR, /**< DistribuTor path selection algorithm. */
	PS_LASTOR, /**< LASTor path selection algorithm. */
	PS_UNIFORM, /**< Uniform Tor path selection algorithm. */
	PS_SELEKTOR, /**< SelekTOR path selection algorithm. */
	PS_AS_TOR, /**< Autonomous System aware version of vanilla Tor path selection algorithm. */
	PS_AS_DISTRIBUTOR, /**< Autonomous System aware version of DistribuTor path selection algorithm. */
	PS_AS_LASTOR, /**< Autonomous System aware version of LASTor path selection algorithm. */
	PS_AS_UNIFORM, /**< Autonomous System aware version of uniform Tor path selection algorithm. */
	PS_AS_SELEKTOR /**< Autonomous System aware version of SelekTOR. */
};

/**
 * Virtual class for specification of a path selection algorithm used by Tor client.
 */ 
class PathSelectionSpec// : public std::enable_shared_from_this<PathSelectionSpec>
{
	public:
		/**
		 * @return path selection type.
		 */
		virtual PathSelectionType getType() { return type; }
	protected:
		PathSelectionType type; /**< Type of a path selection. */
};

/**
 * Virtual class for standard path selection specification.
 * The class contains variables used by most of the "standard" algorithms.
 * By default, non valid relays are forbidden. Exit flag is required for exit role.
 * Fast and stable flags are not required.
 * Hardcoded guards set is empty.
 * Long lived ports are: 21, 22, 706, 1863, 5050, 5190, 5222, 5223, 6523, 6667, 6697, 8300
 */
class StandardSpec : public PathSelectionSpec
{
	public:
		bool allowNonValidEntry { false }; /**< Indicates whether nodes without "valid" flag can be used as entry node. */
		bool allowNonValidMiddle { false }; /**< Indicates whether nodes without "valid" flag can be used as middle node. */
		bool allowNonValidExit { false }; /**< Indicates whether nodes without "valid" flag can be used as exit node. */
		bool requireExitFlag { true }; /**< Indicates whether "exit" flag is required for exit node. */
		bool requireFastFlags { false }; /**< Indicates whether "fast" flag is required for all nodes in a circuit. */
		bool requireStableFlags { false }; /**< Indicates whether "stable" flag is required for all nodes in a circuit. */
		std::set<std::string> guards; /**< Set of guards (format: name\@ip_address). */
		std::set<uint16_t> longLivedPorts { 21, 22, 706, 1863, 5050, 5190, 5222, 5223, 6523, 6667, 6697, 8300 }; /**< Set of numbers of long lived ports (if exit node supports any of long lived ports, path selection requires all nodes in a circuit to have "stable" flag. */
};

/**
 * Virtual class for specification of Tor-like (based on nodes bandwidths) path selections.
 */
class TorLikeSpec : public StandardSpec { };

/**
 * Specification of DistribuTor path selection algorithm.
 */
class PSDistribuTorSpec : public TorLikeSpec, public std::enable_shared_from_this<PSDistribuTorSpec>
{
	public:
		/**
		 * Sets path selection type to PSDistribuTor type.
		 * By default, AS unaware algorithm is used.
		 * @param bandwidthPerc The percentage of total exit bandwidth that DistribuTor preserves (should be a value from 0 to 1).
		 * @param asAware should path selection be AS aware?
		 */
		PSDistribuTorSpec(double bandwidthPerc = 1, bool asAware = false) : bandwidthPerc(bandwidthPerc) { type = asAware ? PS_AS_DISTRIBUTOR : PS_DISTRIBUTOR; }
		
		double bandwidthPerc; /**< The percentage of total exit bandwidth that DistribuTor preserves (should be a value from 0 to 1). */
};

/**
 * Specification of vanilla Tor path selection algorithm.
 */
class PSTorSpec : public TorLikeSpec, public std::enable_shared_from_this<PSTorSpec>
{
	public:
		/**
		 * Sets path selection type to PSTor type.
		 * By default, AS unaware algorithm is used.
		 * @param asAware should path selection be AS aware?
		 */
		PSTorSpec(bool asAware = false) { type = asAware ? PS_AS_TOR : PS_TOR; }
};

/**
* Specification of uniform Tor path selection algorithm.
*/
class PSUniformSpec : public TorLikeSpec, public std::enable_shared_from_this<PSUniformSpec>
{
public:
	/**
	* Sets path selection type to PSUniform type.
	* By default, AS unaware algorithm is used.
	* @param asAware should path selection be AS aware?
	*/
	PSUniformSpec(bool asAware = false) { type = asAware ? PS_AS_UNIFORM : PS_UNIFORM; }
};

/**
* Specification for SelekTOR.
*/
class PSSelektorSpec : public TorLikeSpec, public std::enable_shared_from_this<PSSelektorSpec>
{
public:
	/**
	* Sets path selection type to PSSelektor type.
	* By default, AS unaware algorithm is used.
	* @param asAware should path selection be AS aware?
	*/
	PSSelektorSpec(std::string tCountry = "US", bool asAware = false) : targetCountry(tCountry){ type = asAware ? PS_AS_SELEKTOR : PS_SELEKTOR;}

	std::string targetCountry;
};

/**
 * Specification of LASTor path selection algorithm.
 */
class PSLASTorSpec : public StandardSpec, public std::enable_shared_from_this<PSLASTorSpec>
{
	public:
		/**
		 * Sets path selection type to LASTor type.
		 * By default, AS unaware algorithm is used.
		 * @param alpha alpha coefficient, minimal: 0 for lowest latency, maximal: 1 for highest relays selection entropy (0 as default)
		 * @param cellSize size of cluster in tenths of degrees (10 as default, it represents 1x1 degree clusters)
		 * @param asAware should path selection be AS aware?
		 */
		PSLASTorSpec(double alpha = 0, int cellSize = 10, bool asAware = false) : alpha(alpha), cellSize(cellSize) { type = asAware ? PS_AS_LASTOR : PS_LASTOR; }
		
		double alpha; /**< Alpha coefficient; minimal value: 0 for lowest latency, maximal: 1 for highest relays selection entropy. */ 
		int cellSize; /**< Size of cell in tenths of degrees (represents length of cell side); min: 1, max: 50. Only values that split Earth to equal clusters are allowed.*/
};

#endif
