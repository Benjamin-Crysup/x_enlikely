#include "whodun_thread.h"

#include <string.h>
#include <iostream>

namespace whodun {

/**Manages threading for a parallel for loop.*/
class ParallelForLoopTask : public JoinableThreadTask{
public:
	void doTask();
	/**The main loop.*/
	ParallelForLoop* mainLoop;
	/**The thread this is for.*/
	uintptr_t threadInd;
};

/**Manages threading for a parallel for loop.*/
class ParallelRaggedNestedForLoopTask : public JoinableThreadTask{
public:
	void doTask();
	/**The main loop.*/
	ParallelRaggedNestedForLoop* mainLoop;
	/**The thread this is for.*/
	uintptr_t threadInd;
};

/**Actually do memcpy.*/
class ThreadedMemCopyLoop : public ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ThreadedMemCopyLoop(uintptr_t numThread);
	/**Clean up.*/
	~ThreadedMemCopyLoop();
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	/**The place to copy to.*/
	char* copyTo;
	/**The place to copy from.*/
	const char* copyFrom;
};

/**Actually do memset.*/
class ThreadedMemSetLoop : public ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ThreadedMemSetLoop(uintptr_t numThread);
	/**Clean up.*/
	~ThreadedMemSetLoop();
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	/**The place to copy to.*/
	char* copyTo;
	/**The value to set to.*/
	int newValue;
};

/**Actually do memswap.*/
class ThreadedMemSwapLoop : public ParallelForLoop{
public:
	/**
	 * Set up.
	 * @param numThread The number of threads to use.
	 */
	ThreadedMemSwapLoop(uintptr_t numThread);
	/**Clean up.*/
	~ThreadedMemSwapLoop();
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	/**The first array.*/
	char* swapA;
	/**The second array.*/
	char* swapB;
};

};

using namespace whodun;

ReadWriteLock::ReadWriteLock() : waitW(&countLock), waitR(&countLock){
	numR = 0;
	numW = 0;
}
ReadWriteLock::~ReadWriteLock(){}
void ReadWriteLock::lockRead(){
	countLock.lock();
	while(numW){ waitR.wait(); }
	numR++;
	countLock.unlock();
}
void ReadWriteLock::unlockRead(){
	countLock.lock();
	numR--;
	if(numR == 0){ waitW.broadcast(); }
	countLock.unlock();
}
void ReadWriteLock::lockWrite(){
	countLock.lock();
	numW++;
	while(numR){ waitW.wait(); }
}
void ReadWriteLock::unlockWrite(){
	numW--;
	if(numW == 0){ waitR.broadcast(); }
	countLock.unlock();
}

JoinableThreadTask::JoinableThreadTask() : waitFinish(&finishLock){
	hasFinish = 0;
}
JoinableThreadTask::~JoinableThreadTask(){}
void JoinableThreadTask::doIt(){
	try{
		doTask();
	}
	catch(WhodunError& errW){
		wasErr = 1;
		errMess = errW.what();
		errContext = errW;
	}
	catch(std::exception& errE){
		wasErr = 1;
		errMess = errE.what();
		errContext = WhodunError(errE, __FILE__, __LINE__);
	}
	finishLock.lock();
	hasFinish = 1;
	waitFinish.broadcast();
	finishLock.unlock();
}
void JoinableThreadTask::join(){
	finishLock.lock();
	while(!hasFinish){
		waitFinish.wait();
	}
	finishLock.unlock();
	if(wasErr){
		throw errContext;
	}
}
void JoinableThreadTask::reset(){
	wasErr = 0;
	errMess.clear();
	hasFinish = 0;
}

void whodun::joinTasks(uintptr_t numWait, JoinableThreadTask** toDo){
	int wasError = 0;
	WhodunError toThrow;
	for(uintptr_t i = 0; i<numWait; i++){
		try{
			toDo[i]->join();
		}
		catch(WhodunError& errW){
			if(!wasError){
				toThrow = errW;
			}
			wasError = 1;
		}
	}
	if(wasError){
		throw toThrow;
	}
}

