#ifndef PS_TOR_HPP
#define PS_TOR_HPP

/** @file */

#include <memory>

#include "tor_like.hpp"
#include "relay.hpp"
#include "utils.hpp"

/**
 * Class for utility of vanilla Tor path selection algorithm. 
 * The algorithm analyses relays' flags and bandwidths, and assigns exit, entry and middle weights to the relays.
 * For the relays with the same flags, the bigger bandwidth value is, the higher weight is assigned.
 * Flags influence weight computed from bandwidth basing on modificators defined in consensus file.
 * The higher relay weight, the bigger probability that it will be selected for a specified role.
 */ 
class PSTor : public TorLike
{	
	public:
		// constructors
		/**
		 * Constructor calls parent classes constructors. Then, it computes possibilities of selecting
		 * a relay for specified role and assigns weights for relays.
		 * @param pathSelectionSpec specification of Tor path selection preferences.
		 * @param senderSpec description of a sender using this path selection.
		 * @param recipientSpec description of a recipient which sender connects to.
		 * @param relationshipManager definition of relations between relays.
		 * @param consensus consensus describing the state of the Tor network.
		 * @see PathSelection
		 * @see TorLike
		 */
		PSTor(std::shared_ptr<PSTorSpec> pathSelectionSpec, 
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus);

	private:
		weight_t biasMiddleSelectionProbabilitiesDueToVias(const Consensus& consensus, weight_t exitWeightSum, weight_t entryWeightSum, weight_t iddleWeightSum);

};

#endif
