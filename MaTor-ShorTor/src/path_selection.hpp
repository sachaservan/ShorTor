#ifndef PATH_SELECTION_HPP
#define PATH_SELECTION_HPP

/** @file */

#include <memory>
#include <vector>

#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "path_selection_spec.hpp"
#include "consensus.hpp"
#include "utils.hpp"
#include "relationship_manager.hpp"

/**
 * This virtual class allows to obtain probabilities for selecting relays (for exit, entry or middle node)
 * in a specified scenario. Probabilities may be based on path selection type, path selection parameters,
 * sender parameters, recipient parameters and Tor network state.
 */ 
class PathSelection
{
	friend class PSAS;
	
	public:
		// constructors
		/** 
		 * Constructor saves consensus, sender specification and recipient specification within the class.
		 * @param senderSpec description of a sender using this path selection.
		 * @param recipientSpec description of a recipient which sender connects to.
		 * @param relationshipManager definitions of relays' relations.
		 * @param consensus consensus describing the state of the Tor network.
		 */
		PathSelection(
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus)
				: consensus(consensus), sender(senderSpec), recipient(recipientSpec), relations(relationshipManager) { };
		
		// functions
		/**
		 * Checks whether the first relay is allowed to serve as an entry node and the second relay is allowed to serve as an exit node simultaneously.
		 * By default, related nodes cannot be used.
		 * @param entry index of relay to be used as entry node.
		 * @param exit index of relay to be used as exit node.
		 * @return is pair allowed to simultaneously serve as entry and exit nodes respectively?
		 */
		virtual bool entryExitAllowed(size_t entry, size_t exit) const
		{
			return !relations->exitEntryRelated(exit, entry);
		}
		
		/**
		 * Checks whether the first relay is allowed to serve as a middle node, the second relay is allowed to serve as an entry node,
		 * and the third relay is allowed to serve as an exit node simultaneously.
		 * By default, relays are disallowed if exit or entry is related to middle.
		 * @param middle index of relay to be used as middle node.
		 * @param entry index of relay to be used as entry node.
		 * @param exit index of relay to be used as exit node.
		 * @return are these relays allowed to simultaneously serve as middle, entry and exit nodes respectively?
		 */
		virtual bool middleEntryExitAllowed(size_t middle, size_t entry, size_t exit) const
		{
			return !(relations->entryMiddleRelated(entry, middle) || relations->exitMiddleRelated(exit, middle));
		}
		/**
		 * Computes probability that a relay may be used as exit node.
		 * @param exit index of exit node candidate
		 * @return probability of selecting a relay as an exit node.
		 */
		virtual probability_t exitProb(size_t exit) const = 0;

		/**
		 * Computes probability that a relay may be used as entry node under condition that a specified relay was used as an exit node.
		 * @param entry index of entry node candidate index
		 * @param exit index of relay used as an exit node 
		 * @return probability of selecting a relay as an entry node.
		 */
		virtual probability_t entryProb(size_t entry, size_t exit) const = 0;
		/**
		 * Computes probability that a relay may be used as entry node under condition that specified relays are used as entry and exit nodes.
		 * @param middle index of middle node candidate
		 * @param entry index of relay used as an entry node
		 * @param exit index of relay used as an exit node
		 * @return probability of selecting a relay as a middle node
		 */
		virtual probability_t middleProb(size_t middle, size_t entry, size_t exit) const = 0;
		
	protected:
		// variables
		const Consensus &consensus; /**< Consensus describing state of the Tor network. */
		std::vector<bool> exitPossible; /**< Vector keeps possibilities for a relay to be an exit node on position corresponding to position in consensus. */
		std::vector<bool> entryPossible; /**< Vector keeps possibilities for a relay to be an entry node on position corresponding to position in consensus. */
		std::vector<bool> middlePossible; /**< Vector keeps possibilities for a relay to be a middle node on position corresponding to position in consensus. */
		std::vector<bool> middlePossibleBecauseVia; /**< override in case middle is a via relay. */
		std::shared_ptr<SenderSpec> sender; /**< Sender who uses this instance of path selection. */ 
		std::shared_ptr<RecipientSpec> recipient; /**< Recipient to whom the sender connects to using this instance of path selection. */
		std::shared_ptr<RelationshipManager> relations; /**< Definitions of relations between relays. */
};

#endif
