#include "whodun_suffix.h"

namespace whodun{

/**
 * Compare suffix array build entries (index, rank 0, rank 1) by rank.
 * @param useUni Ignored.
 * @param itemA The first entry.
 * @param itemB The second entry.
 * @return Whether itemA is less than itemB.
 */
template<typename Itp>
bool compareSuffixEntryRank(void* useUni, void* itemA, void* itemB){
	Itp* itemAP = (Itp*)itemA;
	Itp* itemBP = (Itp*)itemB;
	Itp curRankA = itemAP[1];
	Itp curRankB = itemBP[1];
	if(curRankA != curRankB){
		return curRankA < curRankB;
	}
	Itp nxtRankA = itemAP[2];
	Itp nxtRankB = itemBP[2];
	return nxtRankA < nxtRankB;
}

/**Fill rank/next rank.*/
template<typename Itp>
class SuffixArrayBuildInitRank : public ParallelForLoop{
public:
	/**
	 * Set rank/nextrank from the initial string.
	 * @param numThread The number of threads to use.
	 */
	SuffixArrayBuildInitRank(uintptr_t numThread) : ParallelForLoop(numThread){
		naturalStride = 1024;
	}
	/**Clean up.*/
	~SuffixArrayBuildInitRank(){}
	void doSingle(uintptr_t threadInd, uintptr_t ind){
		uintptr_t baseI = 3*ind;
		arrayTgt[baseI] = ind;
		arrayTgt[baseI+1] = (0x00FF & initStr.txt[ind]) + 1;
		if(ind == (initStr.len - 1)){
			arrayTgt[baseI+2] = 0;
		}
		else{
			arrayTgt[baseI+2] = (0x00FF & initStr.txt[ind+1]) + 1;
		}
	}
	/**The string to build for.*/
	SizePtrString initStr;
	/**The array to build.*/
	Itp* arrayTgt;
};

/**Figure out the next ranks.*/
template<typename Itp>
class SuffixArrayBuildReRankTask : public JoinableThreadTask{
public:
	/**Set up task.*/
	SuffixArrayBuildReRankTask(){}
	/**Clean up.*/
	~SuffixArrayBuildReRankTask(){}
	void doTask(){
		if(fromI == toI){ return; }
		if(phase == 1){
			Itp* curEntry = workArr + 3*fromI;
			Itp prevRank = curEntry[1];
			Itp prevComp = curEntry[2];
			Itp nextRank = 0;
			curEntry[1] = nextRank;
			for(uintptr_t i = fromI+1; i<toI; i++){
				curEntry += 3;
				Itp curRank = curEntry[1];
				Itp curComp = curEntry[2];
				if((curRank != prevRank) || (curComp != prevComp)){
					nextRank++;
					prevRank = curRank;
					prevComp = curComp;
				}
				curEntry[1] = nextRank;
			}
			maxRank = nextRank;
		}
		else{
			Itp* curEntry = workArr + 3*fromI;
			for(uintptr_t i = fromI; i<toI; i++){
				curEntry[1] += rankOffset;
				curEntry += 3;
			}
		}
	}
	
	/**The index this works from.*/
	uintptr_t fromI;
	/**The index this works to.*/
	uintptr_t toI;
	/**The array to work over.*/
	Itp* workArr;
	
	/**The phase this is working on.*/
	uintptr_t phase;
	
	/**The maximum (local) rank at the end.*/
	Itp maxRank;
	
	/**The value to offset ranks by.*/
	Itp rankOffset;
};

/**Build the inverse map.*/
template<typename Itp>
class SuffixArrayBuildInverseMap : public ParallelForLoop{
public:
	/**
	 * Get next rank from index sorted stuff.
	 * @param numThread The number of threads to use.
	 */
	SuffixArrayBuildInverseMap(uintptr_t numThread) : ParallelForLoop(numThread){
		naturalStride = 1024;
	}
	/**Clean up.*/
	~SuffixArrayBuildInverseMap(){}
	void doSingle(uintptr_t threadInd, uintptr_t ind){
		indexArr[entryArr[3*ind]] = ind;
	}
	/**The entries.*/
	Itp* entryArr;
	/**The place to put the indices.*/
	Itp* indexArr;
};

/**Figure out the next ranks.*/
template<typename Itp>
class SuffixArrayBuildNextRank : public ParallelForLoop{
public:
	/**
	 * Get next rank from index sorted stuff.
	 * @param numThread The number of threads to use.
	 */
	SuffixArrayBuildNextRank(uintptr_t numThread) : ParallelForLoop(numThread){
		naturalStride = 1024;
	}
	/**Clean up.*/
	~SuffixArrayBuildNextRank(){}
	void doSingle(uintptr_t threadInd, uintptr_t ind){
		Itp tgtI = ind + lookAhead;
		Itp winRank;
		if(tgtI >= origStr.len){
			winRank = 0;
		}
		else{
			uintptr_t realI = indexArr[tgtI];
			winRank = entryArr[3*realI + 1];
		}
		entryArr[3*ind+2] = winRank;
	}
	
