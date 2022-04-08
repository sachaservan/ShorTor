#ifndef PS_LASTOR_HPP
#define PS_LASTOR_HPP

/** @file */

#include <memory>
#include <vector>
#include <map>

#include "path_selection_standard.hpp"
#include "relay.hpp"
#include "consensus.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "utils.hpp"
#include "types/general_exception.hpp"

/**
 * LASTor initialization exception.
 */
class lastor_init_exception : public general_exception
{
		public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			INVALID_CELL_SIZE, /**< Invalid cell size value (should be integer s such that  1 <= s <= 50 and 900 % s = 0). */
			INVALID_ALPHA, /**< Invalid alpha value (should be floating point value v such that 0 <= v <= 1). */
			INVALID_SENDER_LAT, /**< Sender's geographical latitude out of bounds. */
			INVALID_SENDER_LONG, /**< Sender's geographical longitude out of bounds. */
			INVALID_RECIPIENT_LAT, /**< Recipient's geographical latitude out of bounds. */
			INVALID_RECIPIENT_LONG, /**< Recipient's geographical longitude out of bounds. */
			NO_ENTRY /**< No valid entry cluster within range. */
		};
		
		// constructors
		/**
		 * Constructs exception instance caused by invalid value.
		 * @param why reason of throwing exception
		 * @param value invalid value that caused exception.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		lastor_init_exception(reason why, double val, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case INVALID_CELL_SIZE:
					this->message = "Invalid cell size: \"" + std::to_string((int)(val + 0.5)) + "\". Should be value v such that  1 <= v <= 50 and 900 % v = 0.";
					break;
				case INVALID_ALPHA:
					this->message = "Invalid alpha value: \"" + std::to_string(val) + "\". Should be value v such that 0 <= v <= 1.";
					break;
				case INVALID_SENDER_LAT:
					this->message = "Invalid sender latitude: \"" + std::to_string(val);
					break;
				case INVALID_SENDER_LONG:
					this->message = "Invalid sender longitude: \"" + std::to_string(val);
					break;
				case INVALID_RECIPIENT_LAT:
					this->message = "Invalid recipient latitude: \"" + std::to_string(val);
					break;
				case INVALID_RECIPIENT_LONG:
					this->message = "Invalid recipient longitude: \"" + std::to_string(val);
					break;
				case NO_ENTRY:
					this->message = "No valid entry cluster within the range of " + std::to_string(val) + " kilometers. Select smaller alpha value.";
					break;
				default:
					this->message = "Unknown exception reason.";
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~lastor_init_exception() throw () { }
		
		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
	
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "lastor_init_exception";
			return tag;
		}
};

/**
 * Class for LASTor path selection utility.
 * LASTor is a path selection algorithm that minimizes the latencies in Tor communication by
 * reducing geographic length of the connection circuit - relays which lie within shorter circuits are more probable to be selected.
 * Circuit is defined as sender-entry-middle-exit-recipient path.
 * In order to avoid selecting relays which are very close to each other (for instance, which are run in the same building)
 * LASTor clusters relays which lie in the same cluster as tiles and does not allow to select multiple relays from the same cluster.
 * @see Tile
 */ 
class PSLASTor : public PathSelectionStandard
{
	friend class PSAS;
	
	public:
		// constructors
		/** Constructor extracts data from specification and consensus relays.
		 * For each relay it marks whether the relay may be used as exit, entry or middle relay.
		 * It recomputes relays selection probabilities based on the circuits lengths.
		 * @param pathSelectionSpec description of a path selection parameters and preferences.
		 * @param senderSpec description of a sender using this path selection.
		 * @param recipientSpec description of a recipient which sender connects to.
		 * @param relationshipManager relationship rules.
		 * @param consensus consensus describing the state of the Tor network.
		 */
		PSLASTor(
			std::shared_ptr<PSLASTorSpec> pathSelectionSpec, 
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus &consensus);
		
