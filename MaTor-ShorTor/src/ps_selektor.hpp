#ifndef PS_SELEKTOR_HPP
#define PS_SELEKTOR_HPP

/** @file */

#include <memory>

#include "tor_like.hpp"
#include "relay.hpp"
#include "utils.hpp"

/**
 * Class for the SelekTOR variant of Tors path selection algorithm. 
 * The algorithm assigns weight 0 to all exit relays that are outside of the exit country specified in the constructor (typically the US).
 */ 
class PSSelektor : public TorLike
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
		PSSelektor(std::shared_ptr<PSSelektorSpec> pathSelectionSpec,
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus);
	private:
		std::string ExitCountry;
};

#endif
