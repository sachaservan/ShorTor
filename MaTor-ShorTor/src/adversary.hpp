#ifndef ADVERSARY_HPP
#define ADVERSARY_HPP

/** @file */

#include "costmap.hpp"

/**
 * Anonymity game adversary.
 * Regarding worst case adversary, 
 * adversary class defines costs of relays and adversary capability to compromise them (budget). 
 */ 
class Adversary
{
	public:
		// constructors
		/**
		 * Creates adversary with 0 budget and empty costmap.
		 */
		Adversary() : budget(0) { }
		
		// getters
		/**
		 * @return adversary's budget.
		 */
		virtual double getBudget() { return budget; }
		/**
		 * @return relays costs map.
		 */
		virtual Costmap& getCostmap() { return costmap; }
		
		// setters
		/**
		 * @param budget budget for adversary.
		 */
		virtual void setBudget(double budget) { this->budget = budget; }
		
	private:
		// variables
		double budget; /**< Adversary's budget. */
		Costmap costmap; /**< Costmap of network relays. */
};

/**
 * Simplified k-of-n adversary
 */ 
class KofNAdversary : public Adversary
{
	public:
		// constructors
		/**
		 * Creates adversary with 0 budget and empty costmap.
		 */
		KofNAdversary() : Adversary() { }
		
		/**
		 * KofNAdversary ConstCostCostmap reference. All changes to the cost map are ignored.
		 * @return relays costs map.
		 */
		virtual Costmap& getCostmap() { return costmap; }
		
	private:
		struct ConstCostCostmap : public Costmap
		{
			virtual double operator[] (int relayPos) { return 1; }
		};
		ConstCostCostmap costmap;
};

#endif

