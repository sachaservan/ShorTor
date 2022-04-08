#ifndef WORK_MANAGER_HPP
#define WORK_MANAGER_HPP


#include <queue>
#include <mutex>
#include <thread>
#include <functional> 
#include "../utils.hpp"


/** @file */

/**
 * Work manager is an utility class for multithreaded computation parts.
 * It executes assigned functions, using available number of threads.
 * After feeding work manager with functions, startAndJoinAll function should be called. 
 */
class WorkManager
{
	public:
		/**
		 * Constructor initializes thread queue and obtains available hardware threads number.
		 */
		WorkManager(unsigned = std::thread::hardware_concurrency());
		
		/**
		 * Feeds work manager with functions to be executed.
		 * @param fun function to be executed, may be lambda function (syntax: addTask([&]() { context.function(args ...); }))
		 */
		void addTask(std::function<void()> fun);
		
		/**
		 * Starts threads executing queued functions and waits until the job is done.
		 */
		void startAndJoinAll();
		
		/**
		 * @return number of available hardware thread contexts.
		 */
		int getHardwareConcurrency() const { return hardwareConcurrency; }
	
	private:
	
		/**
		 * Thread-safe utility class for function calls queue.
		 * Used by Work Manager.
		 */
		template <typename T>
		class ThreadQueue
		{
			friend class WorkManager;
			protected:
				/**
				 * Creates empty queue.
				 */
				ThreadQueue() { }
				
				/**
				 * Pushes function to the queue.
				 * @param function function to be put in queue.
				 */
				void pushQ(T function)
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					queueT.push(function);
				}
				
				/**
				 * @return function from the queue or NULL, if queue is empty.
				 */
				T popQ()
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					if(queueT.empty())
						return nullptr;
					
					T el = queueT.front();
					queueT.pop();
					return el;
				}
			
			private:
				std::queue<T> queueT; /**< Function queue. */
				std::mutex queueMutex; /**< Mutex for locking access to the queue. */
		};
		
		/**
		 * Job run by single thread.
		 * Threads take functions from queue and execute them as long as any functions are available.
		 */
		void run();
		
		unsigned hardwareConcurrency; /**< Number of available hardware thread contexts. */
		ThreadQueue<std::function<void()>> tq; /**< Queue of tasks to be done. */
		std::vector<std::thread> jobs; /**< Threads spawned by manager. */

#ifdef MATOR_LOCK_PARALLEL_CODE
		static std::mutex wmLock;
#endif
};

#endif
