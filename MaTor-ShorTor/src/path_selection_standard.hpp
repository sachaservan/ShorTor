#ifndef PATH_SELECTION_STANDARD_H
#define PATH_SELECTION_STANDARD_H

/** @file */

#include <memory>
#include <set>

#include "path_selection.hpp"
#include "relay.hpp"
#include "consensus.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "path_selection_spec.hpp"

/**
 * Virtual class for path selection algorithm using fields from standard specification
 * (such as long lived ports, guards and some flags checks).
 */ 
class PathSelectionStandard : public PathSelection
{
	public:
		// constructor
		/**
		 * @copydoc PathSelection::PathSelection()
		 * @param pathSelectionSpec specification of path selection (for stable flags requirement check)
		 */
		PathSelectionStandard(
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<StandardSpec> pathSelectionSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus& consensus) :
				PathSelection(senderSpec, recipientSpec, relationshipManager, consensus)
		{
			requireStable = pathSelectionSpec->requireStableFlags | LLPRequiresStable(pathSelectionSpec->longLivedPorts); 
		}
	
	protected:
		// functions
		/**
		 * Checks whether recipient server's ports overlap sender's long lived ports set.
		 * @param llports sender's long lived ports set.
		 * @return true, if any of recipient's ports is on sender's long lived ports list.
		 */
		virtual bool LLPRequiresStable(const std::set<uint16_t>& llports) const;
		/**
		 * Checks which relays may be used as exit, entry or middle nodes (based on specification) and saves result for each tested relay.
		 * Computes cardinality of a maximal set of recipient's ports that is suported by a single relay.
		 * A relay supports port if its accept policy list contains it and its reject policy does not contain it.
		 * All relays which support set of ports of size smaller than this cardinality, are disallowed to be exit nodes.
		 * @param psSpec path selection specification
		 * @param useGuards should guards be taken into account? (default true)
		 */
		virtual void computeRelaysPossibilities(std::shared_ptr<StandardSpec> psSpec, bool useGuards = true);
		/**
		 * Checks if a relay can be an exit node (based on specification).
		 * @param relay tested relay
		 * @param requiredFlags flags required for exit role
		 * @param guards parsed guards set
		 * @return true iff relay can be an exit in this scenario.
		 */
		virtual bool canBeExit(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const;
		/**
		 * Checks if a relay can be a middle node (based on specification).
		 * @param relay tested relay
		 * @param requiredFlags flags required for entry role
		 * @param guards parsed guards set
		 * @return true iff relay can be a middle node in this scenario.
		 */
		virtual bool canBeEntry(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const;
		/**
		 * Checks if a relay can be an entry node (based on specification).
		 * @param relay tested relay
		 * @param requiredFlags flags required for middle role
		 * @param guards parsed guards set
		 * @return true iff relay can be an entry in this scenario.
		 */
		virtual bool canBeMiddle(const Relay& relay, int requiredFlags, const std::vector<int>& guards) const;	
		/**
		 * @param guards guards listed in specification
		 * @return guards positions in consensus.
		 */
		virtual std::vector<int> translateGuards(const std::set<std::string>& guards) const;
		
		// variables
		bool requireStable; /**< Since path selection may require stable connection due to it's properties, it keeps it's own flag for this requirement. */
};

#endif
