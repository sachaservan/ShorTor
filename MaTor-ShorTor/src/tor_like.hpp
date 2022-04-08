#ifndef TOR_LIKE_HPP
#define TOR_LIKE_HPP

/** @file */

#include <memory>
#include <set>

#include "path_selection_standard.hpp"
#include "relay.hpp"
#include "consensus.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "path_selection_spec.hpp"
#include "utils.hpp"
#include "types/symmetric_matrix.hpp"
#include "types/work_manager.hpp"

/**
 * Virtual class for path selection algorithms which compute probabilities based on weights assigned to relays.
 */ 
class TorLike : public PathSelectionStandard
{
	friend class PSAS;
	
	public:
		// constructor
		/**
		 * @copydoc PathSelectionStandard::PathSelectionStandard()
		 */
		TorLike(
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<TorLikeSpec> pathSelectionSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus) :
				PathSelectionStandard(senderSpec, recipientSpec, pathSelectionSpec, relationshipManager, consensus),
				middleSumRelatedInv(consensus.getSize(), false),
				exitWeights(consensus.getSize()),
				entryWeights(consensus.getSize()),
				middleWeights(consensus.getSize()),
				entrySumRelatedInv(consensus.getSize()) { }
		
		// functions
		virtual probability_t exitProb(size_t exit) const;
		virtual probability_t entryProb(size_t entry, size_t exit) const;
		virtual probability_t middleProb(size_t middle, size_t entry, size_t exit) const;
		
	protected:
		// functions
		/** 
		 * Computes relay weight based on relay's flags for exit node role.
		 * @param relay the relay which weight is computed
		 * @return exit weight of the relay
		 */
		virtual weight_t getExitWeight(const Relay& relay) const;
		/** 
		 * Computes relay weight based on relay's flags for entry relay role.
		 * @param relay the relay which weight is computed
		 * @return entry weight of the relay
		 */
		virtual weight_t getEntryWeight(const Relay& relay) const;
		/** 
		 * Computes relay weight based on relay's flags for middle relay role.
		 * @param relay the relay which weight is computed
		 * @return middle weight of the relay
		 */
		virtual weight_t getMiddleWeight(const Relay& relay) const;		
		/**
		 * Computes related weights(entry-, middle- related inverses).
		 * @param entryWeightSum total entry weight
		 * @param middleWeightSum total middle weight
		 */
		virtual void assignRelatedWeight(weight_t entryWeightSum, weight_t middleWeightSum);
				
		/**
		 * Computes related weights(entry-, middle- related inverses) in chunks (to enable multithreaded computation).
		 * @param start chunk start index
		 * @param stop chunk end index
		 * @param entryWeightSum sum of path selection entry weight
		 * @param middleWeightSum sum of path selection middle weight
		 * @param exitEntryRelatives sets of relatives indices in relation exit-entry
		 * @param exitMiddleRelatives sets of relatives indices in relation exit-middle
		 * @param entryMiddleRelatives sets of relatives indices in relation entry-middle
		 */
		virtual void assignRelatedWeightChunk(size_t start, size_t stop, weight_t entryWeightSum, weight_t middleWeightSum,
			std::vector<std::set<size_t>> exitEntryRelatives, std::vector<std::set<size_t>> exitMiddleRelatives, std::vector<std::set<size_t>> entryMiddleRelatives);	
		
		// variables 
		weight_t exitSumInv; /**< 1 / Total sum of all relay's exit weight. */
		std::vector<weight_t> entrySumRelatedInv; /**< 1 / (entry sum - related entry bandwidth) for a relay at corresponding position. */
		SymmetricMatrix<weight_t> middleSumRelatedInv; /**< 1 / (middle sum - related middle bandwidth) for a pair [i][j] of relays' positions. */
		
		std::vector<weight_t> exitWeights; /**< Exit weights of relays on positions corresponding to consensus positions. */
		std::vector<weight_t> entryWeights; /**< Entry weights of relays on positions corresponding to consensus positions. */
		std::vector<weight_t> middleWeights; /**< Middle weights of relays on positions corresponding to consensus positions. */
};

#endif

