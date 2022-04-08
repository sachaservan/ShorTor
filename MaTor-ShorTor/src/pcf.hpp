#ifndef PCF_HPP
#define PCF_HPP

/** @file  */

#include <functional>

#include "pcf_parser.hpp"
#include "relay.hpp"
#include "json.hpp"


typedef std::function<double(const Relay&, double)> pcfEffect; /**< The PCF effect type signature, signifying a function that maps a double relay cost to an updated relay cost.*/

typedef std::function<bool(const Relay&)> pcfPredicate; /**< The PCF predicate type signature, a function which evaluates a predicate on a relay, yielding a bool.*/

class ProgrammableCostFunction : public std::enable_shared_from_this<ProgrammableCostFunction>
{

public:
	/** Constructor for PCF.*/
	ProgrammableCostFunction() {};

	/** Constructor for PCF which parses a string expression into a PCF.
	* @param expression String which contains PCF expression.*/
	ProgrammableCostFunction(std::string expression);

	ProgrammableCostFunction(int, pcfEffect effect);

	/** Applies the PCF. If the PCF is applicable, the initial relay cost
	 * is manipulated according to the PCF effect.
	 * @param r Relay which is considered for PCF application
	 * @param initialRelayCost relay cost prior to PCF application
	 * @return The new relay cost.
	 * */
	double apply(const Relay& r, double initialRelayCost);

	/** Prints a description of the PCF. */
	void print();

	/** Reads a PCF from a JSON string.
	 * @param filename Path to file that holds the PCF JSON object*/
	void fromJSONFile(const std::string& filename);

	/** Serialization of a PCF object to a JSON string.
	 * @param filename Path to file that should hold the PCF JSON object*/
	void toJSONFile(const std::string& filename);

private:
	std::vector<std::pair<pcfPredicate, pcfEffect>> subPCFs;
	nlohmann::json jsonObject; /**< The JSON representation of the PCF. */
	std::shared_ptr<std::vector<pcfparser::PCFToken>> tokens; /**< Token stream resulting from lexing a PCF expression string. */

	static pcfPredicate defaultPredicate;
};
#endif

