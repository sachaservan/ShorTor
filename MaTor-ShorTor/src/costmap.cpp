#include "costmap.hpp"
#include "utils.hpp"
#include "pcf.hpp"


double Costmap::operator[](size_t relayPos) {
	return costs[relayPos];/* something */;
}

void Costmap::addPCF(std::shared_ptr<ProgrammableCostFunction> PCF) {
	pcfs.push_back(PCF);
}

void Costmap::commit(const std::vector<Relay>& relays) {
	costs.resize(relays.size());
	int index = 0;
	for (auto r : relays) {
		double val = 1.0;
		for (auto pcf : pcfs) {
			val = pcf->apply(r, val);
		}
		costs[index++] = val;
	}
}

void Costmap::printPCFs() {
	for (auto pcf : pcfs) {
		pcf->print();
	}
}

void Costmap::reset() {
	pcfs.clear();
}

bool Costmap::isConstant() {
	return pcfs.empty();
}

bool Costmap::isInitialized(int numRelays) {
	return costs.size() >= numRelays;
}

