#ifndef WHODUN_THREAD_H
#define WHODUN_THREAD_H 1

/**
 * @file
 * @brief Higher order utilities for threads.
 */

#include <set>
#include <map>
#include <deque>
#include <stdint.h>

#include "whodun_ermac.h"
#include "whodun_oshook.h"
#include "whodun_container.h"

namespace whodun {

/**Allow many things to read, but only one to write.*/
class ReadWriteLock{
public:
	/**Setup*/
	ReadWriteLock();
	/**Teardown*/
	~ReadWriteLock();
	/**Lock for reading.*/
	void lockRead();
	/**Unlock for reading.*/
	void unlockRead();
	/**Lock for writing.*/
	void lockWrite();
	/**Unlock for writing.*/
	void unlockWrite();
	/**The number of things reading.*/
	uintptr_t numR;
	/**The number of things waiting to write.*/
	uintptr_t numW;
	/**For counts.*/
	OSMutex countLock;
	/**For writers.*/
	OSCondition waitW;
	/**For readers.*/
	OSCondition waitR;
};

/**A task to a thread.*/
class JoinableThreadTask : public ThreadTask{
public:
	/**Set up.*/
	JoinableThreadTask();
	/**Clean up.*/
	~JoinableThreadTask();
	/**Wrap the sub operations*/
	void doIt();
	/**Wait for the task to finish.*/
	void join();
	/**Reset the task for another use.*/
	void reset();
	/**Perform the task itself. */
	virtual void doTask() = 0;
	/**Whether the task had run.*/
	int hasFinish;
	/**A more thorough description of the error.*/
	WhodunError errContext;
	/**Lock the flag.*/
	OSMutex finishLock;
	/**Wait for it to finish.*/
	OSCondition waitFinish;
};

/**
 * Wait for multiple tasks: will wait for all of them before throwing the first error.
 * @param numWait The number of tasks to wait on.
 * @param toDo The tasks to wait on.
 */
void joinTasks(uintptr_t numWait, JoinableThreadTask** toDo);

class ThreadPool;

/**The actual task to run for thread pool.*/
class ThreadPoolLoopTask : public ThreadTask{
public:
	void doIt();
	/**Remember the actual thread pool.*/
	ThreadPool* mainPool;
};

/**A pool of reusable threads.*/
class ThreadPool{
public:
	/**
	 * Set up the threads.
	 * @param numThread The number of threads.
	 */
	ThreadPool(int numThread);
	/**Kill the threads.*/
	~ThreadPool();
	/**
	 * Add a task to run.
	 * @param toDo The thing to do.
	 */
	void addTask(ThreadTask* toDo);
	/**
	 * Add a task to run.
	 * @param toDo The thing to do.
	 */
	void addTask(JoinableThreadTask* toDo);
	/**
	 * Add multiple tasks in one go.
	 * @param numAdd The number of tasks to add.
	 * @param toDo The tasks to add.
	 */
	void addTasks(uintptr_t numAdd, ThreadTask** toDo);
	/**
	 * Add multiple tasks in one go.
	 * @param numAdd The number of tasks to add.
	 * @param toDo The tasks to add.
	 */
	void addTasks(uintptr_t numAdd, JoinableThreadTask** toDo);
	/**
	 * Wait for all tasks to start.
	 */
	void drainIn();
	
