#include "scenario.hpp"
#include "ps_tor.hpp"
#include "ps_distributor.hpp"
#include "PreciseAnonymity.hpp"
#include "adversary.hpp"

#include <iostream>
#include <string>
#include <chrono>

int main(int argc, char* argv[])
{
	clogsn("Main hello!");

	std::chrono::system_clock::time_point __start;
	std::chrono::system_clock::time_point __stop;
	if (argc<3)
	{
		std::cout << "plz enter consensus filename, db filename, via file name, and number of added nodes\n";
		return 0;
	}
	Consensus cons = Consensus(argv[1], argv[2], argv[3]);
	int addedSize= std::stoi(argv[4]);
	clogsn("addedSize in main "<< addedSize );

/*
	std::shared_ptr<SenderSpec> ssA = std::make_shared<SenderSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<SenderSpec> ssB = std::make_shared<SenderSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<RecipientSpec> rs1 = std::make_shared<RecipientSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<RecipientSpec> rs2 = std::make_shared<RecipientSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	rs1->ports.insert(443);
	rs2->ports.insert(443);*/
	std::shared_ptr<SenderSpec> ssA = std::make_shared<SenderSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<SenderSpec> ssB = std::make_shared<SenderSpec>(IP("129.132.0.0"), 9.25, 46.3434);
	std::shared_ptr<RecipientSpec> rs1 = std::make_shared<RecipientSpec>(IP("131.107.0.0"), 49.25, 6.3434);
	std::shared_ptr<RecipientSpec> rs2 = std::make_shared<RecipientSpec>(IP("134.96.225.86"), 49.25, 6.3434);
		rs1->ports.insert(443);
		rs2->ports.insert(25);
	std::shared_ptr<PSTorSpec> pstors = std::make_shared<PSTorSpec>();
	std::shared_ptr<PSLASTorSpec> pslt = std::make_shared<PSLASTorSpec>();
	pslt->cellSize = 10;
	std::shared_ptr<PathSelection> psA1 = Scenario::makePathSelection(pstors, ssA, rs1, cons);
	std::shared_ptr<PathSelection> psA2 = Scenario::makePathSelection(pstors, ssA, rs2, cons);
	std::shared_ptr<PathSelection> psB1 = Scenario::makePathSelection(pstors, ssB, rs1, cons);
	std::shared_ptr<PathSelection> psB2 = Scenario::makePathSelection(pstors, ssB, rs2, cons);

	__start = std::chrono::system_clock::now();
//	clogsn("addedSize in main "<< addedSize );

	PreciseAnonymity pa =PreciseAnonymity(cons, *psA1, *psA2, *psB1, *psB2, 1, addedSize);
	__stop = std::chrono::system_clock::now();
	clogsn("Time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(__stop - __start)).count());


}
