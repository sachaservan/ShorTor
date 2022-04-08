#include "generic_precise_anonymity.hpp"
#include "types/const_vector.hpp"
#include "types/work_manager.hpp"
#include "utils.hpp"
#include <numeric>
#include <math.h>       /* log */

constexpr size_t uint_precision = sizeof(numeric_type) * 8 * 15 / 16; // *15/16 == make 64 -> 60, 32 -> 30, etc...
constexpr probability_t conversion_const = (1ull << uint_precision);
constexpr probability_t conversion_const_inv = 1.0 / conversion_const;

// Anonymity Indexes
const int SA1 = 0;
const int SA2 = 1;
const int RA1 = 2;
const int RA2 = 3;
const int REL1 = 4;
const int REL2 = 5;

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

static bool checkObservation(observation o, const bool& SG, const bool& GM, const bool& MX, const bool& XR)
{
	return (o[0] == (SG)) && (o[1] == (SG||GM)) && (o[2] == (GM||MX)) && (o[3] == (MX||XR) && (o[4] == XR));
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
	//if(stest)
	Probability stest = scenario1 - scenario2;
	{
		//if(stest > 0)
		if(stest > 0)
			function(accumScenario1, stest);
		else
			function(accumScenario2, -stest);
	}
}

numeric_type Phi(numeric_type X, numeric_type Y)
{
	if (X > Y)
		return X - Y;
	return 0;

}

static void addDeltasSA(const_vector<numeric_type>& myDelta, numeric_type prA1, numeric_type prB1, observation o)
{
	//myassert(o[0] && o[1] && o[2] && o[3] && o[4]);
	myDelta[SA1] += o[0] ? prA1 : Phi(prA1, prB1);
	myDelta[SA2] += o[0] ? prB1 : Phi(prB1, prA1);
}

static void addDeltasRA(const_vector<numeric_type>& myDelta, numeric_type prA1, numeric_type prA2, observation o)
{
	myDelta[RA1] += o[4] ? prA1 : Phi(prA1, prA2);
	myDelta[RA2] += o[4] ? prA2 : Phi(prA2, prA1);
}

static void addDeltasREL(const_vector<numeric_type>& myDelta, numeric_type prA1, numeric_type prA2, numeric_type prB1, numeric_type prB2, observation o)
{

	if (o[0] && o[4]) // We can see sender and recipient, so we immediately distinguish for REL
	{
		myDelta[REL1] += (prA1 + prB2)/2;
		myDelta[REL2] += (prA2 + prB1)/2;
	}
	else
	{
		if (o[0]) // we can see the sender, so we distinguish depending on recipient
		{
			myDelta[REL1] += (Phi(prA1, prA2) + Phi(prB2, prB1)) /2;
			myDelta[REL2] += (Phi(prA2, prA1) + Phi(prB1, prB2)) /2;
		}
		else
		{
			if (o[4]) // we can see the recipient, so we distinguish depending on the sender
			{
				myDelta[REL1] += (Phi(prA1, prB1) + Phi(prB2, prA2)) /2;
				myDelta[REL2] += (Phi(prA2, prB2) + Phi(prB1, prA1)) /2;
			}
			else { // we see neither sender nor recipient, so we have to make a full "Phi"
				myDelta[REL1] += Phi((prA1 + prB2)/2, (prA2 + prB1)/2);
				myDelta[REL2] += Phi((prA2 + prB1)/2, (prA1 + prB2)/2);
			}

		}
	}
}

static void handleAndAddToDelta(const_vector<numeric_type>& myDelta, observation o, numeric_type& SA_A1, numeric_type& SA_B1, numeric_type& RA_A1, numeric_type& RA_A2, numeric_type& REL_A1, numeric_type& REL_A2, numeric_type& REL_B1, numeric_type& REL_B2)
{
	if (SA_A1 + SA_B1 > 0)
		addDeltasSA(myDelta, SA_A1, SA_B1, o);
	if (RA_A1 + RA_A2 > 0)
		addDeltasRA(myDelta, RA_A1, RA_A2, o);
	if (REL_A1 + REL_A2 + REL_B1 + REL_B2 > 0)
		addDeltasREL(myDelta, REL_A1, REL_A2, REL_B1, REL_B2, o);
	SA_A1 = 0;
	SA_B1 = 0;
	RA_A1 = 0;
	RA_A2 = 0;
	REL_A1 = 0;
	REL_A2 = 0;
	REL_B1 = 0;
	REL_B2 = 0;
}

