#ifndef MATOR_HPP
#define MATOR_HPP

/** @file */

#include <memory>
#include <string>
#include <functional>

#include "adversary.hpp"
#include "types/general_exception.hpp"
#include "config.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "scenario.hpp"
#include "generic_worst_case_anonymity.hpp"
#include "generic_precise_anonymity.hpp"


class ASMap;
class Consensus;
class PathSelectionSpec;
class PathSelection;
class Relay;



/**
 * Thrown if an error in MATor initialization appears.
 */
class mator_init_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			PLACEHOLDER /**< Temporary placeholder for reason enum. */
		};

		// constructors
		/**
		 * Constructs exception instance.
		 * @param why reason of throwing exception
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		mator_init_exception(reason why, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				default:
					this->message = "unknown reason.";
			}
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~mator_init_exception() throw () { }

		virtual int why() const { return reasonWhy; }

	protected:
		reason reasonWhy; /**< Exception reason. */

		virtual const std::string& getTag() const
		{
			const static std::string tag = "mator_init_exception";
			return tag;
		}
};

/**
 * Main class for MATor functionalities.
 * //TODO:: some nice, exhaustive description
 */
class MATor : public std::enable_shared_from_this<MATor>
{
	public:
		// constructors
		/**
		 * Constructor initializes MATor class with specified parameters.
		 * @param senderSpec1 specification of sender A.
		 * @param senderSpec2 specification of sender B.
		 * @param recipientSpec1 specification of recipient 1.
		 * @param recipientSpec2 specification of recipient 2.
		 * @param pathSelectionSpec1 specification of path selection used by sender A.
		 * @param pathSelectionSpec2 specification of path selection used by sender B.
		 * @param config other configuration container.
		 */
		MATor(
			std::shared_ptr<SenderSpec> senderSpec1,
			std::shared_ptr<SenderSpec> senderSpec2,
			std::shared_ptr<RecipientSpec> recipientSpec1,
			std::shared_ptr<RecipientSpec> recipientSpec2,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec1,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec2,
			Config config);

		MATor(
			std::shared_ptr<SenderSpec> senderSpec1,
			std::shared_ptr<SenderSpec> senderSpec2,
			std::shared_ptr<RecipientSpec> recipientSpec1,
			std::shared_ptr<RecipientSpec> recipientSpec2,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec1,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec2,
			std::string& consensus, std::string& database = emptystring, std::string& viaAllPairsFilename = emptystring, bool useVias = false, bool fast = false)
			: MATor(senderSpec1, senderSpec2, recipientSpec1, recipientSpec2, pathSelectionSpec1, pathSelectionSpec2, Config(consensus, database, viaAllPairsFilename, useVias, fast)){}

		MATor(
			std::shared_ptr<SenderSpec> senderSpec1,
			std::shared_ptr<SenderSpec> senderSpec2,
			std::shared_ptr<RecipientSpec> recipientSpec1,
			std::shared_ptr<RecipientSpec> recipientSpec2,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec1,
			std::shared_ptr<PathSelectionSpec> pathSelectionSpec2,
			std::shared_ptr<Consensus> consensus, bool fast = false);


		/**
		 * Constructor initializes MATor class with parameters provided in config file.
		 * If any specification is missing in config file,
		 * MATor won't start computation before it is provided with setter.
		 * @param configJSONfile config file in JSON format.
		 */
		MATor(const std::string& configJSONfile);

		/**
		 * Constructor for MATor instance with unset specification.
		 * Requires feeding with specifications and consensus before starting anonymity computation.
		 */
		MATor()
		{
			computeFlags = 15; // 0b1111
			epsilon = 0.;
		}

		// destructor
		~MATor();