void ThreadPoolLoopTask::doIt(){
	std::string errMess;
	mainPool->taskMut.lock();
	while(mainPool->poolLive){
		if(mainPool->openTasks.size()){
			ThreadTask* nextRun = *(mainPool->openTasks.popFront(1));
			//run the task outside the lock
			mainPool->taskMut.unlock();
			try{
				nextRun->doIt();
			}catch(std::exception& err){
				nextRun->wasErr = 1;
				nextRun->errMess = err.what();
			}
			mainPool->taskMut.lock();
		}
		else{
			mainPool->drainCond.broadcast();
			mainPool->taskCond.wait();
		}
	}
	mainPool->taskMut.unlock();
}

ThreadPool::ThreadPool(int numThread) : taskCond(&taskMut), drainCond(&taskMut){
	poolLive = true;
	numThr = numThread;
	uniStore.resize(numThread);
	for(int i = 0; i<numThread; i++){
		uniStore[i].mainPool = this;
		liveThread.push_back(new OSThread(&(uniStore[i])));
	}
}
ThreadPool::~ThreadPool(){
	taskMut.lock();
		poolLive = false;
		taskCond.broadcast();
	taskMut.unlock();
	for(unsigned i = 0; i<liveThread.size(); i++){
		liveThread[i]->join();
		delete(liveThread[i]);
	}
	if(openTasks.size()){
		std::cerr << "Killing pool with stuff in the queue" << std::endl;
		std::terminate();
	}
}
void ThreadPool::addTask(ThreadTask* toDo){
	taskMut.lock();
		*(openTasks.pushBack(1)) = toDo;
		taskCond.signal();
	taskMut.unlock();
}
void ThreadPool::addTask(JoinableThreadTask* toDo){
	toDo->reset();
	addTask((ThreadTask*)toDo);
}
void ThreadPool::addTasks(uintptr_t numAdd, ThreadTask** toDo){
	taskMut.lock();
	
	uintptr_t numLeft = numAdd;
	ThreadTask** nextAdd = toDo;
	while(numLeft){
		uintptr_t numPush = std::max((uintptr_t)1, std::min(numLeft, openTasks.pushBackSpan()));
		ThreadTask** newT = openTasks.pushBack(numPush);
		memcpy(newT, nextAdd, numPush * sizeof(ThreadTask*));
		numLeft -= numPush;
		nextAdd += numPush;
	}
	taskCond.broadcast();
	
	taskMut.unlock();
}
void ThreadPool::addTasks(uintptr_t numAdd, JoinableThreadTask** toDo){
	for(uintptr_t i = 0; i<numAdd; i++){
		toDo[i]->reset();
	}
	addTasks(numAdd, (ThreadTask**)toDo);
}
void ThreadPool::drainIn(){
	taskMut.lock();
	while(openTasks.size()){
		drainCond.wait();
	}
	taskMut.unlock();
}

void ParallelForLoopTask::doTask(){
	mainLoop->taskMut.lock();
	
	doItAgain:
	if(mainLoop->anyErrors == 0){
		uintptr_t curStartI = mainLoop->nextIndex;
		if(curStartI < mainLoop->endIndex){
			uintptr_t curEndI = curStartI + mainLoop->naturalStride;
				curEndI = std::min(curEndI, mainLoop->endIndex);
			mainLoop->nextIndex = curEndI;
			mainLoop->taskMut.unlock();
			
			mainLoop->doRange(threadInd, curStartI, curEndI);
			
			mainLoop->taskMut.lock();
			goto doItAgain;
		}
	}
	
	mainLoop->taskMut.unlock();
}
ParallelForLoop::ParallelForLoop(uintptr_t numThread){
	allUni.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){
		ParallelForLoopTask* curT = new ParallelForLoopTask();
		curT->mainLoop = this;
		curT->threadInd = i;
		allUni[i] = curT;
	}
}
ParallelForLoop::~ParallelForLoop(){
	deleteAll(&allUni);
}
void ParallelForLoop::startIt(ThreadPool* inPool){
	anyErrors = 0;
	nextIndex = 0;
	for(uintptr_t i = 0; i<allUni.size(); i++){
		allUni[i]->reset();
	}
	inPool->addTasks(allUni.size(), (JoinableThreadTask**)&(allUni[0]));
}
void ParallelForLoop::startIt(ThreadPool* inPool, uintptr_t startI, uintptr_t endI){
	startIndex = startI;
	endIndex = endI;
	startIt(inPool);
}
void ParallelForLoop::joinIt(){
	joinTasks(allUni.size(), (JoinableThreadTask**)&(allUni[0]));
}
void ParallelForLoop::doIt(ThreadPool* inPool){
	startIt(inPool);
	joinIt();
}
void ParallelForLoop::doIt(ThreadPool* inPool, uintptr_t startI, uintptr_t endI){
	startIndex = startI;
	endIndex = endI;
	doIt(inPool);
}
void ParallelForLoop::doIt(){
	doRange(0, startIndex, endIndex);
}
void ParallelForLoop::doIt(uintptr_t startI, uintptr_t endI){
	startIndex = startI;
	endIndex = endI;
	doIt();
}
void ParallelForLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	try{
		for(uintptr_t i = fromI; i<toI; i++){
			doSingle(threadInd, i);
		}
	} catch(std::exception& err){
		taskMut.lock();
			anyErrors = 1;
		taskMut.unlock();
		doRangeError(threadInd, fromI, toI);
		throw;
	}
	doRangeEnd(threadInd, fromI, toI);
}
void ParallelForLoop::doRangeStart(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){}
void ParallelForLoop::doRangeEnd(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){}
void ParallelForLoop::doRangeError(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){}