static void handleInnermost(const_vector<numeric_type>& myDelta, observation o, numeric_type prA1, numeric_type prA2, numeric_type prB1, numeric_type prB2, bool AG, bool BG, bool GM, bool MX, bool X1, bool X2)
{
	numeric_type SA_A1 = checkObservation(o, AG, GM, MX, true) ? prA1 : 0;
	numeric_type SA_B1 = checkObservation(o, BG, GM, MX, true) ? prB1 : 0;
	numeric_type RA_A1 = checkObservation(o, true, GM, MX, X1) ? prA1 : 0;
	numeric_type RA_A2 = checkObservation(o, true, GM, MX, X2) ? prA2 : 0;

	numeric_type REL_A1 = checkObservation(o, AG, GM, MX, X1) ? prA1 : 0;
	numeric_type REL_A2 = checkObservation(o, AG, GM, MX, X2) ? prA2 : 0;
	numeric_type REL_B1 = checkObservation(o, BG, GM, MX, X1) ? prB1 : 0;
	numeric_type REL_B2 = checkObservation(o, BG, GM, MX, X2) ? prB2 : 0;

	handleAndAddToDelta(myDelta, o, SA_A1, SA_B1, RA_A1, RA_A2, REL_A1, REL_A2, REL_B1, REL_B2);
}