		// setters
		/**
		 * Sets pointer for sender A specification.
		 * @param senderSpec pointer to new sender A specification.
		 */
		void setSenderSpecA(std::shared_ptr<SenderSpec> senderSpec)
		{
			computeFlags |= 3; // | 0b0011
			senderSpec1 = senderSpec;
		}
		/**
		 * Sets pointer for sender B specification.
		 * @param senderSpec pointer to new sender B specification.
		 */
		void setSenderSpecB(std::shared_ptr<SenderSpec> senderSpec)
		{
			computeFlags |= 12; // | 0b1100
			senderSpec2 = senderSpec;
		}
		/**
		 * Sets pointer for specification of path selection used by sender A.
		 * @param pathSelectionSpec pointer to new path selection specification.
		 */
		void setPathSelectionSpec1(std::shared_ptr<PathSelectionSpec> pathSelectionSpec)
		{
			computeFlags |= 3; // | 0b0011
			pathSelectionSpec1 = pathSelectionSpec;
		}
		/**
		 * Sets pointer for specification of path selection used by sender B.
		 * @param pathSelectionSpec pointer to new path selection specification.
		 */
		void setPathSelectionSpec2(std::shared_ptr<PathSelectionSpec> pathSelectionSpec)
		{
			computeFlags |= 12; // | 0b1100
			pathSelectionSpec2 = pathSelectionSpec;
		}
		/**
		 * Sets pointer for recipient 1 specification.
		 * @param recipientSpec pointer to new recipient 1 specification.
		 */
		void setRecipientSpec1(std::shared_ptr<RecipientSpec> recipientSpec)
		{
			computeFlags |= 5; // | 0b0101
			recipientSpec1 = recipientSpec;
		}
		/**
		 * Sets pointer for recipient 2 specification.
		 * @param recipientSpec pointer to new recipient 2 specification.
		 */
		void setRecipientSpec2(std::shared_ptr<RecipientSpec> recipientSpec)
		{
			computeFlags |= 10; // | 0b1010
			recipientSpec2 = recipientSpec;
		}
		/**
		 * Sets pointer to computed consensus instance that should be used by MATor.
		 * @param consensus pointer to new consensus instance.
		 */
		void setConsensus(std::shared_ptr<Consensus> consensus)
		{
			computeFlags |= 15; // | 0b1111
			this->consensus = consensus;
		}


		void addMicrodesc(std::string& microdesc) {
			computeFlags |= 15;
			NOT_IMPLEMENTED;
		}

		void addMicrodescFile(std::string& filename) {
			computeFlags |= 15;
			NOT_IMPLEMENTED;
		}

		// functions
		/**
		 * Computes worst case anonymity guarantees for sender anonymity.
		 * If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		 * @see GenericWorstCaseAnonymity.senderAnonymity()
		 * @return upper bound probability of sender de-anonymization in two-senders game.
		 */
		double getSenderAnonymity();
		/**
		 * Computes worst case anonymity guarantees for recipient anonymity.
		 * If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		 * @see GenericWorstCaseAnonymity.recipientAnonymity()
		 * @return upper bound probability of recipient de-anonymization in two-recipients game.
		 */
		double getRecipientAnonymity();
		/**
		 * Computes worst case anonymity guarantees for relationship anonymity.
		 * If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		 * @see GenericWorstCaseAnonymity.relationshipAnonymity()
		 * @return upper bound probability of sender-recipient pair de-anonymization in two-senders two-recipients game.
		 */
		double getRelationshipAnonymity();

		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		*/
		void getGreedyListForSenderAnonymity(std::vector<size_t>& output);

		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		*/
		void getGreedyListForRecipientAnonymity(std::vector<size_t>& output);

		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		*/
		void getGreedyListForRelationshipAnonymity(std::vector<size_t>& output);


		/**
		* Computes precise anonymity guarantees for sender anonymity.
		* If the precise guarantees have not been computed so far, they are calculated.
		* @see GenericPreciseAnonymity.senderAnonymity()
		*/
		double getPreciseSenderAnonymity();
		/**
		* Computes precise anonymity guarantees for recipient anonymity.
		* If the precise guarantees have not been computed so far, they are calculated.
		* @see GenericPreciseAnonymity.senderAnonymity()
		*/
		double getPreciseRecipientAnonymity();
		/**
		* Computes precise anonymity guarantees for relationship anonymity.
		* If the precise guarantees have not been computed so far, they are calculated.
		* @see GenericPreciseAnonymity.recipientAnonymity()
		*/
		double getPreciseRelationshipAnonymity();

		/**
		* Computes precise anonymity guarantees for a greedily choosing adversary for sender anonymity.
		* If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		* @see GenericWorstCaseAnonymity
		* Then calculates the precise anonymity guarantee for this list of nodes.
		* @see GenericPreciseAnonymity.relationshipAnonymity()
		*/

		double getNetworkSenderAnonymity();
		double getNetworkRecipientAnonymity();
		double getNetworkRelationshipAnonymity();

