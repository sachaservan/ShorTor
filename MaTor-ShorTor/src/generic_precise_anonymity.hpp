#ifndef GENERIC_PRECISE_ANONYMITY_HPP
#define GENERIC_PRECISE_ANONYMITY_HPP

/** @file */

#include <vector>
#include <atomic>
#include <cstdint>

#include "types/symmetric_matrix.hpp"
#include "types/const_vector.hpp"
#include "consensus.hpp"
#include "path_selection.hpp"
#include "adversary.hpp"
#include "generic_worst_case_anonymity.hpp"
#include <unordered_map>

//typedef uint32_t numeric_type;
//typedef double numeric_type;
//typedef std::atomic<numeric_type> atomic_type;
typedef bool observation[5];
typedef std::pair<observation, int> obstask;


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
class GenericPreciseAnonymity
{
	public:
		// constructors
		/**
		 * Constructor computes adversary's advantages in distinguishing the scenarios
		 * based on variety of observations.
		 * @param consensus consensus describing Tor network state.
		 * @param psA1 path selection for sender A and recipient 1 pair
		 * @param psA2 path selection for sender A and recipient 2 pair
		 * @param psB1 path selection for sender B and recipient 1 pair
		 * @param psB2 path selection for sender B and recipient 2 pair
		 * @param compromisednodes defines the compromised connections between Tor nodes
		 * @param compromisedsenderA defines the compromised connections between the sender A and Tor nodes
		 * @param compromisedsenderB defines the compromised connections between the sender B and Tor nodes
		 * @param compromisedrecipient1 defines the compromised connections between Tor nodes and recipient 1
		 * @param compromisedrecipient2 defines the compromised connections between Tor nodes and recipient 2
		 * @param epsilon multiplicative factor
		 */
		GenericPreciseAnonymity(
			const Consensus& consensus,
			const PathSelection& psA1,
			const PathSelection& psA2,
			const PathSelection& psB1,
			const PathSelection& psB2,
			const_vector<const_vector<bool>>& compromisednodes,
			const_vector<bool>& compromisedsenderA,
			const_vector<bool>& compromisedsenderB,
			const_vector<bool>& compromisedrecipient1,
			const_vector<bool>& compromisedrecipient2,
			double epsilon = 1);

		// functions
		/**
		* returns the guarantee for sender anonymity.
		 */
		double senderAnonymity();
		/**
		* returns the guarantee for recipient anonymity.
		*/
		double recipientAnonymity();
		/**
		 * returns the guarantee for relationship anonymity.
		 */
		double relationshipAnonymity();
		

	private:
		size_t size;

		numeric_type deltaSA;
		numeric_type deltaRA;
		numeric_type deltaREL;
		
};

#endif