	/**The string this is all for.*/
	SizePtrString origStr;
	/**The entries.*/
	Itp* entryArr;
	/**The place to put the indices.*/
	Itp* indexArr;
	/**How far ahead to go.*/
	Itp lookAhead;
};

/**Dump a build suffix array.*/
template<typename Itp>
class SuffixArrayBuildDumpIt : public ParallelForLoop{
public:
	/**
	 * Get next rank from index sorted stuff.
	 * @param numThread The number of threads to use.
	 */
	SuffixArrayBuildDumpIt(uintptr_t numThread) : ParallelForLoop(numThread){
		naturalStride = 1024;
	}
	/**Clean up.*/
	~SuffixArrayBuildDumpIt(){}
	void doSingle(uintptr_t threadInd, uintptr_t ind){
		endArr[ind] = entryArr[3*ind];
	}
	/**The entry array.*/
	Itp* entryArr;
	/**The place to write to.*/
	Itp* endArr;
};

/**Fill in a suffix array builder with the proper stuff.*/
template<typename Itp>
void suffixArrayBuilderSetup(SuffixArrayBuilder* toFill){
	uintptr_t numThread = toFill->numThread;
	ThreadPool* usePool = toFill->usePool;
	PODSortOptions rankOpts(3*sizeof(Itp), 0, compareSuffixEntryRank<Itp>);
	toFill->rankSort = usePool ? new PODInMemoryMergesort(&rankOpts, numThread, usePool) : new PODInMemoryMergesort(&rankOpts);
	toFill->initRank = new SuffixArrayBuildInitRank<Itp>(numThread);
	for(uintptr_t i = 0; i<numThread; i++){
		toFill->reRanks.push_back(new SuffixArrayBuildReRankTask<Itp>());
	}
	toFill->invMapB = new SuffixArrayBuildInverseMap<Itp>(numThread);
	toFill->nextRank = new SuffixArrayBuildNextRank<Itp>(numThread);
	toFill->dumpEnd = new SuffixArrayBuildDumpIt<Itp>(numThread);
}

/**
 * Actually build the array.
 * @param theMain The builder this is for.
 * @param buildFor The string to build for.
 * @param saveArray The place to put the final array.
 */
template<typename Itp>
void suffixArrayBuild(SuffixArrayBuilder* theMain, SizePtrString buildFor, void* saveArr){
	uintptr_t numThread = theMain->numThread;
	ThreadPool* usePool = theMain->usePool;
	//make some space
		Itp* saveArrT = (Itp*)saveArr;
		theMain->entryStore.clear();
		theMain->entryStore.resize(3*sizeof(Itp)*buildFor.len);
		Itp* entryArr = (Itp*)(theMain->entryStore[0]);
		theMain->invMapS.clear();
		theMain->invMapS.resize(sizeof(Itp)*buildFor.len);
		Itp* invmapArr = (Itp*)(theMain->invMapS[0]);
	//make the initial array of rank/next-rank and sort
		SuffixArrayBuildInitRank<Itp>* initRank = (SuffixArrayBuildInitRank<Itp>*)(theMain->initRank);
		initRank->initStr = buildFor;
		initRank->arrayTgt = entryArr;
		if(usePool){ initRank->doIt(usePool, 0, buildFor.len); }else{ initRank->doIt(0, buildFor.len); }
	//sort it
		theMain->rankSort->sort(buildFor.len, (char*)entryArr);
	//build for longer pieces
		uintptr_t numPT = buildFor.len / numThread;
		uintptr_t numET = buildFor.len % numThread;
		for(uintptr_t k = 4; k < 2*buildFor.len; k = k << 1){
			//rerank in chunks (note whether the chunk boundaries invoke a difference)
				uintptr_t curFI = 0;
				for(uintptr_t i = 0; i<numThread; i++){
					uintptr_t curNum = numPT + (i<numET);
					if(curNum == 0){ theMain->chunkInc[i] = 0; continue; }
					uintptr_t curBI = 3*curFI;
					if(curFI == 0){ theMain->chunkInc[i] = 1; }
					else{ theMain->chunkInc[i] = (entryArr[curBI+1] != entryArr[curBI-2]) || (entryArr[curBI+2] != entryArr[curBI-1]); }
					SuffixArrayBuildReRankTask<Itp>* curTask = (SuffixArrayBuildReRankTask<Itp>*)(theMain->reRanks[i]);
					curTask->fromI = curFI;
					curFI += curNum;
					curTask->toI = curFI;
					curTask->workArr = entryArr;
					curTask->phase = 1;
				}
				if(usePool){
					usePool->addTasks(numThread, (JoinableThreadTask**)&(theMain->reRanks[0]));
					joinTasks(numThread, &(theMain->reRanks[0]));
				}
				else{
					theMain->reRanks[0]->doTask();
				}
			//consolidate between chunks
				Itp curRank = 0;
				for(uintptr_t i = 0; i<numThread; i++){
					SuffixArrayBuildReRankTask<Itp>* curTask = (SuffixArrayBuildReRankTask<Itp>*)(theMain->reRanks[i]);
					curTask->phase = 2;
					if(theMain->chunkInc[i]){ curRank++; }
					curTask->rankOffset = curRank;
					curRank += curTask->maxRank;
				}
				if(usePool){
					usePool->addTasks(numThread, (JoinableThreadTask**)&(theMain->reRanks[0]));
					joinTasks(numThread, &(theMain->reRanks[0]));
				}
				else{
					theMain->reRanks[0]->doTask();
				}
			//inverse map
				SuffixArrayBuildInverseMap<Itp>* invMapB = (SuffixArrayBuildInverseMap<Itp>*)(theMain->invMapB);
				invMapB->entryArr = entryArr;
				invMapB->indexArr = invmapArr;
				if(usePool){ invMapB->doIt(usePool, 0, buildFor.len); }else{ invMapB->doIt(0, buildFor.len); }
			//next rank
				SuffixArrayBuildNextRank<Itp>* nextRank = (SuffixArrayBuildNextRank<Itp>*)(theMain->nextRank);
				nextRank->origStr = buildFor;
				nextRank->entryArr = entryArr;
				nextRank->indexArr = invmapArr;
				nextRank->lookAhead = k >> 1;
				if(usePool){ nextRank->doIt(usePool, 0, buildFor.len); }else{ nextRank->doIt(0, buildFor.len); }
			//sort
				theMain->rankSort->sort(buildFor.len, (char*)entryArr);
		}
	//spit it out
		SuffixArrayBuildDumpIt<Itp>* dumpDo = (SuffixArrayBuildDumpIt<Itp>*)(theMain->dumpEnd);
		dumpDo->entryArr = entryArr;
		dumpDo->endArr = saveArrT;
		if(usePool){ dumpDo->doIt(usePool, 0, buildFor.len); }else{ dumpDo->doIt(0, buildFor.len); }
}

/**
 * Limit a match.
 * @param searchS The search to update.
 * @param toStr The characters to match.
 */
template<typename Itp>
void suffixArrayLimitMatch(SuffixArraySearchState* searchS, int toChar){
	//get stuff
		Itp* suffArr = (Itp*)(searchS->inArray);
		uintptr_t charInd = searchS->charInd;
		SizePtrString origStr = searchS->forStr;
	//find the lower bound
		uintptr_t lowBndL = searchS->fromInd;
		uintptr_t lowBndH = searchS->toInd;
		while(lowBndH != lowBndL){
			uintptr_t count = lowBndH - lowBndL;
			uintptr_t step = count / 2;
			uintptr_t midBnd = lowBndL + step;
			Itp midCharI = suffArr[midBnd];
			uintptr_t availLen = origStr.len - (midCharI + charInd);
			int isLow;
			if(availLen){
				int origC = 0x00FF & origStr.txt[midCharI + charInd];
				isLow = origC < toChar;
			}
			else{
				isLow = 1;
			}
			if(isLow){ lowBndL = midBnd + 1; }
			else{ lowBndH = midBnd; }
		}
		searchS->fromInd = lowBndL;
	//find the upper bound
		lowBndH = searchS->toInd;
		while(lowBndH != lowBndL){
			uintmax_t count = lowBndH - lowBndL;
			uintmax_t step = count / 2;
			uintmax_t midBnd = lowBndL + step;
			Itp midCharI = suffArr[midBnd];
			uintptr_t availLen = origStr.len - (midCharI + charInd);
			int isLow;
			if(availLen){
				int origC = 0x00FF & origStr.txt[midCharI + charInd];
				isLow = origC <= toChar;
			}
			else{
				isLow = 1;
			}
			if(isLow){ lowBndL = midBnd + 1; }
			else{ lowBndH = midBnd; }
		}
		searchS->toInd = lowBndL;
	//and update character match count
		searchS->charInd++;
}

//TODO

};

