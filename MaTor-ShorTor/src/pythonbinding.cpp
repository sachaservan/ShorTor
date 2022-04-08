#ifdef USE_PYTHON

/*
FOR VS STUDIO:
- 2015 or higher required
- define environment variable "PYTHON_PATH" to point to "C:\...\python2"
FOR LINUX:
- enjoy cmake
*/

#include <memory>
#include <iostream>
#include <thread>
#include "pybind11/pybind11.h" 
#include "pybind11/cast.h" 
#include "pybind11/stl.h" 
#include "pybind11/functional.h"

namespace py = pybind11;
using namespace std;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
/*
namespace pybind11{ namespace detail{
template <class T> class type_caster<std::shared_ptr<T>> : public type_caster_holder<T, std::shared_ptr<T>> {
    typedef type_caster<T> parent;
public:
    bool load(PyObject *src, bool convert) {
        if (!parent::load(src, convert))
            return false;
        holder = ((T *) parent::value)->shared_from_this();
        return true;
    }
    explicit operator T*() { return this->value; }
    explicit operator T&() { return *(this->value); }
    explicit operator std::shared_ptr<T>&() { return holder; }
    explicit operator std::shared_ptr<T>*() { return &holder; }
    using type_caster<T>::cast;
    static PyObject *cast(const std::shared_ptr<T> &src, return_value_policy policy, PyObject *parent) {
        return type_caster<T>::cast(src.get(), policy, parent);
    }
protected:
    std::shared_ptr<T> holder;
};
}}
*/


#include "mator.hpp"
#include "pcf.hpp"
#include "relay.hpp"


class PythonBinder {
public:
	static void bindRelay(pybind11::module m);
};




