#include "generic_worst_case_anonymity.hpp"
#include "types/const_vector.hpp"
#include "types/work_manager.hpp"
#include "utils.hpp"

#include <numeric>

//constexpr size_t uint_precision = sizeof(numeric_type) * 8 * 15 / 16; // *15/16 == make 64 -> 60, 32 -> 30, etc...
//constexpr probability_t conversion_const = (1ull << uint_precision);
//constexpr probability_t conversion_const_inv = 1.0 / conversion_const;

const bool CONSIDER_INDIRECT_IMPACT = true;

/*
static inline numeric_type convert_d2i(probability_t value)
{
	return static_cast<numeric_type>(value * conversion_const);
}

static inline probability_t convert_i2d(numeric_type value)
{
	return static_cast<probability_t>(value) * conversion_const_inv;
}*/
static inline numeric_type convert_d2i(probability_t value)
{
	return value;
//	return static_cast<numeric_type>(value * conversion_const);
}

static inline probability_t convert_i2d(numeric_type value)
{
	return value;
//	return static_cast<probability_t>(value) * conversion_const_inv;
}

// accumulates the advantage of the better scenario to its Accumulator. 
// better = higher probability
template<typename Probability, typename Accumulator>
static inline void phi(const Probability scenario1, const Probability scenario2, 
	Accumulator &accumScenario1, Accumulator &accumScenario2, 
	void function(Accumulator&, const Probability) ) 
{
	//asm("addl $0, %eax"); // marker to find the function in assembly code
	//typedef typename std::make_signed<Probability>::type sProbability;
	//sProbability stest = scenario1 - scenario2;
	Probability stest = scenario1 - scenario2;
	//if(stest)
	{
		if(stest > 0)
			function(accumScenario1, stest);
		else
			function(accumScenario2, -stest);
	}
}

template<typename Value, typename Accumulator>
static inline void atomic_add(Accumulator& accumulator, const Value value)
{
	accumulator.fetch_add(value);
}

template<typename Value, typename Accumulator>
static inline void normal_add(Accumulator& accumulator, const Value value)
{
	accumulator += value;
}

