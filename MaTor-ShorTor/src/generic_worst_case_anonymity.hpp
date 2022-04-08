#ifndef GENERIC_WORST_CASE_ANONYMITY_HPP
#define GENERIC_WORST_CASE_ANONYMITY_HPP

/** @file */

#include <vector>
#include <atomic>
#include <cstdint>

#include "types/symmetric_matrix.hpp"
#include "consensus.hpp"
#include "path_selection.hpp"
#include "adversary.hpp"

//typedef uint32_t numeric_type;
typedef double numeric_type;

class myatomic_type :public std::atomic<double> {
public:
	double fetch_add(double arg) {
		double expected = this->load();
		while (!atomic_compare_exchange_weak(this, &expected, expected + arg))
			;
		return expected;
	};
	myatomic_type(double v) :std::atomic<double>(v) {};
};

//typedef std::atomic<numeric_type> atomic_type;
typedef myatomic_type atomic_type;

/**
 * Class provides computational utility for obtaining upper bound for anonymity guarantees
 * in Tor network. Given specified consensus and path selection details,
 * it computes adversary advantage in distinguishing scenarios in anonymity games.
 * Adversary's advantage is based on differences in probabilities of creating particular circuits
 * (for instance, different probabilities for selecting relay as exit node for two senders 
 * in sender anonymity game). Because it is worst case analysis,
 * after obtaining advantages for every node, adversary simulation compromises affordable relays
 * which give it the highest probability of distinguishing scenarios.  
 * @see senderAnonymity()
 * @see recipientAnonymity()
 * @see relationshipAnonymity()
 * @see Adversary
 */
class GenericWorstCaseAnonymity
{
	public:
		// constructors
		/**
		 * Constructor computes adversary's advantages in distinguishing scenarios
		 * based on variety of observations.
		 * @param consensus consensus describing Tor network state.
		 * @param psA1 path selection for sender A and recipient 1 pair
		 * @param psA2 path selection for sender A and recipient 2 pair
		 * @param psB1 path selection for sender B and recipient 1 pair
		 * @param psB2 path selection for sender B and recipient 2 pair
		 * @param epsilon multiplicative factor
		 */
		GenericWorstCaseAnonymity(
			const Consensus& consensus,
			const PathSelection& psA1,
			const PathSelection& psA2,
			const PathSelection& psB1,
			const PathSelection& psB2,
			double epsilon = 1);
		
		// functions
		/**
		 * Computes worst case anonymity guarantees for sender anonymity.
		 * There are two senders, one of them connects to malicious recipient (adversary).
		 * Adversary plays a game in which he tries to guess which sender connects to them.
		 * In this case, only recipient 1 is taken into account.
		 * @param adversary adversary definition
		 * @return upper bound anonymity guarantees for relationship anonymity.
		 */
		double senderAnonymity(Adversary& adversary);
		/**
		 * Computes worst case anonymity guarantees for recipient anonymity.
		 * There are two recipients and a client connecting to one of them.
		 * Adversary is assumed to control the link between the client and the Tor network
		 * (for instance, malicious ISP).
		 * Adversary plays a game in which he tries to guess to which recipient sender connects.
		 * Adversary may monitor the traffic between sender and Tor network.
		 * In this case, only sender A is taken into account.
		 * @param adversary adversary definition
		 * @return upper bound anonymity guarantees for recipient anonymity.
		 */
		double recipientAnonymity(Adversary& adversary);
		/**
		 * Computes worst case anonymity guarantees for relationship anonymity.
		 * There are two recipients and two clients as two connections pairs.
		 * There are two scenarios:
		 * either ClientA connects to Repipient1 and ClientB connects to Recipient2
		 * or ClientA connects to Repipient2 and ClientB connects to Recipient1.
		 * Adversary plays a game in which he tries to distinguish abovementioned scenarios.
		 * @param adversary adversary definition
		 * @return upper bound anonymity guarantees for relationship anonymity.
		 */
		double relationshipAnonymity(Adversary& adversary);


		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		* @param adversary adversary definition
		*/
		void greedySenderAnonymity(std::vector<size_t>& output, Adversary& adversary);

		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		* @param adversary adversary definition
		*/
		void greedyRecipientAnonymity(std::vector<size_t>& output, Adversary& adversary);