		// functions
		virtual probability_t exitProb(size_t exit) const;
		virtual probability_t entryProb(size_t entry, size_t exit) const;
		virtual probability_t middleProb(size_t middle, size_t entry, size_t exit) const;
		
		virtual bool entryExitAllowed(size_t entry, size_t exit) const;		
		virtual bool middleEntryExitAllowed(size_t middle, size_t entry, size_t exit) const;
	
	protected:
		// functions
		/**
		 * Computes cluster ID for specified coordinates.
		 * ID packs information about converted (unsigned integer) coordinates at which cluster starts.
		 * Cluster spans area of cellSize x cellSize, excluding highest coordinates (at two of cell sides) which are assigned to the next cell. 
		 * @param latitude point latitude 
		 * @param longitude point longitude
		 */ 
		uint32_t getClusterID(double latitude, double longitude) const;



		// subclasses
		/**
		 * Structure containing information about a cluster.
		 */
		struct Cluster
		{
			uint32_t id; /**< Cluster ID (based on latitude and longitude). */
			size_t index; /**< Cluster's index. */
			std::vector<size_t> relays; /**< Indices of the relays assigned to this cluster. */
			int exitCount; /**< Number of exit nodes in cluster. */
			int entryCount; /**< Number of entry nodes in cluster. */
			int middleCount; /**< Number of middle nodes in cluster. */
			double centerLat; /**< Precomputed cluster's center latitude. */
			double centerLong; /**< Precomputed cluster's center longitude. */
			
			/**
			 * Constructor by default pass the information that cluster contains no possible exit, entry or middle nodes. 
			 * It also computes center latitude and longitude of the cluster.
			 * @param id cluster's ID
			 * @param cellS size of the cell in tenths of degrees
			 */
			Cluster(uint32_t id, double cellS) : exitCount(0), entryCount(0), middleCount(0), id(id)
			{
				centerLat = (((double)(id >> 16)) - 900) / 10 + (double)cellS / 20.;
				centerLong = (((double)(id & 0x0fff)) - 1800) / 10 + (double)cellS / 20.;
			}
		};
		
		/**
		 * Structure containing information about clusters containing entry nodes.
		 * All computation is done for an exit clusted this structure is nested into.
		 * @see ExitCluster
		 */
		 struct EntryCluster
		 {
			 probability_t entryProb; /**< Probability for selecting this tile as entry for a fixed exit. */
			 std::vector<probability_t> middleProbs; /**< Vector for probabilities of selecting this tile as middle for fixed entry and exit. */
			 /**
			  * Initializes probability to 0.
			  */
			 EntryCluster() : entryProb(0) { }
		 };
		
		/**
		 * Structure containing information about all clusters containing possible exit node.
		 */
		 struct ExitCluster
		 {
			 probability_t exitProb; /**< Probability for selecting this tile as exit. */
			 std::vector<EntryCluster> entries; /**< Vector for entry probabilities and middles related to this exit. */
			 /**
			  * Initializes probability to 0.
			  */
			 ExitCluster() : exitProb(0) { }
		 };	
		
		// variables
		double alpha; /**< Alpha coefficient; minimal value: 0 for lowest latency, maximal: 1 for highest relays selection entropy. */ 
		int cellSize; /**< Size of cell in tenths of degrees (represents length of cell side). */ 
		
		std::vector<size_t> clusterOf; /**< Vector mapping each relay's position in consensus to cluster index. */
		std::vector<ExitCluster> exitClusters; /**< Vector of exit clusters (clusters with at least one relay which can be selected as an exit node). */
		/**
		 * Clusters A, B are related (clusterRelations[A][B] == true) iff
		 * There exist relays a from A and b from B, s.t. a is related to b (based on relationship manager) when a is exit and b is entry.
		 */
		std::vector<std::vector<bool>> clusterASRelations;
		/**
		 * Clusters A, B are related (clusterRelations[A][B] == true) iff
		 * There exist relays a from A and b from B, s.t. a is related to b (based on relationship manager) when either a or b is middle.
		 */
		std::vector<std::vector<bool>> clusterRelations;
};

#endif