PYBIND11_PLUGIN(pymator) {
	py::module m("pymator", "Python binding for libmator");

	// MATor interface
	py::class_<MATor, shared_ptr<MATor>>(m, "MATor")
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, Config>())
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, std::string&, std::string&, std::string&, bool>())
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<Consensus>, bool>())
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, std::string&, std::string&, std::string&>())
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, std::string&, std::string&>())
		.def(py::init<std::shared_ptr<SenderSpec>, std::shared_ptr<SenderSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<RecipientSpec>, std::shared_ptr<PathSelectionSpec>, std::shared_ptr<PathSelectionSpec>, std::string&>())
		.def(py::init<const std::string&>())
		.def("setSenderSpecA", &MATor::setSenderSpecA)
		.def("setSenderSpecB", &MATor::setSenderSpecB)
		.def("setPathSelectionSpec1", &MATor::setPathSelectionSpec1)
		.def("setPathSelectionSpec2", &MATor::setPathSelectionSpec2)
		.def("setRecipientSpec1", &MATor::setRecipientSpec1)
		.def("setRecipientSpec2", &MATor::setRecipientSpec2)
		.def("setConsensus", &MATor::setConsensus)
		.def("addMicrodesc", &MATor::addMicrodesc)
		.def("addMicrodescFile", &MATor::addMicrodescFile)
		.def("getSenderAnonymity", &MATor::getSenderAnonymity)
		.def("getRecipientAnonymity", &MATor::getRecipientAnonymity)
		.def("getRelationshipAnonymity", &MATor::getRelationshipAnonymity)
		.def("addProgrammableCostFunction", &MATor::addProgrammableCostFunction)
		.def("commitPCFs", &MATor::commitPCFs)
		.def("resetPCFs", &MATor::resetPCFs)
		.def("setPCF", static_cast<void(MATor::*)(string&)>(&MATor::setPCF))
		.def("setPCFCallback", &MATor::setPCFCallback, py::keep_alive<1, 2>())
		.def("setAdversaryBudget", &MATor::setAdversaryBudget)
		.def("commitSpecification", &MATor::commitSpecification)
		// GIL-aware functions
		.def("prepare", [](MATor& mator){
			py::gil_scoped_release release;
			mator.prepareCalculation();
		})
		.def("preparePreciseCalculation", [](MATor& mator, vector<size_t> nodes) {
			py::gil_scoped_release release;
			mator.preparePreciseCalculation(nodes);
		})
		.def("prepareNetworkCalculation", [](MATor& mator, vector<std::string> compromisedASes) {
			py::gil_scoped_release release;
			mator.prepareNetworkCalculation(compromisedASes);
		})
		.def("getSenderAnonymityGIL", [](MATor& mator) {
			py::gil_scoped_release release;
			return mator.getSenderAnonymity();
		})
		.def("getRecipientAnonymityGIL", [](MATor& mator) {
			py::gil_scoped_release release;
			return mator.getRecipientAnonymity();
		})
		.def("getRelationshipAnonymityGIL", [](MATor& mator) {
			py::gil_scoped_release release;
			return mator.getRelationshipAnonymity();
		})
		.def("setPCFGIL", [](MATor& mator, string& pcf) {
			py::gil_scoped_release release;
			return mator.setPCF(pcf);
		})
		.def("getGreedyListForSenderAnonymity", [](MATor& mator) {
			vector<size_t> tmp;
			mator.getGreedyListForSenderAnonymity(tmp);
			return tmp;
		})
		.def("getGreedyListForRecipientAnonymity", [](MATor& mator) {
			vector<size_t> tmp;
			mator.getGreedyListForRecipientAnonymity(tmp);
			return tmp;
		})
		.def("getGreedyListForRelationshipAnonymity", [](MATor& mator) {
			vector<size_t> tmp;
			mator.getGreedyListForRelationshipAnonymity(tmp);
			return tmp;
		})
		.def("getPreciseSenderAnonymity", &MATor::getPreciseSenderAnonymity)
		.def("getPreciseRecipientAnonymity", &MATor::getPreciseRecipientAnonymity)
		.def("getPreciseRelationshipAnonymity", &MATor::getPreciseRelationshipAnonymity)
		.def("getNetworkSenderAnonymity", &MATor::getPreciseSenderAnonymity)
		.def("getNetworkRecipientAnonymity", &MATor::getPreciseRecipientAnonymity)
		.def("getNetworkRelationshipAnonymity", &MATor::getPreciseRelationshipAnonymity)
		.def("getGreedyPreciseSenderAnonymity", [](MATor& mator) {
			vector<size_t> tmp; 
			mator.getGreedyListForSenderAnonymity(tmp);
			mator.preparePreciseCalculation(tmp);
			return mator.getPreciseSenderAnonymity();
		})
		.def("getGreedyPreciseRecipientAnonymity", [](MATor& mator) {
			vector<size_t> tmp;
			mator.getGreedyListForRecipientAnonymity(tmp);
			mator.preparePreciseCalculation(tmp);
			return mator.getPreciseRecipientAnonymity();
		})
		.def("getGreedyPreciseRelationshipAnonymity", [](MATor& mator) {
			vector<size_t> tmp;
			mator.getGreedyListForRelationshipAnonymity(tmp);
			mator.preparePreciseCalculation(tmp);
			return mator.getPreciseRelationshipAnonymity();
		})
		;

	py::class_<SenderSpec, shared_ptr<SenderSpec>>(m, "SenderSpec")
		.def(py::init<string&>())
		.def(py::init<string&, double, double>())
		.def(py::init<const IP&>())
		.def(py::init<const IP&, double, double>())
		.def_readwrite("address", &SenderSpec::address)
		.def_readwrite("latitude", &SenderSpec::latitude)
		.def_readwrite("longitude", &SenderSpec::longitude)
		;

	py::class_<RecipientSpec, shared_ptr<RecipientSpec>>(m, "RecipientSpec")
		.def(py::init<string&>())
		.def(py::init<string&, double, double>())
		.def(py::init<const IP&>())
		.def(py::init<const IP&, double, double>())
		.def_readwrite("address", &RecipientSpec::address)
		.def_readwrite("latitude", &RecipientSpec::latitude)
		.def_readwrite("longitude", &RecipientSpec::longitude)
		.def_readwrite("ports", &RecipientSpec::ports)
		;

	py::class_<PathSelectionSpec, shared_ptr<PathSelectionSpec>>(m, "PathSelectionSpec");
	py::class_<PSDistribuTorSpec, shared_ptr<PSDistribuTorSpec>>(m, "PSDistribuTorSpec", py::base<PathSelectionSpec>())
		.def(py::init())
		.def(py::init<double>())
		.def(py::init<double, bool>())
		;

	py::class_<PSTorSpec, shared_ptr<PSTorSpec>>(m, "PSTorSpec", py::base<PathSelectionSpec>())
		.def(py::init())
		.def(py::init<bool>())
		;
	py::class_<PSUniformSpec, shared_ptr<PSUniformSpec>>(m, "PSUniformSpec", py::base<PathSelectionSpec>())
		.def(py::init())
		.def(py::init<bool>())
		;
	py::class_<PSSelektorSpec, shared_ptr<PSSelektorSpec>>(m, "PSSelektorSpec", py::base<PathSelectionSpec>())
		.def(py::init())
		.def(py::init<string&>())
		.def(py::init<string&, bool>())
		;
	py::class_<PSLASTorSpec, shared_ptr<PSLASTorSpec>>(m, "PSLASTorSpec", py::base<PathSelectionSpec>())
		.def(py::init())
		.def(py::init<double, int>())
		.def(py::init<double, int, bool>())
		;

	py::class_<Consensus, shared_ptr<Consensus>>(m, "Consensus")
		.def("__init__", [](Consensus &instance, string& consensus, string& dbname, string& viaAllPairs, bool useVias) {
				py::gil_scoped_release release;
				new (&instance) Consensus(consensus, dbname, viaAllPairs, useVias);
		})
		.def("clone", [](Consensus& instance) {
				return make_shared<Consensus>(instance);
		})
		;

	PythonBinder::bindRelay(m);

	m.def("hardwareConcurrency", []() {
		return std::thread::hardware_concurrency();
	});

	return m.ptr();
}



void PythonBinder::bindRelay(pybind11::module m) {
	py::class_<Relay, shared_ptr<Relay>>(m, "Relay")
		.def_readonly("name", &Relay::name)
		.def_readonly("publishedDate", &Relay::publishedDate)
		.def_readonly("fingerprint", &Relay::fingerprint)
		.def_readonly("version", &Relay::version)
		.def_readonly("country", &Relay::country)
		.def_readonly("ASNumber", &Relay::ASNumber)
		.def_readonly("ASName", &Relay::ASName)
		.def_readonly("platform", &Relay::platform)
		.def_readonly("bandwidth", &Relay::bandwidth)
		.def_readonly("averagedBandwidth", &Relay::averagedBandwidth)
		.def_readonly("flags", &Relay::flags)
		.def_readonly("latitude", &Relay::latitude)
		.def_readonly("longitude", &Relay::longitude)
		.def_readonly("registeredAt", &Relay::registeredAt)
		.def_property_readonly("ip", [](Relay& r) { return (string)r.getAddress(); })

		.def("supportsConnection", [](Relay& relay, string ip, uint16_t port) {
			return relay.supportsConnection(IP(ip), port);
		})
		.def("getSubnet", &Relay::getSubnet)
		;
}

#endif