		/**
		* Greedily computes a list of nodes the adversary can compromise.
		* @param output the list of nodes is stored in this parameter.
		* @param adversary adversary definition
		*/
		void greedyRelationshipAnonymity(std::vector<size_t>& output, Adversary& adversary);

		void printBests(const Consensus& consensus, size_t number) const;
	
	private:
		size_t size;
		
		// variables
		std::vector<numeric_type> deltaPerNodeRelA1; /**< Overall avantage (sum) for relationship anonymity at specified node. */ 
		std::vector<numeric_type> deltaPerNodeRelA2; /**< Overall avantage (sum) for relationship anonymity at specified node (symmetric). */
		std::vector<numeric_type> deltaPerNodeSA1; /**< Overall avantage (sum) for sender anonymity at specified node: A1 and B1. */
		std::vector<numeric_type> deltaPerNodeSA2; /**< Overall avantage (sum) for sender anonymity at specified node: B1 and A1. */
		std::vector<numeric_type> deltaPerNodeRA1; /**< Overall avantage (sum) for recipient anonymity at specified node: A1 and A2. */
		std::vector<numeric_type> deltaPerNodeRA2; /**< Overall avantage (sum) for recipient anonymity at specified node: A2 and A1. */
		
		SymmetricMatrix<numeric_type> deltaPairs1; /**< Advantage obtained by observing both exit and entry for one of senders in A1 B2 case (for relationship anonymity). */
		SymmetricMatrix<numeric_type> deltaPairs2; /**< Advantage obtained by observing both exit and entry for one of senders in B1 A2 case (for relationship anonymity). */

		SymmetricMatrix<numeric_type> deltaIndirectPairsSA1; /**< Indirect impact of a pair of nodes for sender anonymity. */
		SymmetricMatrix<numeric_type> deltaIndirectPairsSA2; /**< Indirect impact of a pair of nodes for sender anonymity. */
		SymmetricMatrix<numeric_type> deltaIndirectPairsRA1; /**< Indirect impact of a pair of nodes for recipient anonymity. */
		SymmetricMatrix<numeric_type> deltaIndirectPairsRA2; /**< Indirect impact of a pair of nodes for recipient anonymity. */
		SymmetricMatrix<numeric_type> deltaIndirectPairsREL1; /**< Indirect impact of a pair of nodes for relationship anonymity. */
		SymmetricMatrix<numeric_type> deltaIndirectPairsREL2; /**< Indirect impact of a pair of nodes for relationship anonymity. */

		std::vector<numeric_type> deltaIndirectPerNodeSA1; /**< Indirect impact per node for sender anonymity. */
		std::vector<numeric_type> deltaIndirectPerNodeSA2; /**< Indirect impact per node for sender anonymity. */
		std::vector<numeric_type> deltaIndirectPerNodeRA1; /**< Indirect impact per node for recipient anonymity. */
		std::vector<numeric_type> deltaIndirectPerNodeRA2; /**< Indirect impact per node for recipient anonymity. */
		// For Relationship Anonymity, such an indirect impact per node does not exist.

		numeric_type deltaServer1; /**< Overall advantage sum for adversary-controlled Server (sum of phi(A1, B1)) computed for differences of exits selection probabilities. */
		numeric_type deltaServer2; /**< Overall advantage sum for adversary-controlled Server (sum of phi(B1, A1)) computed for differences of exits selection probabilities. */
		numeric_type deltaISP1; /**< Overall advantage sum for adversary-controlled ISP (sum of phi(A1, A2)) computed for differences of entry selection probabilities. */
		numeric_type deltaISP2; /**< Overall advantage sum for adversary-controlled ISP (sum of phi(A2, A1)) computed for differences of entry selection probabilities. */
		
		double solveWithPairs(std::vector<numeric_type>& scenario1, std::vector<numeric_type>& scenario2, SymmetricMatrix<numeric_type>& scenario1pairs, SymmetricMatrix<numeric_type>& scenario2pairs,
			numeric_type flatAdd1, numeric_type flatAdd2, Adversary& adversary); // Our solver that returns the anonymity impact of a budget adversary.

		void greedyAlgorithm(std::vector<size_t>& output, 
			std::vector<numeric_type>& scenario1, std::vector<numeric_type>& scenario2,	SymmetricMatrix<numeric_type>& scenario1pairs, SymmetricMatrix<numeric_type>& scenario2pairs,
			Adversary& adversary); // Greedy algorithm that chooses nodes that are still within the budget and puts them into the output vector
};

#endif