using namespace whodun;

SuffixArrayBuilder::SuffixArrayBuilder(uintptr_t itemS){
	numThread = 1;
	usePool = 0;
	chunkInc.resize(1);
	itemSize = itemS;
	switch(itemSize){
		case WHODUN_SUFFIX_ARRAY_PTR: suffixArrayBuilderSetup<uintptr_t>(this); break;
		case WHODUN_SUFFIX_ARRAY_I32: suffixArrayBuilderSetup<uint32_t>(this); break;
		case WHODUN_SUFFIX_ARRAY_I64: suffixArrayBuilderSetup<uint64_t>(this); break;
		default: throw std::runtime_error("Invalid item size provided.");
	};
}
SuffixArrayBuilder::SuffixArrayBuilder(uintptr_t itemS, uintptr_t numThr, ThreadPool* mainPool){
	numThread = numThr;
	usePool = mainPool;
	itemSize = itemS;
	chunkInc.resize(numThr);
	switch(itemSize){
		case WHODUN_SUFFIX_ARRAY_PTR: suffixArrayBuilderSetup<uintptr_t>(this); break;
		case WHODUN_SUFFIX_ARRAY_I32: suffixArrayBuilderSetup<uint32_t>(this); break;
		case WHODUN_SUFFIX_ARRAY_I64: suffixArrayBuilderSetup<uint64_t>(this); break;
		default: throw std::runtime_error("Invalid item size provided.");
	};
}
SuffixArrayBuilder::~SuffixArrayBuilder(){
	delete(rankSort);
	delete(initRank);
	for(uintptr_t i = 0; i<reRanks.size(); i++){ delete(reRanks[i]); }
	delete(invMapB);
	delete(nextRank);
	delete(dumpEnd);
}
void SuffixArrayBuilder::build(SizePtrString buildOn, void* saveArr){
	switch(itemSize){
		case WHODUN_SUFFIX_ARRAY_PTR: suffixArrayBuild<uintptr_t>(this, buildOn, saveArr); break;
		case WHODUN_SUFFIX_ARRAY_I32: suffixArrayBuild<uint32_t>(this, buildOn, saveArr); break;
		case WHODUN_SUFFIX_ARRAY_I64: suffixArrayBuild<uint64_t>(this, buildOn, saveArr); break;
		default: throw std::runtime_error("Invalid item size provided.");
	};
}

#define SUFFIX_BATCH_SIZE 64

SuffixArraySearch::SuffixArraySearch(){}
SuffixArraySearch::~SuffixArraySearch(){}
void SuffixArraySearch::startSearch(SizePtrString origStr, uintptr_t arrSize, void* suffArr, SuffixArraySearchState* saveS){
	saveS->forStr = origStr;
	saveS->inArray = suffArr;
	saveS->arrSize = arrSize;
	saveS->charInd = 0;
	saveS->fromInd = 0;
	saveS->toInd = origStr.len;
}
void SuffixArraySearch::limitMatch(SuffixArraySearchState* searchS, char toChar){
	switch(searchS->arrSize){
		case WHODUN_SUFFIX_ARRAY_PTR: suffixArrayLimitMatch<uintptr_t>(searchS, 0x00FF & toChar); break;
		case WHODUN_SUFFIX_ARRAY_I32: suffixArrayLimitMatch<uint32_t>(searchS, 0x00FF & toChar); break;
		case WHODUN_SUFFIX_ARRAY_I64: suffixArrayLimitMatch<uint64_t>(searchS, 0x00FF & toChar); break;
		default: throw std::runtime_error("Invalid item size provided.");
	};
}