		double lowerBoundSenderAnonymity();
		/**
		* Computes precise anonymity guarantees for a greedily choosing adversary for recipient anonymity.
		* If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		* @see GenericWorstCaseAnonymity
		* Then calculates the precise anonymity guarantee for this list of nodes.
		* @see GenericPreciseAnonymity.recipientAnonymity()
		*/
		double lowerBoundRecipientAnonymity();
		/**
		* Computes precise anonymity guarantees for a greedily choosing adversary for relationship anonymity.
		* If generic adversary advantage hasn't been computed for current specifications yet, it gets calculated.
		* @see GenericWorstCaseAnonymity
		* Then calculates the precise anonymity guarantee for this list of nodes.
		* @see GenericPreciseAnonymity.relationshipAnonymity()
		*/
		double lowerBoundRelationshipAnonymity();


		/**
		 * Adds a programmable cost function for adversary's cost map.
		 * @param pcf new cost PCF.
		 */
		void addProgrammableCostFunction(std::shared_ptr<ProgrammableCostFunction> pcf);
		/**
		 * Computes PCF-dependent relays costs for given relays.
		 * This function should be called after adding all desired PCFs
		 * and before calculating anonymities.
		 */
		void commitPCFs();
		/**
		 * Removes all PCFs, and resets to the basic k-of-N model.
		 * Call commitPCFs() afterwards to let the changes take effect.
		 */
		void resetPCFs();
		/**
		 * Sets the attacker's costs. The given string will be parsed as the only PCF.
		 * No commitPCFs() call is necessary afterwards.
		 */
		void setPCF(std::string& pcf);
		/**
		* Sets the attacker's costs. The given function will be used as the only PCF, without predicate.
		* No commitPCFs() call is necessary afterwards.
		*/
		void setPCFCallback(const std::function<double(const Relay&, double)>& pcf);
		/**
		 * Set a budget adversary may use to compromise relays.
		 * @param budget adversary budget.
		 */
		void setAdversaryBudget(double budget);


		/**
		 * Creates Path Selection instances if their specification changed.
		 * This function should be called to apply changes in specifications.
		 */
		void commitSpecification();

		/**
		 * Prepares the path selection - including delta calculation. Not really necessary...
		 */
		void prepareCalculation();

		void preparePreciseCalculation(std::vector<size_t> compromisedNodes);

		void prepareNetworkCalculation(std::vector<std::string> compromisedASes);


	private:
		// variables
		int computeFlags = 15; /**< Set of flags (x|...|x|PSB2|PSB1|PSA2|PSA1) indicating whether any of Path Selection instances should be recomputed. */
		std::shared_ptr<PathSelection> pathSelectionA1; /**< Path selection computed from specification for sender A, path selection 1 and recipient 1. */
		std::shared_ptr<PathSelection> pathSelectionA2; /**< Path selection computed from specification for sender A, path selection 1 and recipient 2. */
		std::shared_ptr<PathSelection> pathSelectionB1; /**< Path selection computed from specification for sender B, path selection 2 and recipient 1. */
		std::shared_ptr<PathSelection> pathSelectionB2; /**< Path selection computed from specification for sender B, path selection 2 and recipient 2. */
		std::shared_ptr<Consensus> consensus; /**< Consensus describing current state of Tor network. */
		Adversary adversary; /**< Adversary instance. */
		std::unique_ptr<GenericWorstCaseAnonymity> gwca; /** Class for computing generic worst case anonymities. Since it may be uninitialized, pointer is used. */
		std::unique_ptr<GenericPreciseAnonymity> gpra; /** Class for computing generic precise anonymities. Since it may be uninitialized, pointer is used. */
		double epsilon = 1; /** Multiplicative factor used in computations. */

		std::shared_ptr<ASMap> asmap; /**< Consensus describing current state of Tor network. */
		std::shared_ptr<SenderSpec> senderSpec1; /** Specification of sender A. */
		std::shared_ptr<SenderSpec> senderSpec2; /** Specification of sender B. */
		std::shared_ptr<RecipientSpec> recipientSpec1; /** Specification of recipient 1. */
		std::shared_ptr<RecipientSpec> recipientSpec2; /** Specification of recipient 2. */
		std::shared_ptr<PathSelectionSpec> pathSelectionSpec1; /** Specification path selection in game 1. */
		std::shared_ptr<PathSelectionSpec> pathSelectionSpec2; /** Specification path selection in game 2. */
};

#endif
