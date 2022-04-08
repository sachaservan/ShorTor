#include <memory>
#include <string>
#include <chrono>

#include "mator.hpp"
#include "pcf.hpp"

using namespace std;
#define DATAPATH "../../test/Data/"



// ===== DEBUG =====
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define DEBUG_TIME(s) fprintf(stderr, ANSI_COLOR_YELLOW "%7.2fs: " ANSI_COLOR_RESET "%s\n", getDebugTimeMS()/1000.0, s); fflush(stderr);
#define DEBUG_TIME_F(s,  ...) fprintf(stderr, ANSI_COLOR_YELLOW "%7.2fs: " ANSI_COLOR_RESET s "\n", getDebugTimeMS()/1000.0, ##__VA_ARGS__); fflush(stderr);
#define DEBUG_TIME_RESET {debug_time_counter = std::chrono::high_resolution_clock::now();}
#define DEBUG_COLOR_F(color, s1, s2, ...) fprintf(stderr, ANSI_COLOR_##color s1 ANSI_COLOR_RESET s2 "\n", ##__VA_ARGS__); fflush(stderr);
#define DEBUG(s, ...) fprintf(stderr, s "\n", ##__VA_ARGS__); fflush(stderr);

chrono::steady_clock::time_point debug_time_counter = std::chrono::high_resolution_clock::now();
long long getDebugTimeMS() {
	return chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - debug_time_counter).count();
}
// ===== DEBUG_END =====





int main(int argc, char* argv[]) {
	// calculate some simple sender anonymity
	shared_ptr<SenderSpec> sender1 = make_shared<SenderSpec>(IP("134.2.3.4"));
	shared_ptr<SenderSpec> sender2 = make_shared<SenderSpec>(IP("8.8.8.8"));
	shared_ptr<RecipientSpec> recipient1 = make_shared<RecipientSpec>(IP("129.132.0.0"));
	shared_ptr<RecipientSpec> recipient2 = make_shared<RecipientSpec>(IP("129.132.0.0"));
	recipient1->ports.insert(443);
	recipient2->ports.insert(443);
	recipient2->ports.insert(1);
	shared_ptr<PathSelectionSpec> ps(new PSTorSpec());
	Config config;
	config.epsilon = 1;
	config.consensusFile = DATAPATH "2014-10-04-05-00-00-consensus-filtered-fast";
	config.databaseFile = ""; // no database given

	// initialize mator
	DEBUG_TIME("Creating MATor...");
	MATor mator(sender1, sender2, recipient1, recipient2, ps, ps, config);
	DEBUG_TIME("MATor created, commit specification...");
	mator.commitSpecification();
	DEBUG_TIME("Spec committed.");

	// 10-of-n
	mator.setAdversaryBudget(1000);  // 10-of-n => SA 0.321168
	mator.commitPCFs();

	double result;

	result = mator.getSenderAnonymity();
	DEBUG_TIME("SA Anonymity");
	cout << "Result: " << result << endl;

	cout << "Calculating greedy list:" << endl;
	std::vector<size_t> greedylist;
	mator.getGreedyListForSenderAnonymity(greedylist);
	if (greedylist.size() > 0)
	{
		cout << "[";
		for (int i = 0; i < greedylist.size() - 1; i++)
		{
			cout << greedylist[i] << ",";
		}
		
		cout << greedylist[greedylist.size() - 1] << "]" << endl;
	}

	mator.preparePreciseCalculation(greedylist);

	result = mator.getPreciseSenderAnonymity();
	DEBUG_TIME("Precise SA Anonymity");
	cout << "Result: " << result << endl;


	result = mator.getRecipientAnonymity();
	DEBUG_TIME("RA Anonymity");
	cout << "Result: " << result << endl;

	// Calculate REL
	result = mator.getRelationshipAnonymity();
	DEBUG_TIME("REL Anonymity");
	cout << "Result: " << result << endl;



	// 10-of-n non-const
	shared_ptr<ProgrammableCostFunction> pcf = make_shared<ProgrammableCostFunction>("ANY ? SET 2.5");
	mator.setAdversaryBudget(25);
	mator.addProgrammableCostFunction(pcf);
	mator.commitPCFs();

	// Calculate SA0
	result = mator.getSenderAnonymity();
	DEBUG_TIME("SA Anonymity");
	cout << "Result: " << result << endl;

	// Calculate REL
	result = mator.getRelationshipAnonymity();
	DEBUG_TIME("REL Anonymity");
	cout << "Result: " << result << endl;

	cin.get();
	return 0;
}