void ParallelRaggedNestedForLoopTask::doTask(){
	mainLoop->taskMut.lock();
	
	doItAgain:
	if(mainLoop->anyErrors == 0){
		uintptr_t curStartI = mainLoop->nextI;
		if(curStartI < mainLoop->numOut){
			uintptr_t curStartJ = mainLoop->nextJ;
			uintptr_t curNumJ = mainLoop->numIn[curStartI];
			uintptr_t curEndJ = curStartJ + mainLoop->naturalStride;
			if(curEndJ >= curNumJ){
				curEndJ = curNumJ;
				mainLoop->nextI = curStartI + 1;
				mainLoop->nextJ = 0;
			}
			else{
				mainLoop->nextJ = curEndJ;
			}
			mainLoop->taskMut.unlock();
			
			mainLoop->doRange(threadInd, curStartI, curStartJ, curEndJ);
			
			mainLoop->taskMut.lock();
			goto doItAgain;
		}
	}
	
	mainLoop->taskMut.unlock();
}

ParallelRaggedNestedForLoop::ParallelRaggedNestedForLoop(uintptr_t numThread){
	allUni.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){
		ParallelRaggedNestedForLoopTask* curT = new ParallelRaggedNestedForLoopTask();
		curT->mainLoop = this;
		curT->threadInd = i;
		allUni[i] = curT;
	}
}
ParallelRaggedNestedForLoop::~ParallelRaggedNestedForLoop(){
	deleteAll(&allUni);
}
void ParallelRaggedNestedForLoop::startIt(ThreadPool* inPool, uintptr_t numOuter, uintptr_t* innerLens){
	numOut = numOuter;
	numIn = innerLens;
	anyErrors = 0;
	nextI = 0;
	nextJ = 0;
	for(uintptr_t i = 0; i<allUni.size(); i++){
		allUni[i]->reset();
	}
	inPool->addTasks(allUni.size(), (JoinableThreadTask**)&(allUni[0]));
}
void ParallelRaggedNestedForLoop::joinIt(){
	joinTasks(allUni.size(), (JoinableThreadTask**)&(allUni[0]));
}
void ParallelRaggedNestedForLoop::doIt(ThreadPool* inPool, uintptr_t numOuter, uintptr_t* innerLens){
	startIt(inPool, numOuter, innerLens);
	joinIt();
}
void ParallelRaggedNestedForLoop::doIt(uintptr_t numOuter, uintptr_t* innerLens){
	numOut = numOuter;
	numIn = innerLens;
	for(uintptr_t i = 0; i<numOut; i++){
		doRange(0, i, 0, numIn[i]);
	}
}
void ParallelRaggedNestedForLoop::doRange(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ){
	doRangeStart(threadInd, forI, fromJ, toJ);
	try{
		for(uintptr_t i = fromJ; i<toJ; i++){
			doSingle(threadInd, forI, i);
		}
	} catch(std::exception& err){
		taskMut.lock();
			anyErrors = 1;
		taskMut.unlock();
		doRangeError(threadInd, forI, fromJ, toJ);
		throw;
	}
	doRangeEnd(threadInd, forI, fromJ, toJ);
}
void ParallelRaggedNestedForLoop::doRangeStart(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ){}
void ParallelRaggedNestedForLoop::doRangeEnd(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ){}
void ParallelRaggedNestedForLoop::doRangeError(uintptr_t threadInd, uintptr_t forI, uintptr_t fromJ, uintptr_t toJ){}

