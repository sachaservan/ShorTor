#include "pcf.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include "utils.hpp"

ProgrammableCostFunction::ProgrammableCostFunction(std::string expression) {
	pcfparser::PCFParser parser(expression);
	for (auto pcf : parser.getPCFs()) {
		subPCFs.push_back(pcf);
	}
}

pcfPredicate ProgrammableCostFunction::defaultPredicate = [](const Relay& r) { return true; };

ProgrammableCostFunction::ProgrammableCostFunction(int, pcfEffect effect) {
	subPCFs.push_back(std::pair<pcfPredicate, pcfEffect>(defaultPredicate, effect));
}




void ProgrammableCostFunction::fromJSONFile(const std::string& filename) {
	NOT_IMPLEMENTED;
}

double ProgrammableCostFunction::apply(const Relay& r, double initialRelayCost) {
	if (subPCFs.empty())
		return initialRelayCost;

	for (std::pair<pcfPredicate, pcfEffect> pair : subPCFs) {
		pcfPredicate predicate = pair.first;
		pcfEffect effect = pair.second;

		// This is a hack. We should have a better parser. Seriously.
		/*if (predicate(r))
			return effect(r, initialRelayCost);
		else
			return initialRelayCost;*/
		// That's my idea of a semantic
		if (predicate(r))
			initialRelayCost = effect(r, initialRelayCost);
	}
	return initialRelayCost;
}

void ProgrammableCostFunction::print() {
	std::cout << "PCF Tokens:" << std::endl << "\t";
	//parser.printTokens(tokens);
	NOT_IMPLEMENTED;
}

void ProgrammableCostFunction::toJSONFile(const std::string& filename) {
	NOT_IMPLEMENTED;
}