// TODO: we assume epsilon == 1. Add handling weird, different cases.
GenericWorstCaseAnonymity::GenericWorstCaseAnonymity(
	const Consensus& consensus,
	const PathSelection& psA1,
	const PathSelection& psA2,
	const PathSelection& psB1,
	const PathSelection& psB2,
	double epsilon) : 
	size(consensus.getSize()),
	deltaPairs1(size, true), deltaPairs2(size, true),
	deltaPerNodeSA1(size, 0), deltaPerNodeSA2(size, 0),
	deltaPerNodeRA1(size, 0), deltaPerNodeRA2(size, 0),
	deltaPerNodeRelA1(size, 0), deltaPerNodeRelA2(size, 0),
	deltaServer1(0), deltaServer2(0),
	deltaISP1(0), deltaISP2(0),
	deltaIndirectPairsSA1(size, true), deltaIndirectPairsSA2(size, true),
	deltaIndirectPairsRA1(size, true), deltaIndirectPairsRA2(size, true),
	deltaIndirectPairsREL1(size, true), deltaIndirectPairsREL2(size, true),
	deltaIndirectPerNodeSA1(size, 0), deltaIndirectPerNodeSA2(size, 0),
	deltaIndirectPerNodeRA1(size, 0), deltaIndirectPerNodeRA2(size, 0)
	{
	
	if (epsilon != 1) {
		NOT_IMPLEMENTED;
	}
	
	// the following variables are used as accumulators for multi-threaded computation
	// atomic type for multicore concurrent computation.

	// for exit loop
	clogsn("Resizing vectors...");
	atomic_type accumDeltaServer1(0); /**< Accumulator for deltaServer1 in thread-safe type. */
	atomic_type accumDeltaServer2(0); /**< Accumulator for deltaServer2 in thread-safe type. */
	const_vector<numeric_type> probForExitRA1(size, 0); /**< Advantage obtained by simply compromising this exit = probability of selecting relay as exit (instant recipient anonymity break) if A connects to 1. */
	const_vector<numeric_type> probForExitRA2(size, 0); /**< Advantage obtained by simply compromising this exit = probability of selecting relay as exit (instant recipient anonymity break) if A connects to 2. */
	// (at the end of the loop)
	const_vector<numeric_type> deltaForExitRelA1(size, 0); /**< Advantage obtained by exit node probability differences for relationship anonymity (phi(A1, B1) + phi(B2, A2)) / 2. */
	const_vector<numeric_type> deltaForExitRelA2(size, 0); /**< Advantage obtained by exit node probability differences for relationship anonymity (phi(B1, A1) + phi(A2, B2)) / 2. */
	
	const_vector<numeric_type> deltaForExitSA1(size, 0); /**< Advantage obtained by exit node probability differences for sender anonymity (phi(A1, B1)). */
	const_vector<numeric_type> deltaForExitSA2(size, 0); /**< Advantage obtained by exit node probability differences for sender anonymity (phi(B1, A1)). */
	
	// for entry loop
	const_vector<atomic_type> probForEntryA1(size, 0); /**< Sum of probabilities of selecting relay as entry for all exits if A connects to 1 (also advantage for instant sender anonymity break). */
	const_vector<atomic_type> probForEntryA2(size, 0); /**< Sum of probabilities of selecting relay as entry for all exits if A connects to 2 (also advantage for instant sender anonymity break). */
	const_vector<atomic_type> probForEntryB1(size, 0); /**< Sum of probabilities of selecting relay as entry for all exits if B connects to 1 (also advantage for instant sender anonymity break). */
	//const_vector<atomic_type> probForEntryB2(size, 0); /**< Sum of probabilities of selecting relay as entry for all exits if B connects to 2 (also advantage for instant sender anonymity break). */
	
	const_vector<const_vector<numeric_type>> probForExitEntryRelA1(size, size, 0); /**< Probability (advantage in relationship anonymity) of selecting the pair of entry and exit nodes (A1 B2). [exit][entry] matrix. */
	const_vector<const_vector<numeric_type>> probForExitEntryRelA2(size, size, 0); /**< Probability (advantage in relationship anonymity) of selecting the pair of entry and exit nodes (A2 B1). [exit][entry] matrix. */
	
	// for middle loop
	const_vector<const_vector<atomic_type>> probForEntryMiddlePairA1(size, size, 0); /**< Sum of probabilities (for all exits) of selecting pair of relays as entry and middle when A connects to 1. [entry][middle] matrix. */
	const_vector<const_vector<atomic_type>> probForEntryMiddlePairA2(size, size, 0); /**< Sum of probabilities (for all exits) of selecting pair of relays as entry and middle when A connects to 2. [entry][middle] matrix. */
	const_vector<const_vector<atomic_type>> probForEntryMiddlePairB1(size, size, 0); /**< Sum of probabilities (for all exits) of selecting pair of relays as entry and middle when B connects to 1. [entry][middle] matrix. */
	const_vector<const_vector<atomic_type>> probForEntryMiddlePairB2(size, size, 0); /**< Sum of probabilities (for all exits) of selecting pair of relays as entry and middle when B connects to 2. [entry][middle] matrix. */
	
	//const_vector<const_vector<numeric_type>> probForExitMiddlePairA1(size, size, 0); /**< Sum of probabilities (for all entries) of selecting pair of relays as exit and middle when A connects to 1. [exit][middle] matrix. */
	//const_vector<const_vector<numeric_type>> probForExitMiddlePairA2(size, size, 0); /**< Sum of probabilities (for all entries) of selecting pair of relays as exit and middle when A connects to 2. [exit][middle] matrix. */
	//const_vector<const_vector<numeric_type>> probForExitMiddlePairB1(size, size, 0); /**< Sum of probabilities (for all entries) of selecting pair of relays as exit and middle when B connects to 1. [exit][middle] matrix. */
	//const_vector<const_vector<numeric_type>> probForExitMiddlePairB2(size, size, 0); /**< Sum of probabilities (for all entries) of selecting pair of relays as exit and middle when B connects to 2. [exit][middle] matrix. */
	
	const_vector<atomic_type> deltaForMiddleSA1(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing differences between A1 and B1 scenario as middle node ((phi(A1, B1)). */
	const_vector<atomic_type> deltaForMiddleSA2(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing differences between B1 and A1 scenario as middle node ((phi(B1, A1)). */
	const_vector<atomic_type> deltaForMiddleRA1(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing differences between A1 and A2 scenario as middle node ((phi(A1, A2)). */
	const_vector<atomic_type> deltaForMiddleRA2(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing differences between A2 and A1 scenario as middle node ((phi(A2, A1)). */
	const_vector<atomic_type> deltaTriple1(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing whole circuit as middle node in A1 B2 case (for relationship anonymity). */
	const_vector<atomic_type> deltaTriple2(size, 0); /**< Sum of advantage (for all entries and exits) obtained by observing whole circuit as middle node in B1 A2 case (for relationship anonymity). */
	
	const_vector<const_vector<atomic_type>> deltaForEntryMiddleRelA1(size, size, 0); /**< Sum of advantage (for each exit) obtained by observing differences between A1 and A2 or B1 and B2 as entry and middle. [entry][middle] matrix.*/
	const_vector<const_vector<atomic_type>> deltaForEntryMiddleRelA2(size, size, 0); /**< Sum of advantage (for each exit) obtained by observing differences between A2 and A1 or B2 and B1 as entry and middle. [entry][middle] matrix.*/
	const_vector<const_vector<numeric_type>> deltaForExitMiddleRelA1(size, size, 0); /**< Sum of advantage (for each entry) obtained by observing differences between A1 and B1 or A2 and B2 as exit and middle. [exit][middle] matrix.*/
	const_vector<const_vector<numeric_type>> deltaForExitMiddleRelA2(size, size, 0); /**< Sum of advantage (for each entry) obtained by observing differences between B1 and A1 or B2 and A2 as exit and middle. [exit][middle] matrix.*/
		
	// for the outer quadloop
	const_vector<numeric_type> deltaForEntryRA1(size, 0); /**< Advantage obtained by entry node probability differences for recipient anonymity (phi(A1, A2)). */
	const_vector<numeric_type> deltaForEntryRA2(size, 0); /**< Advantage obtained by entry node probability differences for recipient anonymity (phi(A2, A1)). */
	
	const_vector<numeric_type> deltaForEntryRelA1(size, 0); /**< Advantage obtained by entry node probability differences for relationship anonymity (phi(A1, A2) + phi(B2, B1)) / 2. */
	const_vector<numeric_type> deltaForEntryRelA2(size, 0); /**< Advantage obtained by entry node probability differences for relationship anonymity (phi(A2, A1) + phi(B1, B2)) / 2. */

	/* Here start the variables for the indirect impact!*/

	// the Guard-Exit impacts Impact_{indirect}^{(ab)(cd)}(n,n')
	const_vector<const_vector<atomic_type>> impactIndirectA1A2(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectA2A1(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectB1B2(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectB2B1(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */

	const_vector<const_vector<atomic_type>> impactIndirectA1B1(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectB1A1(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectA2B2(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */
	const_vector<const_vector<atomic_type>> impactIndirectB2A2(size, size, 0); /**< Sum (over all middle nodes) of differences in probabilities that n and n' are guard and exit (with the respective middle node). */

	// Indirect impact Rec2
	const_vector<const_vector<atomic_type>> impactIndirectRec2A1B1(size, size, 0); /**< Sum (over all exit nodes) of differences in probabilities that n and n' are guard and middle or middle and guard (with the respective exit node). */
	const_vector<const_vector<atomic_type>> impactIndirectRec2B1A1(size, size, 0); /**< Sum (over all exit nodes) of differences in probabilities that n and n' are guard and middle or middle and guard (with the respective exit node). */

	// Indirect impact Sen2
	const_vector<const_vector<atomic_type>> impactIndirectSen2A1A2(size, size, 0); /**< Sum (over all guard nodes) of differences in probabilities that n and n' are middle and exit or exit and middle (with the respective guard node). */
	const_vector<const_vector<atomic_type>> impactIndirectSen2A2A1(size, size, 0); /**< Sum (over all guard nodes) of differences in probabilities that n and n' are middle and exit or exit and middle (with the respective guard node). */

	// Accumulating probabilities for Rec1
	const_vector<const_vector<atomic_type>> gmProbForXA1(size, size, 0); /**< Probability of a node to be guard or middle node for a specific exit node */
	const_vector<const_vector<atomic_type>> gmProbForXB1(size, size, 0); /**< Probability of a node to be guard or middle node for a specific exit node */

	// Accumulating probabilities for Sen1
	const_vector<const_vector<atomic_type>> mxProbForGA1(size, size, 0); /**< Probability of a node to be middle node or exit node for a specific guard node */
	const_vector<const_vector<atomic_type>> mxProbForGA2(size, size, 0); /**< Probability of a node to be middle node or exit node for a specific guard node */


	// Partition the job into chunks
	// The larger the chunks, the less tasks, but might cause unbalanced work distribution
	constexpr size_t chunk_size = 16;
	WorkManager manager;
	for(size_t i = 0; i < size; i += chunk_size)
	{
		size_t begin = i, end = begin + chunk_size;
		// last chunk: stop at size, don't go further
		if(end > size) end = size;
		manager.addTask([&, begin, end](){
			for(size_t exit_index = begin; exit_index < end; ++exit_index)
			{
				// temporary arrays to store cumulative observations of exit node on (_, _, middle, exit, recipient)
				const_vector<numeric_type> middleExitSumA1(size, 0);
				const_vector<numeric_type> middleExitSumA2(size, 0);
				const_vector<numeric_type> middleExitSumB1(size, 0);
				const_vector<numeric_type> middleExitSumB2(size, 0);
				
				probability_t exitProbabilityA1 = psA1.exitProb(exit_index);
				probability_t exitProbabilityA2 = psA2.exitProb(exit_index);
				probability_t exitProbabilityB1 = psB1.exitProb(exit_index);
				probability_t exitProbabilityB2 = psB2.exitProb(exit_index);

				// xP stands for e(x)it(P)robability
				numeric_type conv_xPA1 = convert_d2i(exitProbabilityA1);
				numeric_type conv_xPA2 = convert_d2i(exitProbabilityA2);
				numeric_type conv_xPB1 = convert_d2i(exitProbabilityB1);
				numeric_type conv_xPB2 = convert_d2i(exitProbabilityB2);
				
				// Test, whether the node can be an exit node in any scenario, continue otherwise
				if(!(conv_xPA1 || conv_xPA2 || conv_xPB1 || conv_xPB2))
					continue;
				
				// compute phi-s for server in both scenarios:
				phi(conv_xPA1, conv_xPB1, accumDeltaServer1, accumDeltaServer2, atomic_add);
				
				// assign probabilities for distinguishing events for recipient anonymity (same sender) 
				probForExitRA1[exit_index] = conv_xPA1;
				probForExitRA2[exit_index] = conv_xPA2;
				
				for(size_t entry_index = 0; entry_index < size; ++entry_index)
				{
					probability_t entryProbabilityA1 = exitProbabilityA1 * psA1.entryProb(entry_index, exit_index);
					probability_t entryProbabilityA2 = exitProbabilityA2 * psA2.entryProb(entry_index, exit_index);
					probability_t entryProbabilityB1 = exitProbabilityB1 * psB1.entryProb(entry_index, exit_index);
					probability_t entryProbabilityB2 = exitProbabilityB2 * psB2.entryProb(entry_index, exit_index);
					
					// gxP is the probability of selecting BOTH (g)uard and (e)xit
					// NOT the conditional probability of entryProb.
					numeric_type conv_gxPA1 = convert_d2i(entryProbabilityA1);
					numeric_type conv_gxPA2 = convert_d2i(entryProbabilityA2);
					numeric_type conv_gxPB1 = convert_d2i(entryProbabilityB1);
					numeric_type conv_gxPB2 = convert_d2i(entryProbabilityB2);
					
					// Test, whether the pair can be selected in any scenario, continue otherwise
					if(!(conv_gxPA1 || conv_gxPA2 || conv_gxPB1 || conv_gxPB2))
						continue;
						
					// add partial probabilities to the probability of selecting entry node
					probForEntryA1[entry_index].fetch_add(conv_gxPA1);
					probForEntryA2[entry_index].fetch_add(conv_gxPA2);
					probForEntryB1[entry_index].fetch_add(conv_gxPB1);
					//probForEntryB2[entry_index].fetch_add(conv_gxPB2);
					
					// assign probabilities for distinguishing events for relationship anonymity (A1 B2 vs A2 B1)
					probForExitEntryRelA1[exit_index][entry_index] = (conv_gxPA1 + conv_gxPB2) /2;
					probForExitEntryRelA2[exit_index][entry_index] = (conv_gxPA2 + conv_gxPB1) /2;
					
					for(size_t middle_index = 0; middle_index < size; ++middle_index)
					{
						probability_t middleProbabilityA1 = entryProbabilityA1 * psA1.middleProb(middle_index, entry_index, exit_index);
						probability_t middleProbabilityA2 = entryProbabilityA2 * psA2.middleProb(middle_index, entry_index, exit_index);
						probability_t middleProbabilityB1 = entryProbabilityB1 * psB1.middleProb(middle_index, entry_index, exit_index);
						probability_t middleProbabilityB2 = entryProbabilityB2 * psB2.middleProb(middle_index, entry_index, exit_index);
						
						// gmxP is the probability of selecting (g)uard (m)iddle and (e)xit (the circuit
						// NOT the conditional probability of middleProb.
						numeric_type conv_gmxPA1 = convert_d2i(middleProbabilityA1);
						numeric_type conv_gmxPA2 = convert_d2i(middleProbabilityA2);
						numeric_type conv_gmxPB1 = convert_d2i(middleProbabilityB1);
						numeric_type conv_gmxPB2 = convert_d2i(middleProbabilityB2);
					
						// Test, whether the circuit can be selected in any scenario, continue otherwise
						if(!(conv_gmxPA1 || conv_gmxPA2 || conv_gmxPB1 || conv_gmxPB2))
							continue;

						// accumulate probabilities of _, _, (middle), exit, (recipient) observations
						middleExitSumA1[middle_index] += conv_gmxPA1;
						middleExitSumA2[middle_index] += conv_gmxPA2;
						middleExitSumB1[middle_index] += conv_gmxPB1;
						middleExitSumB2[middle_index] += conv_gmxPB2;
						
						// accumulate probabilities of (sender), entry, (middle), _, _ observations
						probForEntryMiddlePairA1[entry_index][middle_index].fetch_add(conv_gmxPA1);
						probForEntryMiddlePairA2[entry_index][middle_index].fetch_add(conv_gmxPA2);
						probForEntryMiddlePairB1[entry_index][middle_index].fetch_add(conv_gmxPB1);
						probForEntryMiddlePairB2[entry_index][middle_index].fetch_add(conv_gmxPB2);
						
						// compute probabilities of the circuits in relationship anonymity case:
						numeric_type conv_gmxRelA1 = (conv_gmxPA1 + conv_gmxPB2) /2; // A1 B2
						numeric_type conv_gmxRelA2 = (conv_gmxPB1 + conv_gmxPA2) /2; // B1 A2
						
						// compute the deltas for middle nodes:
						//  Sender Anonymity (A1 vs B1)
						phi(conv_gmxPA1, conv_gmxPB1, deltaForMiddleSA1[middle_index], deltaForMiddleSA2[middle_index], atomic_add);
						//  Recipient Anonymity (A1 vs A2)
						phi(conv_gmxPA1, conv_gmxPA2, deltaForMiddleRA1[middle_index], deltaForMiddleRA2[middle_index], atomic_add);
						//  Relationship Anonymity (A1 B2 vs B1 A2)
						phi(conv_gmxRelA1, conv_gmxRelA2, deltaTriple1[middle_index], deltaTriple2[middle_index], atomic_add);
						
						// compute the deltas for pairs of nodes: (remember to divide this by 2 in the end!)
						//  (sender), entry, middle, (exit), _ 
						phi(conv_gmxPA1, conv_gmxPA2, 
							deltaForEntryMiddleRelA1[entry_index][middle_index], 
							deltaForEntryMiddleRelA2[entry_index][middle_index], atomic_add);
						phi(conv_gmxPB2, conv_gmxPB1, 
							deltaForEntryMiddleRelA1[entry_index][middle_index], 
							deltaForEntryMiddleRelA2[entry_index][middle_index], atomic_add);
						// _, (entry), middle, exit, (recipient) 
						phi(conv_gmxPA1, conv_gmxPB1, 
							deltaForExitMiddleRelA1[exit_index][middle_index], 
							deltaForExitMiddleRelA2[exit_index][middle_index], normal_add);
						phi(conv_gmxPB2, conv_gmxPA2, 
							deltaForExitMiddleRelA1[exit_index][middle_index], 
							deltaForExitMiddleRelA2[exit_index][middle_index], normal_add);

						// Compute the (indirect) Guard-Exit impacts Impact_{indirect}^{(ab)(cd)}(n,n')
						phi(conv_gmxPA1, conv_gmxPA2, impactIndirectA1A2[entry_index][exit_index], impactIndirectA2A1[entry_index][exit_index], atomic_add);
						phi(conv_gmxPB1, conv_gmxPB2, impactIndirectB1B2[entry_index][exit_index], impactIndirectB2B1[entry_index][exit_index], atomic_add);
						phi(conv_gmxPA1, conv_gmxPB1, impactIndirectA1B1[entry_index][exit_index], impactIndirectB1A1[entry_index][exit_index], atomic_add);
						phi(conv_gmxPA2, conv_gmxPB2, impactIndirectA2B2[entry_index][exit_index], impactIndirectB2A2[entry_index][exit_index], atomic_add);

						// Compute the (indirect) Sen2 and Rec2 impacts.
						phi(conv_gmxPA1, conv_gmxPB1, impactIndirectRec2A1B1[entry_index][middle_index], impactIndirectRec2B1A1[entry_index][middle_index], atomic_add);
						phi(conv_gmxPA1, conv_gmxPA2, impactIndirectSen2A1A2[middle_index][exit_index], impactIndirectSen2A2A1[middle_index][exit_index], atomic_add);

						// Prepare the (indirect) Rec1 impacts by calculating the probability that a node is guard or middle (for an exit node)
						gmProbForXA1[entry_index][exit_index].fetch_add(conv_gmxPA1);
						gmProbForXA1[middle_index][exit_index].fetch_add(conv_gmxPA1);
						gmProbForXB1[entry_index][exit_index].fetch_add(conv_gmxPB1);
						gmProbForXB1[middle_index][exit_index].fetch_add(conv_gmxPB1);
						// Prepare the (indirect) Sen1 impacts by calculating the probability that a node is middle or exit (for a guard node)
						mxProbForGA1[middle_index][entry_index].fetch_add(conv_gmxPA1);
						mxProbForGA1[exit_index][entry_index].fetch_add(conv_gmxPA1);
						mxProbForGA2[middle_index][entry_index].fetch_add(conv_gmxPA2);
						mxProbForGA2[exit_index][entry_index].fetch_add(conv_gmxPA2);
					}
				}
				
				// for all middles seen by the exit, compute the distinguishing events:
				for(size_t middle_index = 0; middle_index < size; ++middle_index)
				{
					// for sender anonymity (same recipient)
					phi(middleExitSumA1[middle_index], middleExitSumB1[middle_index], deltaForExitSA1[exit_index], deltaForExitSA2[exit_index], normal_add);
					// for relationship anonymity (remember to divide this by 2 in the end!)
					phi(middleExitSumA1[middle_index], middleExitSumB1[middle_index], deltaForExitRelA1[exit_index], deltaForExitRelA2[exit_index], normal_add);
					phi(middleExitSumB2[middle_index], middleExitSumA2[middle_index], deltaForExitRelA1[exit_index], deltaForExitRelA2[exit_index], normal_add);
				}
			}
		});
	}
	// Run prepared jobs:
	std::cout << "Starting parallel jobs." << std::endl;
	manager.startAndJoinAll();
	std::cout << "All parallel jobs done (1 of 3)." << std::endl;

	// Iterate over entry middle pairs to compute distinguishing events of entry nodes for recipient anonymity
	for(size_t i = 0; i < size; i += chunk_size)
	{
		size_t begin = i, end = begin + chunk_size;
		// last chunk: stop at size, don't go further
		if(end > size) end = size;
		manager.addTask([&, begin, end](){
			for(size_t entry_index = begin; entry_index < end; ++entry_index)
			{
				for(size_t middle_index = 0; middle_index < size; ++middle_index)
				{
					// for recipient anonymity (same sender)
					phi(probForEntryMiddlePairA1[entry_index][middle_index].load(std::memory_order::memory_order_relaxed), 
						probForEntryMiddlePairA2[entry_index][middle_index].load(std::memory_order::memory_order_relaxed),
						deltaForEntryRA1[entry_index], deltaForEntryRA2[entry_index],
						normal_add);
					// for relationship anonymity (remember to divide this by 2 in the end!)
					phi(probForEntryMiddlePairA1[entry_index][middle_index].load(std::memory_order::memory_order_relaxed), 
						probForEntryMiddlePairB2[entry_index][middle_index].load(std::memory_order::memory_order_relaxed),
						deltaForEntryRelA1[entry_index], deltaForEntryRelA2[entry_index],
						normal_add);
					phi(probForEntryMiddlePairB1[entry_index][middle_index].load(std::memory_order::memory_order_relaxed), 
						probForEntryMiddlePairA2[entry_index][middle_index].load(std::memory_order::memory_order_relaxed),
						deltaForEntryRelA1[entry_index], deltaForEntryRelA2[entry_index],
						normal_add);
				}
			}
		});
	}
	// Run prepared jobs:
	std::cout << "Starting parallel jobs." << std::endl;
	manager.startAndJoinAll();
	std::cout << "All parallel jobs done (2 of 3)." << std::endl;
	

	// Extract the values from atomic types to final variables:
	// * std::vector<numeric_type> deltaPerNodeRelA1; /**< Overall avantage (sum) for relationship anonymity at specified node. */ 
	// * std::vector<numeric_type> deltaPerNodeRelA2; /**< Overall avantage (sum) for relationship anonymity at specified node (symmetric). */
	// * std::vector<numeric_type> deltaPerNodeSA1; /**< Overall avantage (sum) for sender anonymity at specified node: A1 and B1. */
	// * std::vector<numeric_type> deltaPerNodeSA2; /**< Overall avantage (sum) for sender anonymity at specified node: B1 and A1. */
	// * std::vector<numeric_type> deltaPerNodeRA1; /**< Overall avantage (sum) for recipient anonymity at specified node: A1 and A2. */
	// * std::vector<numeric_type> deltaPerNodeRA2; /**< Overall avantage (sum) for recipient anonymity at specified node: A2 and A1. */
	// 
	// * SymmetricMatrix<numeric_type> deltaPairs1; /**< Advantage obtained by observing both exit and entry for one of senders in A1 B2 case (for relationship anonymity). */
	// * SymmetricMatrix<numeric_type> deltaPairs2; /**< Advantage obtained by observing both exit and entry for one of senders in B1 A2 case (for relationship anonymity). */
	// 
	// * numeric_type deltaServer1; /**< Overall advantage sum for adversary-controlled Server (sum of phi(A1, B1)) computed for differences of exits selection probabilities. */
	// * numeric_type deltaServer2; /**< Overall advantage sum for adversary-controlled Server (sum of phi(B1, A1)) computed for differences of exits selection probabilities. */
	// * numeric_type deltaISP1; /**< Overall advantage sum for adversary-controlled ISP (sum of phi(A1, A2)) computed for differences of entry selection probabilities. */
	// * numeric_type deltaISP2; /**< Overall advantage sum for adversary-controlled ISP (sum of phi(A2, A1)) computed for differences of entry selection probabilities. */
	
	deltaServer1 = accumDeltaServer1.load(std::memory_order::memory_order_relaxed);
	deltaServer2 = accumDeltaServer2.load(std::memory_order::memory_order_relaxed);
	for(size_t i = 0; i < size; i += chunk_size)
	{
		size_t begin = i, end = begin + chunk_size;
		// last chunk: stop at size, don't go further
		if(end > size) end = size;
		manager.addTask([&, begin, end](){
			for(size_t i = begin; i < end; ++i)
			{
				// Divide relationship anonymity deltas where appropriate
				deltaPerNodeRelA1[i] = (deltaForExitRelA1[i] /2) + (deltaForEntryRelA1[i]) + deltaTriple1[i];
				deltaPerNodeRelA2[i] = (deltaForExitRelA2[i] /2) + (deltaForEntryRelA2[i]) + deltaTriple2[i];
				for(size_t j = 0; j < size; ++j)
				{
					if(i != j)
					{
						deltaPairs1[i][j] += ((deltaForEntryMiddleRelA1[i][j] /2) + (deltaForExitMiddleRelA1[i][j] /2) + probForExitEntryRelA1[i][j]); 
						deltaPairs2[i][j] += ((deltaForEntryMiddleRelA2[i][j] /2) + (deltaForExitMiddleRelA2[i][j] /2) + probForExitEntryRelA2[i][j]);
					} 
				}
				
				// Compute entry distinguishing (recipient anonymity)
				phi(probForEntryA1[i].load(std::memory_order::memory_order_relaxed), 
					probForEntryA2[i].load(std::memory_order::memory_order_relaxed), 
					deltaISP1, deltaISP2, normal_add);
				
				// Add to the vectors:
				// (sender anonymity)
				deltaPerNodeSA1[i] = probForEntryA1[i] + deltaForMiddleSA1[i] + deltaForExitSA1[i];
				deltaPerNodeSA2[i] = probForEntryA2[i] + deltaForMiddleSA2[i] + deltaForExitSA2[i];
				// (recipient anonymity)
				deltaPerNodeRA1[i] = probForExitRA1[i] + deltaForMiddleRA1[i] + deltaForEntryRA1[i];
				deltaPerNodeRA2[i] = probForExitRA2[i] + deltaForMiddleRA2[i] + deltaForEntryRA2[i];

				// Indirect Impact(!)
				// 
				if(CONSIDER_INDIRECT_IMPACT)
				{ 
					for (size_t j = 0; j < size; ++j)
					{
						if (i != j)
						{
							// Indirect Impacts per node (for sender anonymity and recipient anonymity)

							// Impact_Rec1 (for sender anonymity)
							// Note that here the inputs of the phi function are flipped (consider the formula in the paper for an explanation)!
							phi(gmProbForXB1[i][j].load(std::memory_order::memory_order_relaxed), 
								gmProbForXA1[i][j].load(std::memory_order::memory_order_relaxed), 
								deltaIndirectPerNodeSA1[i],deltaIndirectPerNodeSA2[i],normal_add);
							//myassert(gmProbForXB1[i][j].load(std::memory_order::memory_order_relaxed) - gmProbForXA1[i][j].load(std::memory_order::memory_order_relaxed) == 0);

							// Impact_Sen1 (for recipient anonymity)
							// Note that here the inputs of the phi function are flipped (consider the formula in the paper for an explanation)!
							phi(mxProbForGA2[i][j].load(std::memory_order::memory_order_relaxed),
								mxProbForGA1[i][j].load(std::memory_order::memory_order_relaxed),
								deltaIndirectPerNodeRA1[i], deltaIndirectPerNodeRA2[i], normal_add);
							//myassert(abs(mxProbForGA2[i][j].load(std::memory_order::memory_order_relaxed) - mxProbForGA1[i][j].load(std::memory_order::memory_order_relaxed)) < 0.0001);


							// Indirect Impacts per pair of nodes (for all three notions)

							// Indirect impact for sender anonymity is: Impact_indirect^(10)(00) + Impact_Rec2
							deltaIndirectPairsSA1[i][j] = impactIndirectB1A1[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectRec2A1B1[i][j].load(std::memory_order::memory_order_relaxed);
							deltaIndirectPairsSA2[i][j] = impactIndirectA1B1[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectRec2B1A1[i][j].load(std::memory_order::memory_order_relaxed);

							// Indirect impact for recipient anonymity is: Impact_indirect^(01)(00) + Impact_Sen2
							deltaIndirectPairsRA1[i][j] = impactIndirectA2A1[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectSen2A1A2[i][j].load(std::memory_order::memory_order_relaxed);
							deltaIndirectPairsRA2[i][j] = impactIndirectA1A2[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectSen2A2A1[i][j].load(std::memory_order::memory_order_relaxed);

							// Indirect impact for relationship anonymity is: 1/2 * (Impact_indirect^(01)(00) + Impact_indirect^(10)(11) + Impact_indirect^(01)(11) + Impact_indirect^(10)(00))
							deltaIndirectPairsREL1[i][j] = (impactIndirectA2A1[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectB1B2[i][j].load(std::memory_order::memory_order_relaxed)
														+   impactIndirectA2B2[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectB1A1[i][j].load(std::memory_order::memory_order_relaxed)) /2;
							deltaIndirectPairsREL2[i][j] = (impactIndirectA1A2[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectB2B1[i][j].load(std::memory_order::memory_order_relaxed)
														+   impactIndirectB2A2[i][j].load(std::memory_order::memory_order_relaxed) + impactIndirectA1B1[i][j].load(std::memory_order::memory_order_relaxed)) /2;
							deltaPairs1[i][j] += deltaIndirectPairsREL1[i][j];
							deltaPairs2[i][j] += deltaIndirectPairsREL2[i][j];
						}
					}
					deltaPerNodeSA1[i] += deltaIndirectPerNodeSA1[i];
					deltaPerNodeSA2[i] += deltaIndirectPerNodeSA2[i];
					deltaPerNodeRA1[i] += deltaIndirectPerNodeRA1[i];
					deltaPerNodeRA2[i] += deltaIndirectPerNodeRA2[i];
				}
			}
		});
	}
	// Run prepared jobs:
	std::cout << "Starting parallel jobs." << std::endl;
	manager.startAndJoinAll();
	std::cout << "All parallel jobs done (3 of 3)." << std::endl;
}



/**
 * Simple solver for top k-of-n problems.
 * 
 */
template<typename RandomAccessIterator, 
		 typename ValueType=typename std::iterator_traits<RandomAccessIterator>::value_type, 
		 typename Compare=std::less<ValueType>>
inline std::vector<ValueType>
top_of_n_vector(RandomAccessIterator first, RandomAccessIterator last, size_t elements,
                Compare comp=Compare())
{
    typedef ValueType ref_t;
    typedef std::vector<ref_t> result_t;

    result_t heap(first, last);
    typename result_t::iterator h_begin = heap.begin(), h_end = heap.end();
    std::make_heap(h_begin, h_end, comp);

    result_t temporary;
    temporary.reserve(elements); // Can't resize for references

    while(elements--)
    {
        temporary.push_back(heap.front());
        std::pop_heap(h_begin, h_end--, comp); // pop heap without unnecessary overhead
    }

    return temporary;
}

// Returns the sum of the highest k elements out of the scenarios, plus some flatAdd factor. 
// All results are bounded by 1. Returns (scenario1, scenario2, higher)
template <typename Container>
std::tuple<numeric_type, numeric_type, numeric_type> solveKofN(
	const Container& scenario1, const Container& scenario2, 
	numeric_type flatAdd1, numeric_type flatAdd2,
	size_t k, size_t n)
{
	numeric_type s1, s2;
	if(k > n)
	{
		s1 = std::accumulate(scenario1.begin(), scenario1.end(), 0ull);
		s2 = std::accumulate(scenario2.begin(), scenario2.end(), 0ull);
	}
	else
	{
		auto bests1 = top_of_n_vector(scenario1.begin(), scenario1.end(), k);
		auto bests2 = top_of_n_vector(scenario2.begin(), scenario2.end(), k);
		s1 = std::accumulate(bests1.begin(), bests1.end(), 0ull);
		s2 = std::accumulate(bests2.begin(), bests2.end(), 0ull);
	}
	numeric_type one = static_cast<numeric_type>(convert_d2i(1)), max;
//	clogsn("KofN:")
//	clogsn(s1 << " < s1 s2 > " << s2);
//	clogsn(flatAdd1 << " < flatAdd1 flatAdd2 > " << flatAdd2);
//	clogsn("1: " << one);
	s1 = std::min(s1 + flatAdd1, one);
	s2 = std::min(s2 + flatAdd2, one);
//	clogsn(s1 << " < s1 s2 > " << s2);
	max = std::max(s1, s2);
//	clogsn("KofN max: " << max);
	return std::make_tuple(s1, s2, max);
}


bool cmpfrac(std::pair<probability_t, int> a, std::pair<probability_t, int> b)
{
	return a.first<b.first;
}

typedef std::vector<std::pair<probability_t, int>> fracvec;

// returns the delta for a budget adversary with costs as specified by the adversary
// The budget is either the budget of the adversary (if OtherBudget <0) or the OtherBudget
// scenario is a vector with one entry delta per node
numeric_type solveSimple_single(std::vector<numeric_type>& scenario,
	Adversary& adversary, double Budget)
{
	double B = Budget;

	numeric_type delta = 0;

	fracvec fraclist;

	for (size_t i = 0; i < scenario.size(); i++)
	{
		double cost = adversary.getCostmap()[i];
		if (cost == 0)
		{
			delta += scenario[i];
		}
		else {
			if(cost <= B)
				fraclist.push_back(std::make_pair(convert_i2d(scenario[i]) / cost, i));
		}
	}
	
	auto bestfrac = top_of_n_vector(fraclist.begin(), fraclist.end(), fraclist.size(), cmpfrac);

	for (size_t i = 0; i < bestfrac.size(); i++)
	{
		int ind = bestfrac[i].second;
		double cost = adversary.getCostmap()[ind];
		if (cost <= B)
		{
			B -= cost;
			delta += scenario[ind];
		} else {
			delta += scenario[ind] * (B/cost);
			B = 0;
			return delta;
		}

	}
	return delta;
}


// Returns the sum of the budget Adversary (budget & costmap), plus some flatAdd factor. 
// All results are bounded by 1. Returns (scenario1, scenario2, higher)
std::tuple<numeric_type, numeric_type, numeric_type> solveSimple(
	std::vector<numeric_type>& scenario1, std::vector<numeric_type>& scenario2,
	numeric_type flatAdd1, numeric_type flatAdd2,
	Adversary& adversary, double Budget)
{


	numeric_type s1, s2;
	s1 = solveSimple_single(scenario1, adversary, Budget);
	s2 = solveSimple_single(scenario2, adversary, Budget);

	numeric_type one = static_cast<numeric_type>(convert_d2i(1)), max;
//	clogsn("SolveSimple:")
//	clogsn(s1 << " < s1 s2 > " << s2);
//	clogsn(flatAdd1 << " < flatAdd1 flatAdd2 > " << flatAdd2);
//	clogsn("1: " << one);
	s1 = std::min(s1 + flatAdd1, one);
	s2 = std::min(s2 + flatAdd2, one);
//	clogsn(s1 << " < s1 s2 > " << s2);
	max = std::max(s1, s2);
//	clogsn("SolveSimple max: " << max);
	return std::make_tuple(s1, s2, max);
}

// Returns the sum of the budget Adversary (budget & costmap), plus some flatAdd factor. 
// All results are bounded by 1. Returns (scenario1, scenario2, higher)
double GenericWorstCaseAnonymity::solveWithPairs(std::vector<numeric_type>& scenario1, std::vector<numeric_type>& scenario2,
	SymmetricMatrix<numeric_type>& scenario1pairs, SymmetricMatrix<numeric_type>& scenario2pairs,
	numeric_type flatAdd1, numeric_type flatAdd2,
	Adversary& adversary)
{
	double B = adversary.getBudget();
	std::vector<numeric_type> maxDeltaOfPairs1(size, 0);
	std::vector<numeric_type> maxDeltaOfPairs2(size, 0);
	for (size_t i = 0; i<size; ++i)
	{
		std::vector<numeric_type> myVectorDeltaOfPairs1;
		std::vector<numeric_type> myVectorDeltaOfPairs2;
		for (size_t j = 0; j < size; ++j)
		{
			myVectorDeltaOfPairs1.push_back(scenario1pairs[i][j]);
			myVectorDeltaOfPairs2.push_back(scenario2pairs[i][j]);
		}
		double cost = adversary.getCostmap()[i];
		if (B - cost >= 0)
		{
			double dummy1;
			double dummy2;
			std::tie(dummy1,dummy2,std::ignore) = solveSimple(myVectorDeltaOfPairs1, myVectorDeltaOfPairs2,
					0, 0, adversary, B - cost);
			maxDeltaOfPairs1[i] =dummy1/2; // "/2"
			maxDeltaOfPairs2[i] =dummy2/2;
		}
		maxDeltaOfPairs1[i] += scenario1[i];
		maxDeltaOfPairs2[i] += scenario2[i];
	}
	double resultSimpleSolver = convert_i2d(std::get<2>(solveSimple(maxDeltaOfPairs1, maxDeltaOfPairs2, flatAdd1, flatAdd2, adversary, adversary.getBudget())));
	return resultSimpleSolver;
}

// Greedily selects nodes for the budget Adversary (budget & costmap).
// The resulting nodes are put into the output vector
void GenericWorstCaseAnonymity::greedyAlgorithm(std::vector<size_t>& output,
	std::vector<numeric_type>& scenario1, std::vector<numeric_type>& scenario2,
	SymmetricMatrix<numeric_type>& scenario1pairs, SymmetricMatrix<numeric_type>& scenario2pairs,
	Adversary& adversary)
{
	std::cout << "Starting greedy algorithm..." << std::endl;
	double B = adversary.getBudget(); 
	std::vector<numeric_type> greedyDelta(size, 0);
	for (size_t i = 0; i<size; ++i)
	{
		std::vector<numeric_type> myVectorDeltaOfPairs1;
		std::vector<numeric_type> myVectorDeltaOfPairs2;
		for (size_t j = 0; j < size; ++j)
		{
			myVectorDeltaOfPairs1.push_back(scenario1pairs[i][j]);
			myVectorDeltaOfPairs2.push_back(scenario2pairs[i][j]);
		}
		double cost = adversary.getCostmap()[i];
		if (B - cost >= 0)
		{
			std::tie(greedyDelta[i],
				std::ignore,
				std::ignore) = solveSimple(myVectorDeltaOfPairs1, myVectorDeltaOfPairs2,
					0, 0, adversary, B - cost);
			greedyDelta[i] /=2; // "/2"
			greedyDelta[i] += scenario1[i];
		}
	}

	B = adversary.getBudget();

	fracvec fraclist;
	output.empty();

	for (size_t i = 0; i < scenario1.size(); i++)
	{
		double cost = adversary.getCostmap()[i];
		if (cost == 0)
		{
			output.push_back(i);
		}
		else {
			if (cost <= B)
				fraclist.push_back(std::make_pair(convert_i2d(greedyDelta[i]) / cost, i));
		}
	}

	auto bestfrac = top_of_n_vector(fraclist.begin(), fraclist.end(), fraclist.size(), cmpfrac);

	for (size_t i = 0; i < bestfrac.size(); i++)
	{
		int ind = bestfrac[i].second;
		double cost = adversary.getCostmap()[ind];
		if (cost <= B)
		{
			B -= cost;
			output.push_back(ind);
		}
	}

	std::cout << "done with greedy algorithm." << std::endl;
}

void GenericWorstCaseAnonymity::greedySenderAnonymity(std::vector<size_t>& output, Adversary& adversary) {
	return greedyAlgorithm(output, deltaPerNodeSA1, deltaPerNodeSA2, deltaIndirectPairsSA1, deltaIndirectPairsSA2, adversary);
}

void GenericWorstCaseAnonymity::greedyRecipientAnonymity(std::vector<size_t>& output, Adversary& adversary) {
	return greedyAlgorithm(output, deltaPerNodeRA1, deltaPerNodeRA2, deltaIndirectPairsRA1, deltaIndirectPairsRA2, adversary);
}

void GenericWorstCaseAnonymity::greedyRelationshipAnonymity(std::vector<size_t>& output, Adversary& adversary) {
	return greedyAlgorithm(output, deltaPerNodeRelA1, deltaPerNodeRelA2, deltaPairs1, deltaPairs2, adversary);
}


double GenericWorstCaseAnonymity::senderAnonymity(Adversary& adversary) {
	return solveWithPairs(deltaPerNodeSA1, deltaPerNodeSA2, deltaIndirectPairsSA1, deltaIndirectPairsSA2, deltaServer1, deltaServer2, adversary);

}

double GenericWorstCaseAnonymity::recipientAnonymity(Adversary& adversary) {
	return solveWithPairs(deltaPerNodeRA1, deltaPerNodeRA2, deltaIndirectPairsRA1, deltaIndirectPairsRA2, deltaISP1, deltaISP2, adversary);

}

double GenericWorstCaseAnonymity::relationshipAnonymity(Adversary& adversary) {
	return solveWithPairs(deltaPerNodeRelA1, deltaPerNodeRelA2, deltaPairs1, deltaPairs2, 0, 0, adversary);
}

void GenericWorstCaseAnonymity::printBests(const Consensus& consensus, size_t number) const
{
	std::set<std::pair<numeric_type, std::string>> bestSA;
	std::set<std::pair<numeric_type, std::string>> bestRA;
	
	for(size_t i = 0; i < size; ++i)
	{
		std::string name = consensus.getRelay(i).getName();
		bestSA.emplace(std::max(deltaPerNodeSA1[i], deltaPerNodeSA2[i]), name);
		bestRA.emplace(std::max(deltaPerNodeRA1[i], deltaPerNodeRA2[i]), name);
	}
	
	auto itSA = bestSA.rbegin();
	auto itRA = bestRA.rbegin();
	
	for(size_t i = 0; i < number; ++i, ++itSA, ++itRA)
		clogsn("[" << (i+1) << ".]\t" << "SA: \t" << itSA->first << "\t" << itSA->second << "\tRA: \t" << itRA->first << "\t" << itRA->second);	
}
