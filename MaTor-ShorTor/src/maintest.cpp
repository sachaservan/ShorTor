#include "scenario.hpp"
#include "ps_tor.hpp"
#include "ps_distributor.hpp"
#include "generic_worst_case_anonymity.hpp"
#include "adversary.hpp"

#include <iostream>
#include <string>
#include <chrono>

int main()
{
	clogsn("Main hello!");
	std::string fakeStr;
	std::chrono::system_clock::time_point __start;
	std::chrono::system_clock::time_point __stop;
	
	Consensus cons = Consensus("con", "testdb", "viaAllPairsfile", false);
	std::shared_ptr<SenderSpec> ssA = std::make_shared<SenderSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<SenderSpec> ssB = std::make_shared<SenderSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<RecipientSpec> rs1 = std::make_shared<RecipientSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	std::shared_ptr<RecipientSpec> rs2 = std::make_shared<RecipientSpec>(IP("134.96.225.86"), 49.25, 6.3434);
	rs1->ports.insert(443);
	rs2->ports.insert(443);
	std::shared_ptr<PSTorSpec> pstors = std::make_shared<PSTorSpec>();
	std::shared_ptr<PSLASTorSpec> pslt = std::make_shared<PSLASTorSpec>();
	pslt->cellSize = 10;
	
	/*
	std::shared_ptr<PathSelection> psA1 = Scenario::makePathSelection(pstors, ssA, rs1, cons);
	std::shared_ptr<PathSelection> psA2 = Scenario::makePathSelection(pstors, ssA, rs2, cons);
	std::shared_ptr<PathSelection> psB1 = Scenario::makePathSelection(pslt, ssB, rs1, cons);
	std::shared_ptr<PathSelection> psB2 = Scenario::makePathSelection(pslt, ssB, rs2, cons);
	
	__start = std::chrono::system_clock::now();
	GenericWorstCaseAnonymity gwca = GenericWorstCaseAnonymity(cons, *psA1, *psA2, *psB1, *psB2);
	__stop = std::chrono::system_clock::now();
	clogsn("Time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(__stop - __start)).count());
	
	//*/
	
	//*
	std::shared_ptr<PathSelection> psA1 = Scenario::makePathSelection(pstors, ssA, rs1, cons);
	std::shared_ptr<PathSelection> psA2 = Scenario::makePathSelection(pstors, ssA, rs2, cons);
	std::shared_ptr<PathSelection> psB1 = Scenario::makePathSelection(pstors, ssB, rs1, cons);
	std::shared_ptr<PathSelection> psB2 = Scenario::makePathSelection(pstors, ssB, rs2, cons);
	
	__start = std::chrono::system_clock::now();
	GenericWorstCaseAnonymity gwca = GenericWorstCaseAnonymity(cons, *psA1, *psA2, *psB1, *psB2);
	__stop = std::chrono::system_clock::now();
	clogsn("Time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(__stop - __start)).count());
	
	KofNAdversary adv;
	//*
	adv.setBudget(1);
	clogsn(gwca.senderAnonymity(adv));
	adv.setBudget(10);
	clogsn(gwca.senderAnonymity(adv));
	adv.setBudget(100);
	clogsn(gwca.senderAnonymity(adv));
	adv.setBudget(1000);
	clogsn(gwca.senderAnonymity(adv));
	adv.setBudget(10000);
	clogsn(gwca.senderAnonymity(adv));
	//*/
	gwca.printBests(cons, 10);
	
	//test(cons, *psA1);
	
	//*/
	
	/*
	probability_t pbb = 0;
	probability_t maxpbb = 0;
	int cnt = 0;
	for(size_t i = 0; i < cons.getSize(); ++i)
	{
		probability_t ep = ps1->exitProb(i);
		if(ep)
			++cnt;
		if(ep > maxpbb)
			maxpbb = ep;
		pbb += ep;
	}
	clogsn("There are " << cnt << " exits out of " << cons.getSize());
	clogsn("Total pbb: " << pbb);
	clogsn("Max pbb: " << maxpbb);
	//*/
	
	/*
	for(size_t some_exit = 0; some_exit < cons.getSize(); ++some_exit)
	{
		probability_t pbb2 = 0;
		for(size_t i = 0; i < cons.getSize(); ++i)
			pbb2 += ps1->entryProb(i, some_exit);
		clogsn("En pbb: " << (pbb2));
	}
	//*/
	
	/*
	for(size_t some_exit = 0; some_exit < cons.getSize(); ++some_exit)
	{
		if(ps1->exitProb(some_exit) > 0)
			for(size_t some_entry = 0; some_entry < cons.getSize(); ++some_entry)
				if(ps1->entryProb(some_entry, some_exit) > 0)
				{
					probability_t pbb3 = 0;
					for(size_t i = 0; i < cons.getSize(); ++i)
						pbb3 += ps1->middleProb(i, some_entry, some_exit);
					clogsn("Mi pbb: " << (pbb3));
				}
	}
	//*/
	
	/*
	std::shared_ptr<PSDistribuTorSpec> psdi = std::make_shared<PSDistribuTorSpec>();
	psdi->bandwidthPerc = 0.4;
	std::shared_ptr<PathSelection> ps2 = Scenario::makePathSelection(psdi, ss, rs, cons);
	
	std::getline(std::cin, fakeStr);
	*/
	
	/*
	
	probability_t d_maxpbb = 0;
	probability_t d_pbb = 0;
	int d_cnt = 0;
	for(size_t i = 0; i < cons.getSize(); ++i)
	{
		probability_t ep = ps2->exitProb(cons.getRelay(i));
		if(ep)
		{
			++d_cnt;
		}
		if(ep > d_maxpbb)
			d_maxpbb = ep;
		d_pbb += ep;
		//clogsn("i: " << i << " e: " << ps1->exitProb(cons.getRelay(i)));
	}
	clogsn("There are " << d_cnt << " exits out of " << cons.getSize());
	clogsn("Total pbb: " << d_pbb);
	clogsn("Max pbb: " << d_maxpbb);*/
	
	/*

	//std::shared_ptr<PathSelection> ps3 = Scenario::makePathSelection(pslt, ss2, rs, cons);
	std::shared_ptr<PathSelection> ps4 = Scenario::makePathSelection(pslt, ss2, rs2, cons);
	
	//std::getline(std::cin, fakeStr);
	//*/
	
	/*
	probability_t pbb = 0;
	probability_t maxpbb = 0;
	int cnt = 0;
	for(size_t i = 0; i < cons.getSize(); ++i)
	{
		probability_t ep = ps4->exitProb(i);
		if(ep)
			++cnt;
		if(ep > maxpbb)
			maxpbb = ep;
		pbb += ep;
	}
	clogsn("There are " << cnt << " exits out of " << cons.getSize());
	clogsn("Total pbb: " << pbb);
	clogsn("Max pbb: " << maxpbb);
	//*/
	
	/*
	for(size_t some_exit = 0; some_exit < cons.getSize(); ++some_exit)
	{
		probability_t pbb2 = 0;
		for(size_t i = 0; i < cons.getSize(); ++i)
			pbb2 += ps4->entryProb(i, some_exit);
		clogsn("En pbb: " << (pbb2));
	}
	//*/
	
	/*
	for(size_t some_exit = 0; some_exit < cons.getSize(); ++some_exit)
	{
		if(ps4->exitProb(some_exit) > 0)
			for(size_t some_entry = 0; some_entry < cons.getSize(); ++some_entry)
				if(ps4->entryProb(some_entry, some_exit) > 0)
				{
					probability_t pbb3 = 0;
					for(size_t i = 0; i < cons.getSize(); ++i)
						if(ps4->middleEntryExitAllowed(i, some_entry, some_exit))
							pbb3 += ps4->middleProb(i, some_entry, some_exit);
					clogsn("Mi pbb: " << (pbb3));
				}
	}
	//*/

}
