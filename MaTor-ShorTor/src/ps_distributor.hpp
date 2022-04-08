#ifndef PS_DISTRIBUTOR_HPP
#define PS_DISTRIBUTOR_HPP

/** @file */

#include <memory>

#include "tor_like.hpp"
#include "relay.hpp"
#include "consensus.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "types/general_exception.hpp"

/**
 * DistribuTor initialization exception.
 */
class distributor_init_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			ZERO_ENTRY_SUM, /**< DistribuTor computed entry sum to be zero. */
			PERCENTAGE_OOR /**< Preservation percentage is out of range. */	
		};
		
		// constructors
		/**
		 * Constructs exception instance.
		 * @param why reason of throwing exception
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		distributor_init_exception(reason why, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case ZERO_ENTRY_SUM:
					this->message = "DistribuTor failed for given consensus and specification: entry bandwidth is zero.";
					break;
				case PERCENTAGE_OOR:
					this->message = "preservation percentage is out of range; correct value should be in [0, 1] range.";
					break;
				default:
					this->message = "unknown DistribuTor failure reason.";
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~distributor_init_exception() throw () { }
		
		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
	
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "distributor_init_exception";
			return tag;
		}
};

/**
 * Class for utility of DistribuTor path selection algorithm.
 * DistribuTor is a path selection algorithm that "redistributes" entry and exit relays' bandwidths.
 * First, it reduces total exit bandwidth to given in path selection specification percantage of the original bandwidth (to the goal bandwidth). 
 * Then, it goes through all relays from the ones with the smallest bandwidth up to the ones with the greatest bandwidth.
 * Class operates on weights which are defined as bandwidth \* maxConsensusModifier (i. e., 10000).
 * As long as tested relay's bandwidth can replace bandwidth of larger relays without exceeding goal bandwidth,
 * relay's original bandwidth is preserved. Otherwise, the bandwidth of all previously tested relays is subtracted from the goal bandwidth,
 * and the remaining value is equally distributed to the currently tested relay and all relays with higher bandwidth. As a result, bandwidth gets flattened for larger relays. The relays which bandwidth
 * got flattened use the rest of their original bandwidth as entry bandwidth. The same algorithm is performed regarding entry bandwidth.
 * All relays which bandwidth was not flattened, are used solely as exit nodes and have entry bandwidth equal to 0.
 */ 
class PSDistribuTor : public TorLike
{
	friend class PSAS;
	
	public:
		// constructors
		/**
		 * Initializes path selection in a Tor-like way and redistributes weights.
		 * @see TorLike()
		 */
		PSDistribuTor(
			std::shared_ptr<PSDistribuTorSpec> pathSelectionSpec, 
			std::shared_ptr<SenderSpec> senderSpec, 
			std::shared_ptr<RecipientSpec> recipientSpec,
			std::shared_ptr<RelationshipManager> relationshipManager,
			const Consensus &consensus);
	
	protected:
		// functions
		virtual weight_t getExitWeight(const Relay& relay)const;
		virtual weight_t getEntryWeight(const Relay& relay) const;
		virtual weight_t getMiddleWeight(const Relay& relay) const;
	
	private:
		// variabes
		const int modifier; /**< Modifier is used to cast integer bandwidth to 100% preserved consensus weight. */
		weight_t maxEntryWeight; /**< Maximal possible entry weight for a single relay. */
		weight_t maxExitWeight; /**< Maximal possible exit weight for a single relay. */
		
		// subclasses
		/**
		 * Comparator for exit bandwidth. Relays with zero exit weight are the smallest.
		 * Relay with non-zero exit weight is greater than another such relay
		 * if it's bandwidth is greater.
		 */
		class compBWExit
		{
			public:
				/**
				 * @param exitWeights relay's "vanilla" exit weights
				 * @param consensus consensus reference
				 */ 
				compBWExit(const std::vector<weight_t>& exitWeights, const Consensus& consensus)
					: exitWeights(exitWeights), consensus(consensus) { }
				
				bool operator() (size_t i, size_t j) const;
				
			private:
				const std::vector<weight_t>& exitWeights;
				const Consensus& consensus;
		};
		
		/**
		 * Comparator for entry bandwidth. Relays with zero entry weight are the smallest.
		 * Value of relays which can be used as exit nodes is reduced by max exit bandwidth.
		 * Modified relay is greater than another if it's bandwidth is greater.
		 */
		class compBWEntry
		{
			public:
				/**
				 * @param exitWeights relay's "vanilla" exit weights
				 * @param entryWeights relay's "vanilla" entry weights
				 * @param consensus consensus reference
				 * @param maxExitBW maximal exit bandwidth allowed for relay
				 */ 
				compBWEntry(const std::vector<weight_t>& exitWeights, const std::vector<weight_t>& entryWeights, const Consensus& consensus, weight_t maxExitBW)
					: exitWeights(exitWeights), entryWeights(entryWeights), consensus(consensus), maxExitBW(maxExitBW) { }
					
				bool operator() (size_t i, size_t j) const;
				
			private:
				const std::vector<weight_t>& exitWeights;
				const std::vector<weight_t>& entryWeights;
				const Consensus& consensus;
				weight_t maxExitBW;
		};
};

#endif
