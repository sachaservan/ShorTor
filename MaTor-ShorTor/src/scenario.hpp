#ifndef SCENARIO_HPP
#define SCENARIO_HPP

/** @file */

#include <memory>

#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "path_selection_spec.hpp"
#include "path_selection.hpp"
#include "consensus.hpp"

/**
 * Factory for classes inheriting from PathSelection.
 * @see PathSelection
 */ 
class Scenario
{
	public:
		// functions
		/**
		 * Creates path selection from specification given (sender connects to recipient using path selection in network described by consensus).
		 * @param pathSelectionSpec description of a path selection for this scenario.
		 * @param senderSpec description of a sender (client) connecting to recipient in this scenario.
		 * @param recipientSpec description of a recipient (server) which sender connects to in this scenario.
		 * @param consensus consensus for Path Selection instance.
		 */
		static std::shared_ptr<PathSelection> makePathSelection(
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec,
			std::shared_ptr<SenderSpec> senderSpec,
			std::shared_ptr<RecipientSpec> recipientSpec,
			const Consensus& consensus);
};

#endif