ThreadedMemCopyLoop::ThreadedMemCopyLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	startIndex = 0;
	naturalStride = 4096;
}
ThreadedMemCopyLoop::~ThreadedMemCopyLoop(){}
void ThreadedMemCopyLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	memcpy(copyTo + fromI, copyFrom + fromI, toI - fromI);
	doRangeEnd(threadInd, fromI, toI);
}
void ThreadedMemCopyLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	copyTo[ind] = copyFrom[ind];
}

ThreadedMemSetLoop::ThreadedMemSetLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	startIndex = 0;
	naturalStride = 4096;
}
ThreadedMemSetLoop::~ThreadedMemSetLoop(){}
void ThreadedMemSetLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	memset(copyTo + fromI, newValue, toI - fromI);
	doRangeEnd(threadInd, fromI, toI);
}
void ThreadedMemSetLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	copyTo[ind] = newValue;
}

ThreadedMemSwapLoop::ThreadedMemSwapLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	startIndex = 0;
	naturalStride = 4096;
}
ThreadedMemSwapLoop::~ThreadedMemSwapLoop(){}
void ThreadedMemSwapLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	memswap(swapA + fromI, swapB + fromI, toI - fromI);
	doRangeEnd(threadInd, fromI, toI);
}
void ThreadedMemSwapLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	int tmpA = swapA[ind];
	int tmpB = swapB[ind];
	swapA[ind] = tmpB;
	swapB[ind] = tmpA;
}

ThreadedMemoryShuttler::ThreadedMemoryShuttler(uintptr_t numThr, ThreadPool* mainPool){
	realCopy = new ThreadedMemCopyLoop(numThr);
	realSet = new ThreadedMemSetLoop(numThr);
	realSwap = new ThreadedMemSwapLoop(numThr);
	usePool = mainPool;
}
ThreadedMemoryShuttler::~ThreadedMemoryShuttler(){
	delete((ParallelForLoop*)realCopy);
	delete((ParallelForLoop*)realSet);
	delete((ParallelForLoop*)realSwap);
}
void ThreadedMemoryShuttler::memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum){
	memcpyStart(cpyTo, cpyFrom, copyNum);
	memcpyJoin();
}
void ThreadedMemoryShuttler::memset(void* setP, int value, size_t numBts){
	memsetStart(setP, value, numBts);
	memsetJoin();
}
void ThreadedMemoryShuttler::memswap(char* arrA, char* arrB, size_t numBts){
	memswapStart(arrA, arrB, numBts);
	memswapJoin();
}
void ThreadedMemoryShuttler::memcpyStart(void* cpyTo, const void* cpyFrom, size_t copyNum){
	ThreadedMemCopyLoop* curDo = (ThreadedMemCopyLoop*)realCopy;
	curDo->endIndex = copyNum;
	curDo->copyTo = (char*)cpyTo;
	curDo->copyFrom = (const char*)cpyFrom;
	curDo->startIt(usePool);
}
void ThreadedMemoryShuttler::memcpyJoin(){
	ThreadedMemCopyLoop* curDo = (ThreadedMemCopyLoop*)realCopy;
	curDo->joinIt();
}
void ThreadedMemoryShuttler::memsetStart(void* setP, int value, size_t numBts){
	ThreadedMemSetLoop* curDo = (ThreadedMemSetLoop*)realSet;
	curDo->endIndex = numBts;
	curDo->copyTo = (char*)setP;
	curDo->newValue = value;
	curDo->startIt(usePool);
}
void ThreadedMemoryShuttler::memsetJoin(){
	ThreadedMemSwapLoop* curDo = (ThreadedMemSwapLoop*)realCopy;
	curDo->joinIt();
}
void ThreadedMemoryShuttler::memswapStart(char* arrA, char* arrB, size_t numBts){
	ThreadedMemSwapLoop* curDo = (ThreadedMemSwapLoop*)realSwap;
	curDo->endIndex = numBts;
	curDo->swapA = (char*)arrA;
	curDo->swapB = (char*)arrB;
	curDo->startIt(usePool);
}
void ThreadedMemoryShuttler::memswapJoin(){
	ThreadedMemSwapLoop* curDo = (ThreadedMemSwapLoop*)realCopy;
	curDo->joinIt();
}



