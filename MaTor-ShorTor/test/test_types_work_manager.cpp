#define TEST_NAME Types

#include "stdafx.h"

#include <types/work_manager.hpp>

#define TASKS_NUM 15

struct WorkManagerFixture {
	WorkManager wm;

	WorkManagerFixture() : wm() {}
};

BOOST_FIXTURE_TEST_SUITE(WorkManagerSuite, WorkManagerFixture)

BOOST_AUTO_TEST_CASE(DoingItsJob)
{
	int testArray [TASKS_NUM] = { };

	for (int i = 0; i < TASKS_NUM; ++i)
	{
		wm.addTask([&testArray, i]() {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			testArray[i]++;
		});
	}
	
	wm.startAndJoinAll();

	for (int i = 0; i < TASKS_NUM; ++i)
	{
		BOOST_CHECK_EQUAL(testArray[i], 1);
	}
}

BOOST_AUTO_TEST_SUITE_END()