	/**Whether the pool is live.*/
	bool poolLive;
	/**The number of threads in this pool.*/
	int numThr;
	/**The task mutex.*/
	OSMutex taskMut;
	/**The task conditions.*/
	OSCondition taskCond;
	/**Wait for the queue to drain.*/
	OSCondition drainCond;
	/**All the tasks on the thing.*/
	StructDeque<ThreadTask*> openTasks;
	/**The real task uniform storage.*/
	std::vector<ThreadPoolLoopTask> uniStore;
	/**The live threads.*/
	std::vector<OSThread*> liveThread;
};

/**Run a for loop in parallel.*/
class ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ParallelForLoop(uintptr_t numThread);
	/**Allow subclasses to tear down.*/
	virtual ~ParallelForLoop();
	/**
	 * Start running the stupid thing.
	 * @param inPool The pool to run in.
	 */
	void startIt(ThreadPool* inPool);
	/**
	 * Start running the stupid thing.
	 * @param inPool The pool to run in.
	 * @param startI The index to start at.
	 * @param endI The index to end at.
	 */
	void startIt(ThreadPool* inPool, uintptr_t startI, uintptr_t endI);
	/**
	 * Wait for the stupid thing to finish.
	 */
	void joinIt();
	/**
	 * Actually run the stupid thing, and wait for it to finish.
	 * @param inPool The pool to run in.
	 * @param startI The index to start at.
	 * @param endI The index to end at.
	 */
	void doIt(ThreadPool* inPool, uintptr_t startI, uintptr_t endI);
	/**
	 * Actually run the stupid thing, and wait for it to finish.
	 * @param inPool The pool to run in.
	 */
	void doIt(ThreadPool* inPool);
	/**Run the stupid thing in one thread.*/
	void doIt();
	/**
	 * Run the stupid thing in one thread.
	 * @param startI The index to start at.
	 * @param endI The index to end at.
	 */
	void doIt(uintptr_t startI, uintptr_t endI);
	/**
	 * Run a range of indices.
	 * @param threadInd The thread this is for.
	 * @param fromI The index to start at.
	 * @param toI The index to run to.
	 */
	virtual void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	/**
	 * Run a single index.
	 * @param threadInd The thread this is for.
	 * @param ind The index to run.
	 */
	virtual void doSingle(uintptr_t threadInd, uintptr_t ind) = 0;
	/**
	 * Called at the start of doing a range: may be called multiple times a run.
	 * @param threadInd The thread this is for.
	 * @param fromI The index to start at.
	 * @param toI The index to run to.
	 */
	virtual void doRangeStart(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	/**
	 * Called at the end of doing a range (unless there was an exception): may be called multiple times a run.
	 * @param threadInd The thread this is for.
	 * @param fromI The index to start at.
	 * @param toI The index to run to.
	 */
	virtual void doRangeEnd(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	/**
	 * Called if doing a range led to an exception. Will be called at most once a run.
	 * @param threadInd The thread this is for.
	 * @param fromI The index to start at.
	 * @param toI The index to run to.
	 */
	virtual void doRangeError(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	/**The threads to run.*/
	std::vector<JoinableThreadTask*> allUni;
	/**The index to run from.*/
	uintptr_t startIndex;
	/**The index to run to*/
	uintptr_t endIndex;
	/**The natural group size.*/
	uintptr_t naturalStride;
	/**The task mutex.*/
	OSMutex taskMut;
	/**Whether any of the pieces hit an error: fail faster.*/
	int anyErrors;
	/**The next index waiting to go.*/
	uintptr_t nextIndex;
};

/**A nested for loop, where the inner loop iteration count changes.*/
class ParallelRaggedNestedForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ParallelRaggedNestedForLoop(uintptr_t numThread);
	/**Allow subclasses to tear down.*/
	virtual ~ParallelRaggedNestedForLoop();
	/**
	 * Start running the stupid thing.
	 * @param inPool The pool to run in.
	 * @param numOuter The number of outer loop iterations.
	 * @param innerLens The number of inner loop iterations for each outer loop.
	 */
	void startIt(ThreadPool* inPool, uintptr_t numOuter, uintptr_t* innerLens);
	/**
	 * Wait for the stupid thing to finish.
	 */
	void joinIt();
	/**
	 * Actually run the stupid thing, and wait for it to finish.
	 * @param inPool The pool to run in.
	 * @param numOuter The number of outer loop iterations.
	 * @param innerLens The number of inner loop iterations for each outer loop.
	 */
	void doIt(ThreadPool* inPool, uintptr_t numOuter, uintptr_t* innerLens);
	/**
	 * Run the stupid thing in one thread.
	 * @param numOuter The number of outer loop iterations.
	 * @param innerLens The number of inner loop iterations for each outer loop.
	 */
	void doIt(uintptr_t numOuter, uintptr_t* innerLens);
	/**
	 * Run a range of indices.
	 * @param threadInd The thread this is for.
	 * @param forI The outer loop index.
	 * @param fromJ The inner index to start at.
	 * @param toJ The inner index to run to.
	 */
	virtual void doRange(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ);
	/**
	 * Run a single index.
	 * @param threadInd The thread this is for.
	 * @param outI The outer index to run.
	 * @param inJ The inner index to run.
	 */
	virtual void doSingle(uintptr_t threadInd, uintptr_t outI, uintptr_t inJ) = 0;
	/**
	 * Called at the start of doing a range: may be called multiple times a run.
	 * @param threadInd The thread this is for.
	 * @param forI The outer loop index.
	 * @param fromJ The inner index to start at.
	 * @param toJ The inner index to run to.
	 */
	virtual void doRangeStart(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ);
	/**
	 * Called at the end of doing a range (unless there was an exception): may be called multiple times a run.
	 * @param threadInd The thread this is for.
	 * @param forI The outer loop index.
	 * @param fromJ The inner index to start at.
	 * @param toJ The inner index to run to.
	 */
	virtual void doRangeEnd(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ);
	/**
	 * Called if doing a range led to an exception. Will be called at most once a run.
	 * @param threadInd The thread this is for.
	 * @param forI The outer loop index.
	 * @param fromJ The inner index to start at.
	 * @param toJ The inner index to run to.
	 */
	virtual void doRangeError(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ);
	/**The threads to run.*/
	std::vector<JoinableThreadTask*> allUni;
	/**The natural group size.*/
	uintptr_t naturalStride;
	/**The task mutex.*/
	OSMutex taskMut;
	/**Whether any of the pieces hit an error: fail faster.*/
	int anyErrors;
	/**The next outer index waiting to go.*/
	uintptr_t nextI;
	/**The next inner index waiting to go.*/
	uintptr_t nextJ;
	/**The number of outer indices to go.*/
	uintptr_t numOut;
	/**The list of inner indices.*/
	uintptr_t* numIn;
};

/**Do memory opertions in threads.*/
class ThreadedMemoryShuttler : public MemoryShuttler{
public:
	/**
	 * Note the threads to use.
	 * @param numThr The number of threads to use.
	 * @param mainPool The thread pool to use.
	 */
	ThreadedMemoryShuttler(uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~ThreadedMemoryShuttler();
	void memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum);
	void memset(void* setP, int value, size_t numBts);
	void memswap(char* arrA, char* arrB, size_t numBts);
	
	/**
	 * Start a memcpy.
	 * @param cpyTo The place to copy to.
	 * @param cpyFrom The place to copy from.
	 * @param copyNum The number of bytes to copy.
	 */
	void memcpyStart(void* cpyTo, const void* cpyFrom, size_t copyNum);
	/**Wait for a memcpy to finish.*/
	void memcpyJoin();
	/**
	 * Start a memset.
	 * @param setP The place to set.
	 * @param value The byte to set to.
	 * @param numBts The number of bytes.
	 */
	void memsetStart(void* setP, int value, size_t numBts);
	/**Wait for a memset to finish.*/
	void memsetJoin();
	/**
	 * Start a memswap.
	 * @param arrA The first array.
	 * @param arrB The second array.
	 * @param numBts The number of bytes to swap.
	 */
	void memswapStart(char* arrA, char* arrB, size_t numBts);
	/**Wait for a memswap to finish.*/
	void memswapJoin();
	
	/**The task for copying.*/
	void* realCopy;
	/**The task for setting.*/
	void* realSet;
	/**The task for swapping.*/
	void* realSwap;
	/**The pool to use.*/
	ThreadPool* usePool;
};

};

#endif

