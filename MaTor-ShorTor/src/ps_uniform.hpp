#ifndef PS_UNIFORM_HPP
#define PS_UNIFORM_HPP

/** @file */

#include <memory>

#include "tor_like.hpp"
#include "relay.hpp"
#include "utils.hpp"

/**
 * Class for uniform Tor path selection algorithm. 
 * The algorithm analyses relays' flags and bandwidths, and assigns the weight 1 to all viable relays.
 */ 
class PSUniform : public TorLike
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
		PSUniform(std::shared_ptr<PSUniformSpec> pathSelectionSpec,
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus);
};

#endif
