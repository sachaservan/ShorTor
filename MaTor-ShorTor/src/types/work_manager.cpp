#include "work_manager.hpp"

WorkManager::WorkManager(unsigned hardwareConcurrency)
{
	if(hardwareConcurrency < 1)
		this->hardwareConcurrency = 1;
	else
		this->hardwareConcurrency = hardwareConcurrency;
}

void WorkManager::addTask(std::function<void()> fun)
{
	tq.pushQ(fun);
}

void WorkManager::run()
{
	std::function<void()> fun;
	do
	{
		fun = tq.popQ();
		if(fun)
			fun();
	}
	while(fun);	
}

void WorkManager::startAndJoinAll()
{
#ifdef MATOR_LOCK_PARALLEL_CODE
	std::lock_guard<std::mutex> locker(wmLock);
#endif
	for(unsigned int i = 0; i < hardwareConcurrency; ++i)
		jobs.emplace_back([this]() { run(); });
	
	for(auto& j : jobs)
		j.join();
	jobs.clear();
}

#ifdef MATOR_LOCK_PARALLEL_CODE
std::mutex WorkManager::wmLock;
#endif

