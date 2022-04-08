#ifndef COSTMAP_HPP
#define COSTMAP_HPP

/** @file */

#include <map>
#include <vector>
#include <memory>

class ProgrammableCostFunction;
class Relay;

/**
 * Class contains relay's costs information for adversary.
 * Costs may be modified by multiple programmable cost functions.
 */
class Costmap
{
	public:
		// constructors
		/**
		 * Creates costmap without programmable cost functions and relay-cost assignments.
		 */
		Costmap() { }
		
		// operators
		/**
		 * Allows to access map item with [] operator.
		 * If accessed item doesn't exist, it will be created and initialized with default value.
		 * @param relayPos relay position in consensus relays vector.
		 * @return specified relay's cost.
		 */ 
		virtual double operator[] (size_t relayPos);
		
		// functions
		/**
		 * Adds programmable cost function to PCF list.
		 * The PCF is ignored before calling commit method.
		 * @see commit()
		 * @param pcf pointer to the programmable cost function to be added.
		 */
		virtual void addPCF(std::shared_ptr<ProgrammableCostFunction> pcf);
		/**
		 * Computes costs based on programmable cost functions for specified relays.
		 * This function should be called after all desired PCFs have been added
		 * and before anonymity guarantees are computed. If the function is not called
		 * after adding new PCF / set of PCFs, these PCFs are ignored.
		 * @param relays vector of relays which costs will be computed.
		 */
		virtual void commit(const std::vector<Relay>& relays);
		/**
		 * Deletes all the PCFs.
		 * This change is ignored before calling commit method.
		 * @see commit()
		 */
		virtual void reset();
		/**
		 * Prints all the PCFs in PCF vector.
		 */
		virtual void printPCFs();

		virtual bool isConstant();

		virtual bool isInitialized(int numRelays = 1);
	
	private:
		// variables
		std::vector<double> costs = std::vector<double>(0, 1.0); /**< Maps relay's position in consensus relays vector to computed relay's cost. */
		std::vector<std::shared_ptr<ProgrammableCostFunction>> pcfs; /**< Vector of programmable cost functions. */
};

#endif

