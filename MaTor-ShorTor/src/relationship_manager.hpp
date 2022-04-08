#ifndef RELATIONSHIP_MANAGER_HPP
#define RELATIONSHIP_MANAGER_HPP

/** @file */

#include <vector>
#include <set>
#include <memory>

#include "ip.hpp"
#include "consensus.hpp"

#include "types/symmetric_matrix.hpp"

/**
 * Virtual utility class for managing relay's relationships in Path Selection algorithm.
 * Defines relationship and assigns additional relationship constraints.
 * @see PathSelection
 */
class RelationshipManager
{
	public:
		/**
		 * Assigns additional constraints to relations and modifies relays' possibilities.
		 * By default, it does nothing.
		 * @param exitPossible reference to the vector of exit possibilities (based on path selection algorithm).
		 * @param entryPossible reference to the vector of entry possibilities (based on path selection algorithm).
		 */
		virtual void assignConstraints(std::vector<bool>& exitPossible, std::vector<bool>& entryPossible) { }
		
		/**
		 * @param exitPosition exit relay index in consensus
		 * @param entryPosition entry relay index in consensus
		 * @return true iff there is a relation between exit and entry relay.
		 */
		virtual bool exitEntryRelated(size_t exitPosition, size_t entryPosition) = 0;
		/**
		 * @param exitPosition exit relay index in consensus
		 * @param middlePosition middle relay index in consensus
		 * @return true iff there is a relation between exit and middle relay.
		 */
		virtual bool exitMiddleRelated(size_t exitPosition, size_t middlePosition) = 0;
		/**
		 * @param entryPosition entry relay index in consensus
		 * @param middlePosition middle relay index in consensus
		 * @return true iff there is a relation between entry and middle relay.
		 */
		virtual bool entryMiddleRelated(size_t entryPosition, size_t middlePosition) = 0;
};

/**
 * Assign relations exactly as defined in consensus instance
 * (based on families from consensus file and server descriptors only)
 */
class ConsensusRelations : public RelationshipManager
{
	public:
		/**
		 * Constructor saves consensus which will be used for relationship computations.
		 * @param consensus Consensus instance
		 */
		ConsensusRelations(const Consensus& consensus) : consensus(consensus) { }
		
		virtual bool exitEntryRelated(size_t exitPosition, size_t entryPosition)
		{
			return consensus.isRelated(exitPosition, entryPosition);
		}
		virtual bool exitMiddleRelated(size_t exitPosition, size_t middlePosition)
		{
			return consensus.isRelated(exitPosition, middlePosition);
		}
		virtual bool entryMiddleRelated(size_t entryPosition, size_t middlePosition)
		{
			return consensus.isRelated(entryPosition, middlePosition);
		}
		
	protected:
		const Consensus& consensus; /**< Referecnce to consensus instance. */
};

/**
 * Computes relations based on consensus families and relays' subnets.
 * (two relays from the same subnet are regarded as related).
 */
class SubnetRelations : public RelationshipManager
{
	public:
		/**
		 * Constructor saves consensus which will be used for relationship computations.
		 * Relationship is initialized.
		 * @param consensus Consensus instance
		 */
		SubnetRelations(const Consensus& consensus);
		
		virtual bool exitEntryRelated(size_t exitPosition, size_t entryPosition)
		{
			return relations[exitPosition][entryPosition];
		}
		virtual bool exitMiddleRelated(size_t exitPosition, size_t middlePosition)
		{
			return relations[exitPosition][middlePosition];
		}
		virtual bool entryMiddleRelated(size_t entryPosition, size_t middlePosition)
		{
			return relations[entryPosition][middlePosition];
		}
	protected:
		const Consensus& consensus; /**< Referecnce to consensus instance. */
		SymmetricMatrix<bool> relations; /**< Precomputed relations matrix. */
};

/**
 * Computes relations based on consensus families, relays' subnets and Autonomous Systems in circuit.
 * Computed relations are not necesarilly symmetric.
 * Only exit and entry relations are affected by autonomous systems,
 * other relations are standard subnet relations.
 * Relays are AS related, if paths sender-entry and exit-recipient cross at least one same AS. 
 */
class ASRelations : public SubnetRelations
{
	public:
		/**
		 * Initializes variables. Standard subnet relations are computed.
		 * @param consensus consensus instance
		 * @param senderIP IP adress of a sender in a circuit.
		 * @param recipientIP recipient IP address in a circuit.
		 */
		ASRelations(const Consensus& consensus, const IP& senderIP, const IP& recipientIP);
		
		virtual void assignConstraints(std::vector<bool>& exitPossible, std::vector<bool>& entryPossible);
		
		virtual bool exitEntryRelated(size_t exitPosition, size_t entryPosition)
		{
			return ASrelations[exitPosition][entryPosition];
		}
	
	private:
		std::vector<std::vector<bool>> ASrelations; /**< AS relations matrix. */
		std::vector<bool> canBeExit; /**< Can relay be an exit node? */
		std::vector<bool> canBeEntry; /**< Can relay be an entry node? */
		
		const IP& senderIP; /**< Sender's IP address. */
		const IP& recipientIP; /**< Recipient's IP address. */
		
		/**
		 * Symmulates route between two IP addresses in a network.
		 * // TODO: since we have no nice way of achieving this goal at the moment, this function returns empty set.
		 * @param startIP start IP address
		 * @param endIP end IP address
		 * @return set of AS names crossed in a route.
		 */
		std::set<std::string> tracert(const IP& startIP, const IP& endIP);
};

/**
 * Class combining relationship managers.
 */
class CombinedRelations : public RelationshipManager
{
	public:
		/**
		 * Constructor saves pointers to two relationship managers which relations it will take into account.
		 * @param r1 relationship manager 
		 * @param r2 another relationship manager
		 */ 
		CombinedRelations(std::shared_ptr<RelationshipManager> r1, std::shared_ptr<RelationshipManager> r2) : r1(r1), r2(r2) { }
	private:
		std::shared_ptr<RelationshipManager> r1;
		std::shared_ptr<RelationshipManager> r2;
		
		virtual bool exitEntryRelated(size_t exitPosition, size_t entryPosition)
		{
			return r1->exitEntryRelated(exitPosition, entryPosition) || r2->exitEntryRelated(exitPosition, entryPosition);
		}
		virtual bool exitMiddleRelated(size_t exitPosition, size_t middlePosition)
		{
			return r1->exitEntryRelated(exitPosition, middlePosition) || r2->exitEntryRelated(exitPosition, middlePosition);
		}
		virtual bool entryMiddleRelated(size_t entryPosition, size_t middlePosition)
		{
			return r1->exitEntryRelated(entryPosition, middlePosition) || r2->exitEntryRelated(entryPosition, middlePosition);
		}
};

#endif