static void set_obstask(obstask& obs, bool S, bool G, bool M, bool X, bool R, int i)
{
	obs.first[0] = S;
	obs.first[1] = G;
	obs.first[2] = M;
	obs.first[3] = X;
	obs.first[4] = R;
	obs.second = i;
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
// NOTE: Senders A & B, Recipients 1 & 2
// E.g., A1 = Sender A talking to recipient 1
// via dict is a dictionary between pairs of nodes and their fastest vias (if any)
GenericPreciseAnonymity::GenericPreciseAnonymity(
	const Consensus& consensus,
	const PathSelection& psA1,
	const PathSelection& psA2,
	const PathSelection& psB1,
	const PathSelection& psB2,
	const_vector<const_vector<bool>>& observedNodes,
	const_vector<bool>& observedSenderA,
	const_vector<bool>& observedSenderB,
	const_vector<bool>& observedRecipient1,
	const_vector<bool>& observedRecipient2,
	double epsilon) : size(consensus.getSize())
	{

	if (epsilon != 1) {
		NOT_IMPLEMENTED;
	}
	// the following variables are used as accumulators for multi-threaded computation
	// atomic type for multicore concurrent computation.
	
	atomic_type deltaSA1(0);
	atomic_type deltaSA2(0);
	atomic_type deltaRA1(0);
	atomic_type deltaRA2(0);
	atomic_type deltaREL1(0);
	atomic_type deltaREL2(0);

	atomic_type prEmptyObsA1(0);
	atomic_type prEmptyObsA2(0);
	atomic_type prEmptyObsB1(0);
	atomic_type prEmptyObsB2(0);


	const int LOOP_G = 0;
	const int LOOP_M = 1;
	const int LOOP_X = 2;

	// All observations: and the order in which the loops are nested
	// Index 0 Loops: XMG
	// Index 1 Loops: GMX
	// Index 2 Loops: XGM
	obstask obstasks[3][4];
	int translation[3][3];
	translation[0][0] = LOOP_X; translation[0][1] = LOOP_M; translation[0][2] = LOOP_G;
	translation[1][0] = LOOP_G; translation[1][1] = LOOP_M; translation[1][2] = LOOP_X;
	translation[2][0] = LOOP_X; translation[2][1] = LOOP_G; translation[2][2] = LOOP_M;


	//					 	    S  G  M  X  R  index
	set_obstask(obstasks[0][0], 0, 0, 0, 0, 0, 0); // The empty observation												has to be handled outside anyway
	set_obstask(obstasks[0][1], 0, 0, 0, 1, 1, 1); // R is compromised or XR is observed								Requires X**	has to be handled in the 1st loop (per X)
	set_obstask(obstasks[0][2], 0, 0, 1, 1, 0, 2); // MX is observed													Requires MX*	has to be handled in the 2nd loop (per MX)
	set_obstask(obstasks[0][3], 0, 0, 1, 1, 1, 2); // X is compromised or MX and XR are observed						Requires MX*	has to be handled in the 2nd loop (per MX)
	set_obstask(obstasks[1][0], 0, 1, 1, 0, 0, 2); // GM is observed													Requires GM*	has to be handled in the 2nd loop (per GM)
	set_obstask(obstasks[2][0], 0, 1, 1, 1, 0, 3); // M is compromised or GM and MX are observed						Requires ALL	has to be handled in the 3rd loop (per GMX)
	set_obstask(obstasks[1][1], 0, 1, 1, 1, 1, 3); // M and X or M and R are compromised or GM, MX, XR are observed		Requires ALL	has to be handled in the 3rd loop (per GMX)
	set_obstask(obstasks[1][2], 1, 1, 0, 0, 0, 1); // S is compromised or SG is observed								Requires G**	has to be handled in the 1st loop (per G)
	set_obstask(obstasks[2][1], 1, 1, 0, 1, 1, 2); // S and R are compromised or SG and XR are observed					Requires GX*	has to be handled in the 2nd loop (per GX)
	set_obstask(obstasks[1][3], 1, 1, 1, 0, 0, 2); // G compromised or SG and GM are observed							Requires GM*	has to be handled in the 2nd loop (per GM)
	set_obstask(obstasks[2][2], 1, 1, 1, 1, 0, 3); // G and M are compromised or SG, GM, MX are observed				Requires ALL	has to be handled in the 3rd loop (per GMX)
	set_obstask(obstasks[2][3], 1, 1, 1, 1, 1, 3); // Everything is observed.											Requires ALL	has to be handled in the 3rd loop (per GMX)

	// Partition the job into chunks
	// The larger the chunks, the less tasks, but might cause unbalanced work distribution
	constexpr size_t chunk_size = 16;
	
	WorkManager manager;
	// Run separate loops for all obstasks, as this defines the order of the loops
	for (int obstaskindex = 0; obstaskindex < 3; obstaskindex++)
	{
		std::cout << "Starting loop " << obstaskindex + 1 << " of " << "3" << std::endl;

		for(size_t i = 0; i < size; i += chunk_size)
		{
			size_t begin = i, end = begin + chunk_size;
			// last chunk: stop at size, don't go further
			if(end > size) end = size;
				manager.addTask([&, begin, end, obstaskindex](){
				const_vector<numeric_type> SAObsProbsA1(4, 0); // Sender A talking to Recipient 1
				const_vector<numeric_type> SAObsProbsB1(4, 0); // Sender B talking to Recipient 1
				const_vector<numeric_type> RAObsProbsA1(4, 0); // Sender A talking to Recipient 1
				const_vector<numeric_type> RAObsProbsA2(4, 0); // Sender A talking to Recipient 2
				const_vector<numeric_type> RELObsProbsA1(4, 0); // Sender A talking to Recipient 1
				const_vector<numeric_type> RELObsProbsA2(4, 0); // Sender A talking to Recipient 2
				const_vector<numeric_type> RELObsProbsB1(4, 0); // Sender B talking to Recipient 1
				const_vector<numeric_type> RELObsProbsB2(4, 0); // Sender B talking to Recipient 2

				const_vector<numeric_type> myDelta(6, 0);
				size_t guard_index;
				size_t middle_index;
				size_t exit_index;
				size_t circuit[3];
				bool AG, BG, GM, MX, X1, X2;
				for(size_t l0 = begin; l0 < end; ++l0) // Outermost loop -- node type depends on the translation
				{
					// [Optimization]: there are fewer exists than guards so worth checking exit relay first
					if (translation[obstaskindex][0] == LOOP_X && !consensus.getRelay(l0).hasFlags(RelayFlag::EXIT))
						continue;
					if (translation[obstaskindex][0] == LOOP_G && !consensus.getRelay(l0).hasFlags(RelayFlag::GUARD))
						continue;

					for(size_t l1 = 0; l1 < size; ++l1)
					{
						if (l0 == l1)
							continue;
						if (translation[obstaskindex][1] == LOOP_X && !consensus.getRelay(l1).hasFlags(RelayFlag::EXIT))
							continue;
						if (translation[obstaskindex][1] == LOOP_G && !consensus.getRelay(l1).hasFlags(RelayFlag::GUARD))
							continue;

						for(size_t l2 = 0; l2 < size; ++l2)
						{
							if (l0 == l2 || l1 == l2)
								continue;

							if (translation[obstaskindex][2] == LOOP_X && !consensus.getRelay(l2).hasFlags(RelayFlag::EXIT))
								continue;
							if (translation[obstaskindex][2] == LOOP_G && !consensus.getRelay(l2).hasFlags(RelayFlag::GUARD))
								continue;

							circuit[translation[obstaskindex][0]] = l0;
							circuit[translation[obstaskindex][1]] = l1;
							circuit[translation[obstaskindex][2]] = l2;
							guard_index = circuit[0];
							middle_index = circuit[1];
							exit_index = circuit[2];

							probability_t circuitProbA1 = psA1.exitProb(exit_index) * psA1.entryProb(guard_index, exit_index) * psA1.middleProb(middle_index, guard_index, exit_index);
							probability_t circuitProbA2 = psA2.exitProb(exit_index) * psA2.entryProb(guard_index, exit_index) * psA2.middleProb(middle_index, guard_index, exit_index);
							probability_t circuitProbB1 = psB1.exitProb(exit_index) * psB1.entryProb(guard_index, exit_index) * psB1.middleProb(middle_index, guard_index, exit_index);
							probability_t circuitProbB2 = psB2.exitProb(exit_index) * psB2.entryProb(guard_index, exit_index) * psB2.middleProb(middle_index, guard_index, exit_index);
						
							// gmxP is the probability of selecting (g)uard (m)iddle and (e)xit (the circuit
							// NOT the conditional probability of middleProb.
							numeric_type conv_gmxPA1 = convert_d2i(circuitProbA1);
							numeric_type conv_gmxPA2 = convert_d2i(circuitProbA2);
							numeric_type conv_gmxPB1 = convert_d2i(circuitProbB1);
							numeric_type conv_gmxPB2 = convert_d2i(circuitProbB2);
					
							// Test, whether the circuit can be selected in any scenario, continue otherwise
							if(!(conv_gmxPA1 || conv_gmxPA2 || conv_gmxPB1 || conv_gmxPB2))
								continue;

							// Check which positions are observed
							// hence we don't need to update observations for others. 
							AG = observedSenderA[guard_index]; 
							BG = observedSenderB[guard_index];
							X1 = observedRecipient1[exit_index];
							X2 = observedRecipient2[exit_index];
							GM = observedNodes[guard_index][middle_index];
							MX = observedNodes[middle_index][exit_index];

							// check whether a relevant observation was made:
							for (int i = 0; i < 4; i++)
							{
								if (obstasks[obstaskindex][i].second == 3) // We have to handle the observation in this innermost loop; no need to sum something up
								{ 
									handleInnermost(myDelta, obstasks[obstaskindex][i].first, conv_gmxPA1, conv_gmxPA2, conv_gmxPB1, conv_gmxPB2, AG, BG, GM, MX, X1, X2);
								}
								else
								{
									if (checkObservation(obstasks[obstaskindex][i].first, AG, GM, MX, true))
										SAObsProbsA1[i] += conv_gmxPA1;
									if (checkObservation(obstasks[obstaskindex][i].first, BG, GM, MX, true))
										SAObsProbsB1[i] += conv_gmxPB1;
									if (checkObservation(obstasks[obstaskindex][i].first, true, GM, MX, X1))
										RAObsProbsA1[i] += conv_gmxPA1;
									if (checkObservation(obstasks[obstaskindex][i].first, true, GM, MX, X2))
										RAObsProbsA2[i] += conv_gmxPA2;
									if (checkObservation(obstasks[obstaskindex][i].first, AG, GM, MX, X1))
										RELObsProbsA1[i] += conv_gmxPA1;
									if (checkObservation(obstasks[obstaskindex][i].first, AG, GM, MX, X2))
										RELObsProbsA2[i] += conv_gmxPA2;
									if (checkObservation(obstasks[obstaskindex][i].first, BG, GM, MX, X1))
										RELObsProbsB1[i] += conv_gmxPB1;
									if (checkObservation(obstasks[obstaskindex][i].first, BG, GM, MX, X2))
										RELObsProbsB2[i] += conv_gmxPB2;
								}
							}
						} // End of innermost loop (L = 3)
						for (int i = 0; i < 4; i++)
						{
							if (obstasks[obstaskindex][i].second == 2) // We have to handle the observation in this loop
							{
								handleAndAddToDelta(myDelta, obstasks[obstaskindex][i].first, SAObsProbsA1[i], SAObsProbsB1[i], RAObsProbsA1[i], RAObsProbsA2[i], RELObsProbsA1[i], RELObsProbsA2[i], RELObsProbsB1[i], RELObsProbsB2[i]);
							}
						}
					}// End of middle loop (L = 2)
					for (int i = 0; i < 4; i++)
					{
						if (obstasks[obstaskindex][i].second == 1) // We have to handle the observation in this loop
						{
							handleAndAddToDelta(myDelta, obstasks[obstaskindex][i].first, SAObsProbsA1[i], SAObsProbsB1[i], RAObsProbsA1[i], RAObsProbsA2[i], RELObsProbsA1[i], RELObsProbsA2[i], RELObsProbsB1[i], RELObsProbsB2[i]);
						}
					}

				}
				for (int i = 0; i < 4; i++)
				{
					if (obstasks[obstaskindex][i].second == 0) // The empty observation -- we have to store the probabilies
					{
						prEmptyObsA1.fetch_add(RELObsProbsA1[i]);
						prEmptyObsA2.fetch_add(RELObsProbsA2[i]);
						prEmptyObsB1.fetch_add(RELObsProbsB1[i]);
						prEmptyObsB2.fetch_add(RELObsProbsB2[i]);
					}
				}
				// Finally store all the deltas we accumulated in the loop:
				deltaSA1.fetch_add(myDelta[SA1]);
				deltaSA2.fetch_add(myDelta[SA2]);
				deltaRA1.fetch_add(myDelta[RA1]);
				deltaRA2.fetch_add(myDelta[RA2]);
				deltaREL1.fetch_add(myDelta[REL1]);
				deltaREL2.fetch_add(myDelta[REL2]);

			});
		}

		std::cout << "Finished delegating loop " << obstaskindex + 1 << " of " << "3" << std::endl;
	}

 	// Run prepared jobs:
	std::cout << "Main loop starts." << std::endl;
	manager.startAndJoinAll();
	std::cout << "Main loop done." << std::endl;
	
	// Finally compute the impact of the empty observation.
	const_vector<numeric_type> myDeltaForEmptyObs(6, 0);
	observation emptyObs = { 0,0,0,0,0 };
	numeric_type dummy = 0;
	numeric_type emptyObsProbA1 = prEmptyObsA1.load(std::memory_order::memory_order_relaxed);
	numeric_type emptyObsProbA2 = prEmptyObsA2.load(std::memory_order::memory_order_relaxed);
	numeric_type emptyObsProbB1 = prEmptyObsB1.load(std::memory_order::memory_order_relaxed);
	numeric_type emptyObsProbB2 = prEmptyObsB2.load(std::memory_order::memory_order_relaxed);
	handleAndAddToDelta(myDeltaForEmptyObs, emptyObs, dummy, dummy, dummy, dummy, emptyObsProbA1, emptyObsProbA2, emptyObsProbB1, emptyObsProbB2);
	deltaREL1.fetch_add(myDeltaForEmptyObs[REL1]);
	deltaREL2.fetch_add(myDeltaForEmptyObs[REL2]);

	myassert(abs(deltaSA1.load(std::memory_order::memory_order_relaxed) - deltaSA2.load(std::memory_order::memory_order_relaxed)) < 0.0001);
	myassert(abs(deltaRA1.load(std::memory_order::memory_order_relaxed) - deltaRA2.load(std::memory_order::memory_order_relaxed)) < 0.0001);
	myassert(abs(deltaREL1.load(std::memory_order::memory_order_relaxed)-deltaREL2.load(std::memory_order::memory_order_relaxed)) < 0.0001);

	deltaSA = deltaSA1.load(std::memory_order::memory_order_relaxed);
	deltaRA = deltaRA1.load(std::memory_order::memory_order_relaxed);
	deltaREL = deltaREL1.load(std::memory_order::memory_order_relaxed);

	std::cout << "deltaSA: " << deltaSA << std::endl;
	std::cout << "deltaRA: " << deltaRA << std::endl;
	std::cout << "deltaREL: " << deltaREL << std::endl;
}


// Returns the delta for sender anonymity
double GenericPreciseAnonymity::senderAnonymity() {
	return convert_i2d(deltaSA);
}

// Returns the delta for recipient anonymity
double GenericPreciseAnonymity::recipientAnonymity() {
	return convert_i2d(deltaRA);
}

// Returns the delta for relationship anonymity
double GenericPreciseAnonymity::relationshipAnonymity() {
	return convert_i2d(deltaREL);
}

