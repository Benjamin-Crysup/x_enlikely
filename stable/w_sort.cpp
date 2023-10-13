#include "whodun_sort.h"

namespace whodun{
	
/**Go between between function pointer based and classes.*/
bool whodun_comparatorSortFunc(void* useUni, void* itemA, void* itemB);

/**Merge.*/
class PODMergeQuantTask : public JoinableThreadTask{
public:
	/**
	 * Basic setup
	 * @param theOpts Sorting options.
	 */
	PODMergeQuantTask(PODSortOptions* theOpts);
	/**Tear down*/
	~PODMergeQuantTask();
	void doTask();
	
	/**The number of things in the full first list.*/
	uintptr_t numEA;
	/**The full first list.*/
	char* dataA;
	/**The number of things in the full second list.*/
	uintptr_t numEB;
	/**The full second list.*/
	char* dataB;
	/**The full target list.*/
	char* dataE;
	/**The start quantile.*/
	uintptr_t quantileStart;
	/**The end quantile.*/
	uintptr_t quantileEnd;
	/**Sorting options.*/
	PODSortOptions opts;
	
	/**
	 * Figure out how many things to get from list A to hit the requested quantile.
	 * @param forQuant The desired quantile.
	 * @return The number of things to get from list A.
	 */
	uintptr_t chunkyPairedQuantile(uintptr_t forQuant);
};

/**
 * Performs a small mergesort in memory in a single thread..
 * @param numEnts The entries to merge.
 * @param inMem The data to sort.
 * @param opts Sort options.
 * @param tmpStore Temporary storage (same size as inMem).
 * @return Whether the end result is in tmpStore.
 */
int whodun_inMemoryMergesortSmall(uintptr_t numEnts, char* inMem, PODSortOptions* opts, char* tmpStore);

/**Sort a chunk.*/
class PODSortChunkTask : public JoinableThreadTask{
public:
	/**
	 * Basic setup
	 * @param theOpts Sorting options.
	 */
	PODSortChunkTask(PODSortOptions* theOpts);
	/**Tear down*/
	~PODSortChunkTask();
	void doTask();
	
	/**The number of things in the list.*/
	uintptr_t numElem;
	/**The array the data start in.*/
	char* dataStart;
	/**A temporary array.*/
	char* dataTemp;
	/**Whether the final thing should wind up in dataTemp.*/
	int wantInTemp;
	
	/**Sorting options.*/
	PODSortOptions opts;
};

/**
 * Create a new temporary name.
 * @param inFolder The folder it's in.
 * @param toFill The place to put the name.
 * @param numTemps The number of this temporary.
 */
void whodun_externalSortNewTempName(const char* inFolder, std::string* toFill, uintptr_t numTemps);

/**Manage an external file.*/
class PODExternalFileSource{
public:
	/**
	 * Set up a manager without threads.
	 * @param maxLoad The maximum number of entities to load in one go.
	 * @param theOpts Sorting options.
	 * @param fromFile The file to load from.
	 */
	PODExternalFileSource(uintptr_t maxLoad, PODSortOptions* theOpts, InStream* fromFile);
	/**
	 * Set up a manager with threads.
	 * @param maxLoad The maximum number of entities to load in one go.
	 * @param theOpts Sorting options.
	 * @param fromFile The file to load from.
	 * @param numThr The number of threads to use.
	 * @param mainPool The thread pool to use.
	 */
	PODExternalFileSource(uintptr_t maxLoad, PODSortOptions* theOpts, InStream* fromFile, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up*/
	~PODExternalFileSource();
	
	/**Load in some more data.*/
	void load();
	
	/**Do memory operations*/
	MemoryShuttler* doMemOps;
	/**The number of entities to maintain.*/
	uintptr_t arenaSize;
	/**Options for sorting.*/
	PODSortOptions opts;
	/**The place to load from the file.*/
	char* loadArena;
	/**The offset to the first loaded thing.*/
	uintptr_t loadOffset;
	/**The number of things loaded.*/
	uintptr_t loadSize;
	/**The actual source.*/
	InStream* baseFile;
	
	/**Whether the end of the file has been hit.*/
	int haveHitEnd;
};

};

using namespace whodun;

PODComparator::~PODComparator(){}

bool whodun::whodun_comparatorSortFunc(void* useUni, void* itemA, void* itemB){
	return ((PODComparator*)(useUni))->compare(itemA, itemB);
}
PODSortOptions::PODSortOptions(){
	compMeth = 0;
	useUni = 0;
	itemSize = 0;
}
PODSortOptions::PODSortOptions(uintptr_t itemS, void* useU, bool(*compM)(void*,void*,void*)){
	compMeth = compM;
	useUni = useU;
	itemSize = itemS;
}
PODSortOptions::PODSortOptions(PODComparator* useComp){
	compMeth = whodun_comparatorSortFunc;
	useUni = useComp;
	itemSize = useComp->itemSize();
}

PODSortedDataChunk::PODSortedDataChunk(PODSortOptions* theOpts, char* dataStart, uintptr_t numData){
	opts = *theOpts;
	data = dataStart;
	size = numData;
}
PODSortedDataChunk::~PODSortedDataChunk(){}
uintptr_t PODSortedDataChunk::lowerBound(char* lookFor){
	uintptr_t itemSize = opts.itemSize;
	char* first = data;
	uintptr_t count = size;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(opts.compMeth(opts.useUni, it, lookFor)){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return (first - data) / itemSize;
}
uintptr_t PODSortedDataChunk::upperBound(char* lookFor){
	uintptr_t itemSize = opts.itemSize;
	char* first = data;
	uintptr_t count = size;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(!(opts.compMeth(opts.useUni, lookFor, it))){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return (first - data) / itemSize;
}

PODSortedDataMerger::PODSortedDataMerger(PODSortOptions* theOpts){
	opts = *theOpts;
	usePool = 0;
	{
		PODMergeQuantTask* curTask = new PODMergeQuantTask(theOpts);
		passUnis.push_back(curTask);
	}
}
PODSortedDataMerger::PODSortedDataMerger(PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool){
	opts = *theOpts;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThr; i++){
		PODMergeQuantTask* curTask = new PODMergeQuantTask(theOpts);
		passUnis.push_back(curTask);
	}
}
PODSortedDataMerger::~PODSortedDataMerger(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void PODSortedDataMerger::merge(uintptr_t numEA, char* dataA, uintptr_t numEB, char* dataB, char* dataE){
	startMerge(numEA, dataA, numEB, dataB, dataE);
	joinMerge();
}
void PODSortedDataMerger::startMerge(uintptr_t numEA, char* dataA, uintptr_t numEB, char* dataB, char* dataE){
	uintptr_t numThread = passUnis.size();
	uintptr_t totalNumE = numEA + numEB;
	uintptr_t numPT = totalNumE / numThread;
	uintptr_t numET = totalNumE % numThread;
	uintptr_t curQuant = 0;
	for(uintptr_t i = 0; i<numThread; i++){
		PODMergeQuantTask* curTask = (PODMergeQuantTask*)(passUnis[i]);
		curTask->numEA = numEA;
		curTask->dataA = dataA;
		curTask->numEB = numEB;
		curTask->dataB = dataB;
		curTask->dataE = dataE;
		curTask->quantileStart = curQuant;
		curQuant += (numPT + (i < numET));
		curTask->quantileEnd = curQuant;
	}
	if(usePool){
		usePool->addTasks(passUnis.size(), (JoinableThreadTask**)(&(passUnis[0])));
	}
}
void PODSortedDataMerger::joinMerge(){
	if(usePool){
		joinTasks(passUnis.size(), (JoinableThreadTask**)(&(passUnis[0])));
	}
	else{
		passUnis[0]->doTask();
	}
}

PODMergeQuantTask::PODMergeQuantTask(PODSortOptions* theOpts){
	opts = *theOpts;
}
PODMergeQuantTask::~PODMergeQuantTask(){}
void PODMergeQuantTask::doTask(){
	uintptr_t itemSize = opts.itemSize;
	//figure the quantiles
		uintptr_t quantALow = chunkyPairedQuantile(quantileStart);
		uintptr_t quantAHigh = chunkyPairedQuantile(quantileEnd);
	//turn into arrays
		char* curTgt = dataE + quantileStart;
		char* curElemA = dataA + quantALow;
		uintptr_t numElemA = quantAHigh - quantALow;
		char* curElemB = dataB + (quantileStart - quantALow);
		uintptr_t numElemB = (quantileEnd - quantileStart) - numElemA;
	//merge
		while(numElemA && numElemB){
			//in case of tie, A should go first (to keep it stable)
			if(!(opts.compMeth(opts.useUni, curElemB, curElemA))){
				memcpy(curTgt, curElemA, itemSize);
				curTgt += itemSize;
				curElemA += itemSize;
				numElemA--;
			}
			else{
				memcpy(curTgt, curElemB, itemSize);
				curTgt += itemSize;
				curElemB += itemSize;
				numElemB--;
			}
		}
		if(numElemA){ memcpy(curTgt, curElemA, numElemA*itemSize); }
		if(numElemB){ memcpy(curTgt, curElemB, numElemB*itemSize); }
}
uintptr_t PODMergeQuantTask::chunkyPairedQuantile(uintptr_t forQuant){
	uintptr_t itemSize = opts.itemSize;
	//upper bound in A
	PODSortedDataChunk chunkB(&opts, dataB, numEB);
	uintptr_t fromALow = 0;
	uintptr_t fromAHig = numEA;
	while(fromAHig - fromALow){
		uintptr_t fromAMid = fromALow + ((fromAHig - fromALow) >> 1);
		char* midVal = dataA + itemSize * fromAMid;
		uintptr_t fromBMid = chunkB.lowerBound(midVal);
		if((fromAMid + fromBMid) < forQuant){
			fromALow = fromAMid + 1;
		}
		else{
			fromAHig = fromAMid;
		}
	}
	return fromALow;
}

int whodun::whodun_inMemoryMergesortSmall(uintptr_t numEnts, char* inMem, PODSortOptions* opts, char* tmpStore){
	uintptr_t itemSize = opts->itemSize;
	char* curStore = inMem;
	char* nxtStore = tmpStore;
	uintptr_t size = 1;
	while(size < numEnts){
		uintptr_t curBase = 0;
		while(curBase < numEnts){
			char* curFromA = curStore + (curBase*itemSize);
			char* curTo = nxtStore + (curBase*itemSize);
			uintptr_t nxtBase = curBase + size;
			if(nxtBase > numEnts){
				memcpy(curTo, curFromA, (numEnts - curBase)*itemSize);
				break;
			}
			char* curFromB = curStore + (nxtBase*itemSize);
			uintptr_t finBase = std::min(nxtBase + size, numEnts);
			uintptr_t numLeftA = nxtBase - curBase;
			uintptr_t numLeftB = finBase - nxtBase;
			while(numLeftA && numLeftB){
				if(opts->compMeth(opts->useUni, curFromA, curFromB)){
					memcpy(curTo, curFromA, itemSize);
					curTo += itemSize;
					curFromA += itemSize;
					numLeftA--;
				}
				else{
					memcpy(curTo, curFromB, itemSize);
					curTo += itemSize;
					curFromB += itemSize;
					numLeftB--;
				}
			}
			if(numLeftA){ memcpy(curTo, curFromA, numLeftA*itemSize); }
			if(numLeftB){ memcpy(curTo, curFromB, numLeftB*itemSize); }
			curBase = finBase;
		}
		size = (size << 1);
		char* swapSave = curStore;
		curStore = nxtStore;
		nxtStore = swapSave;
	}
	return curStore != inMem;
}

PODSortChunkTask::PODSortChunkTask(PODSortOptions* theOpts){
	opts = *theOpts;
}
PODSortChunkTask::~PODSortChunkTask(){}
void PODSortChunkTask::doTask(){
	int isInTemp = whodun_inMemoryMergesortSmall(numElem, dataStart, &opts, dataTemp);
	if(wantInTemp){
		if(!isInTemp){
			memcpy(dataTemp, dataStart, numElem * opts.itemSize);
		}
	}
	else{
		if(isInTemp){
			memcpy(dataStart, dataTemp, numElem * opts.itemSize);
		}
	}
}

PODInMemoryMergesort::PODInMemoryMergesort(PODSortOptions* theOpts){
	opts = *theOpts;
	allocSize = 1024;
	allocTmp = (char*)malloc(allocSize * opts.itemSize);
	usePool = 0;
	//figure out whether the initial sorts should wind up in start or finish
		wantInTemp = 0;
	//make the thread stuff
	{
		PODSortChunkTask* curSTask = new PODSortChunkTask(theOpts);
			curSTask->wantInTemp = wantInTemp;
			passUnis.push_back(curSTask);
		PODSortedDataMerger* curMerge = new PODSortedDataMerger(theOpts);
			mergeMeths.push_back(curMerge);
	}
}
PODInMemoryMergesort::PODInMemoryMergesort(PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool){
	opts = *theOpts;
	allocSize = 1024*numThr;
	allocTmp = (char*)malloc(allocSize * opts.itemSize);
	usePool = mainPool;
	//figure out whether the initial sorts should wind up in start or finish
		wantInTemp = 0;
		uintptr_t tmpNumThr = numThr;
		while(tmpNumThr > 1){
			wantInTemp = !wantInTemp;
			tmpNumThr = (tmpNumThr >> 1) + (tmpNumThr & 0x01);
		}
	//make the thread stuff
	for(uintptr_t i = 0; i<numThr; i++){
		PODSortChunkTask* curSTask = new PODSortChunkTask(theOpts);
			curSTask->wantInTemp = wantInTemp;
			passUnis.push_back(curSTask);
		PODSortedDataMerger* curMerge = new PODSortedDataMerger(theOpts, numThr, mainPool);
			mergeMeths.push_back(curMerge);
	}
}
PODInMemoryMergesort::~PODInMemoryMergesort(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
		delete(mergeMeths[i]);
	}
	free(allocTmp);
}
void PODInMemoryMergesort::sort(uintptr_t numEntries, char* entryStore){
	uintptr_t itemSize = opts.itemSize;
	//test storage allocation
		if(numEntries > allocSize){
			if(allocTmp){ free(allocTmp); }
			allocSize = numEntries;
			allocTmp = (char*)malloc(allocSize * itemSize);
		}
	//do the initial sort
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numEntries / numThread;
		uintptr_t numET = numEntries % numThread;
		uintptr_t curI0 = 0;
		threadPieceSizes.clear();
		for(uintptr_t i = 0; i<numThread; i++){
			PODSortChunkTask* curSortT = (PODSortChunkTask*)(passUnis[i]);
			uintptr_t curNum = numPT + (i < numET);
			curSortT->numElem = curNum;
			curSortT->dataStart = entryStore + curI0*itemSize;
			curSortT->dataTemp = allocTmp + curI0*itemSize;
			threadPieceSizes.push_back(curNum);
			curI0 += curNum;
		}
		if(usePool){
			usePool->addTasks(passUnis.size(), (JoinableThreadTask**)(&(passUnis[0])));
			joinTasks(passUnis.size(), (JoinableThreadTask**)(&(passUnis[0])));
		}
		else{
			passUnis[0]->doTask();
		}
	//do the merges
		char* curMergeFrom = wantInTemp ? allocTmp : entryStore;
		char* curMergeTo = wantInTemp ? entryStore : allocTmp;
		while(threadPieceSizes.size() > 1){
			//start the merges
				uintptr_t curI0 = 0;
				uintptr_t startNumP = threadPieceSizes.size();
				uintptr_t curPI = 0;
				uintptr_t curMI = 0;
				while(curPI < startNumP){
					char* curElemE = curMergeTo + curI0*itemSize;
					uintptr_t curNumA = threadPieceSizes[curPI];
					char* curElemA = curMergeFrom + curI0*itemSize;
					curI0 += curNumA;
					curPI++;
					char* curElemB = curMergeFrom + curI0*itemSize;
					uintptr_t curNumB = (curPI < startNumP) ? threadPieceSizes[curPI] : 0;
					curI0 += curNumB;
					curPI++;
					PODSortedDataMerger* curMergeT = mergeMeths[curMI];
					curMergeT->startMerge(curNumA, curElemA, curNumB, curElemB, curElemE);
					threadPieceSizes.push_back(curNumA + curNumB);
					curMI++;
				}
			//wait for them to finish
				for(uintptr_t i = 0; i<curMI; i++){
					mergeMeths[i]->joinMerge();
				}
			//prepare for the next round
				char* tmpMerge = curMergeFrom;
				curMergeFrom = curMergeTo;
				curMergeTo = tmpMerge;
				threadPieceSizes.erase(threadPieceSizes.begin(), threadPieceSizes.begin() + startNumP);
		}
}

void whodun::whodun_externalSortNewTempName(const char* inFolder, std::string* toFill, uintptr_t numTemps){
	char asciiBuff[8*sizeof(uintmax_t)+8];
	toFill->append(inFolder);
	StandardMemorySearcher strLook;
	if(!strLook.memendswith(toSizePtr(toFill->c_str()), toSizePtr(filePathSeparator))){
		toFill->append(filePathSeparator);
	}
	toFill->append("sort_temp");
	sprintf(asciiBuff, "%ju", (uintmax_t)numTemps);
	toFill->append(asciiBuff);
}

#define TEMPORARY_BLOCK_SIZE 0x0100000

PODExternalMergeSort::PODExternalMergeSort(const char* workDirName, PODSortOptions* theOpts){
	statusDump = 0;
	maxLoad = 0x040000;
	maxMergeFiles = 256;
	minMergeEnts = 1024;
	numTemps = 0;
	tempName = workDirName;
	madeTemp = 0;
	opts = *theOpts;
	usePool = 0;
	numThread = 1;
	if(!directoryExists(workDirName)){
		if(directoryCreate(workDirName)){ throw std::runtime_error("Problem creating " + tempName); }
		madeTemp = 1;
	}
}
PODExternalMergeSort::PODExternalMergeSort(const char* workDirName, PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool){
	statusDump = 0;
	maxLoad = 0x040000;
	maxMergeFiles = 256;
	minMergeEnts = 1024;
	numTemps = 0;
	tempName = workDirName;
	madeTemp = 0;
	opts = *theOpts;
	usePool = mainPool;
	numThread = numThr;
	if(!directoryExists(workDirName)){
		if(directoryCreate(workDirName)){ throw std::runtime_error("Problem creating " + tempName); }
		madeTemp = 1;
	}
}
PODExternalMergeSort::~PODExternalMergeSort(){
	for(uintptr_t i = 0; i<allTempBase.size(); i++){
		fileKill(allTempBase[i].c_str());
		fileKill(allTempBlock[i].c_str());
	}
	if(madeTemp){ directoryKill(tempName.c_str()); }
}
void PODExternalMergeSort::addData(uintptr_t numEntries, char* entryStore){
	DeflateCompressionFactory compMeth;
	//make the name of the thing
		std::string baseN;
			whodun_externalSortNewTempName(tempName.c_str(), &baseN, numTemps);
			numTemps++;
		std::string blockN = baseN; blockN.append(".blk");
		allTempBase.push_back(baseN);
		allTempBlock.push_back(blockN);
	//update status
		if(statusDump){
			const char* baseNAC = blockN.c_str();
			statusDump->spitError(WHODUN_ERROR_LEVEL_DEBUG, WHODUN_ERROR_SDESC_UPDATE, __FILE__, __LINE__, "OOM SORT ADD", 1, &baseNAC);
		}
	//make the output
		if(usePool){
			BlockCompOutStream dumpF(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth, numThread, usePool);
			try{dumpF.write(entryStore, numEntries * opts.itemSize);}catch(std::exception& errE){dumpF.close(); throw;}
			dumpF.close();
		}
		else{
			BlockCompOutStream dumpF(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth);
			try{dumpF.write(entryStore, numEntries * opts.itemSize);}catch(std::exception& errE){dumpF.close(); throw;}
			dumpF.close();
		}
}
void PODExternalMergeSort::mergeData(OutStream* toDump){
	DeflateCompressionFactory compMeth;
	//if no files, stop
		if(allTempBase.size() == 0){ return; }
	//if only one file, just dump it
		if(allTempBase.size() == 1){
			char* tmpStore = (char*)malloc(maxLoad);
			InStream* fileA = 0;
			try{
				//open it
				if(usePool){
					fileA = new BlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth, numThread, usePool);
				}
				else{
					fileA = new BlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth);
				}
				//just copy the single file
				uintptr_t numRead = fileA->read(tmpStore, maxLoad);
				while(numRead){
					toDump->write(tmpStore, numRead);
					numRead = fileA->read(tmpStore, maxLoad);
				}
				fileA->close(); delete(fileA); fileA = 0;
				fileKill(allTempBase[0].c_str());
				fileKill(allTempBlock[0].c_str());
				allTempBase.pop_front();
				allTempBlock.pop_front();
			}
			catch(std::exception& errE){
				if(fileA){ try{fileA->close();}catch(std::exception& errB){} delete(fileA); }
				free(tmpStore);
				throw;
			}
			free(tmpStore);
			return;
		}
	//figure out how many entries to load in a single file
		uintptr_t stageSize = maxLoad / 3;
		uintptr_t sinLoadBytes = stageSize / maxMergeFiles;
		uintptr_t sinLoadEnts = sinLoadBytes / opts.itemSize;
			sinLoadEnts = std::max(sinLoadEnts, minMergeEnts);
	//keep spitting while there are a large number of files
		while(allTempBase.size() > maxMergeFiles){
			std::string baseN;
				whodun_externalSortNewTempName(tempName.c_str(), &baseN, numTemps);
				numTemps++;
			std::string blockN = baseN; blockN.append(".blk");
			OutStream* endDump = 0;
			try{
				//update status
					if(statusDump){
						const char* baseNAC[2];
							baseNAC[0] = allTempBase[0].c_str();
							baseNAC[1] = allTempBase[maxMergeFiles-1].c_str();
						statusDump->spitError(WHODUN_ERROR_LEVEL_DEBUG, WHODUN_ERROR_SDESC_UPDATE, __FILE__, __LINE__, "OOM SORT MERGE", 2, baseNAC);
					}
				//open the file
					if(usePool){
						endDump = new BlockCompOutStream(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth, numThread, usePool);
					}
					else{
						endDump = new BlockCompOutStream(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth);
					}
				//merge
					mergeSingle(maxMergeFiles, sinLoadEnts, endDump);
					endDump->close();
					delete(endDump);
			}
			catch(std::exception& errE){
				if(endDump){ try{endDump->close();}catch(std::exception& errB){} delete(endDump); }
				fileKill(baseN.c_str()); fileKill(blockN.c_str());
				throw;
			}
			allTempBase.push_back(baseN);
			allTempBlock.push_back(blockN);
		}
	//do the final merge
		if(statusDump){
			statusDump->spitError(WHODUN_ERROR_LEVEL_DEBUG, WHODUN_ERROR_SDESC_UPDATE, __FILE__, __LINE__, "OOM SORT SPIT", 0, 0);
		}
		mergeSingle(allTempBase.size(), sinLoadEnts, toDump);
}
void PODExternalMergeSort::mergeSingle(uintptr_t numFileOpen, uintptr_t loadEnts, OutStream* toDump){
	uintptr_t i;
	DeflateCompressionFactory compMeth;
	uintptr_t itemSize = opts.itemSize;
	char* mergeArenaA = 0;
	char* mergeArenaB = 0;
	std::vector<PODExternalFileSource*> openFiles;
	std::vector<PODSortedDataMerger*> mergeMeths;
	std::vector<uintptr_t> mergeCount(numFileOpen);
	std::vector<uintptr_t> mergeLens;
	try{
		//open the files
			for(i = 0; i<numFileOpen; i++){
				if(usePool){
					InStream* curRFile = new BlockCompInStream(allTempBase[i].c_str(), allTempBlock[i].c_str(), &compMeth, numThread, usePool);
					openFiles.push_back(new PODExternalFileSource(loadEnts, &opts, curRFile, numThread, usePool));
				}
				else{
					InStream* curRFile = new BlockCompInStream(allTempBase[i].c_str(), allTempBlock[i].c_str(), &compMeth);
					openFiles.push_back(new PODExternalFileSource(loadEnts, &opts, curRFile));
				}
			}
		//allocate ram and mergers
			mergeArenaA = (char*)malloc(loadEnts*numFileOpen*itemSize);
			mergeArenaB = (char*)malloc(loadEnts*numFileOpen*itemSize);
			for(i = 0; i<numFileOpen; i++){
				if(usePool){
					mergeMeths.push_back(new PODSortedDataMerger(&opts, numThread, usePool));
				}
				else{
					mergeMeths.push_back(new PODSortedDataMerger(&opts));
				}
			}
		
		doAnotherLoad:
		//let the things load
		{
			int anyHasSomething = 0;
			for(i = 0; i<openFiles.size(); i++){
				openFiles[i]->load();
				anyHasSomething = anyHasSomething || openFiles[i]->loadSize;
			}
			if(!anyHasSomething){ goto allGone; }
		}
		
		doAnotherMerge:
		//figure out what to merge
		{
			//find the smallest thing at the end of all the files with more stuff (smallest largest)
				char* smallestEnt = 0;
				for(i = 0; i<openFiles.size(); i++){
					PODExternalFileSource* curF = openFiles[i];
					if(curF->haveHitEnd){ continue; }
					if(curF->loadSize){
						char* curExEnt = curF->loadArena + (curF->loadOffset + curF->loadSize - 1)*itemSize;
						if((smallestEnt == 0) || opts.compMeth(opts.useUni, curExEnt, smallestEnt)){
							smallestEnt = curExEnt;
						}
					}
				}
			//figure out how many from each file to merge
				uintptr_t numWithAnyMerge = 0;
				uintptr_t mergeIndex = 0;
				for(i = 0; i<openFiles.size(); i++){
					PODExternalFileSource* curF = openFiles[i];
					PODSortedDataChunk curFD(&opts, curF->loadArena + itemSize*curF->loadOffset, curF->loadSize);
					if(smallestEnt){
						curFD.size = curFD.upperBound(smallestEnt);
					}
					if(curFD.size){
						numWithAnyMerge++;
						mergeIndex = i;
					}
					mergeCount[i] = curFD.size;
				}
			//if only one, just dump it and retry
				if(numWithAnyMerge == 1){
					PODExternalFileSource* curF = openFiles[mergeIndex];
					toDump->write(curF->loadArena + itemSize*curF->loadOffset, itemSize*curF->loadSize);
					curF->loadOffset += curF->loadSize;
					curF->loadSize = 0;
					curF->load();
					goto doAnotherMerge;
				}
			//run the initial merge
				mergeLens.clear();
				char* curMergeTgt = mergeArenaA;
				uintptr_t curI0 = 0;
				uintptr_t curMI = 0;
				i = 0;
				while(i < openFiles.size()){
					char* curElemE = curMergeTgt + curI0*itemSize;
					//get the first thing
						PODExternalFileSource* curF = openFiles[i];
						uintptr_t curNumA = mergeCount[i];
						char* curElemA = curF->loadArena + itemSize*curF->loadOffset;
						curI0 += curNumA;
						curF->loadOffset += curNumA;
						curF->loadSize -= curNumA;
						i++;
					//get the second thing
						uintptr_t curNumB = 0;
						char* curElemB = 0;
						if(i < openFiles.size()){
							curF = openFiles[i];
							curNumB = mergeCount[i];
							curElemB = curF->loadArena + itemSize*curF->loadOffset;
							curF->loadOffset += curNumB;
							curF->loadSize -= curNumB;
						}
						curI0 += curNumB;
						i++;
					//start the merge
						PODSortedDataMerger* curMergeT = mergeMeths[curMI];
						curMergeT->startMerge(curNumA, curElemA, curNumB, curElemB, curElemE);
						mergeLens.push_back(curNumA + curNumB);
						curMI++;
				}
				for(i = 0; i<curMI; i++){
					mergeMeths[i]->joinMerge();
				}
			//do the other merges
				char* curMergeSrc = curMergeTgt;
				curMergeTgt = mergeArenaB;
				while(mergeLens.size() > 1){
					//start the merges
						uintptr_t startNumP = mergeLens.size();
						curI0 = 0;
						curMI = 0;
						i = 0;
						while(i < startNumP){
							char* curElemE = curMergeTgt + curI0*itemSize;
							uintptr_t curNumA = mergeLens[i];
							char* curElemA = curMergeSrc + curI0*itemSize;
							curI0 += curNumA;
							i++;
							char* curElemB = curMergeSrc + curI0*itemSize;
							uintptr_t curNumB = (i < startNumP) ? mergeLens[i] : 0;
							curI0 += curNumB;
							i++;
							PODSortedDataMerger* curMergeT = mergeMeths[curMI];
							curMergeT->startMerge(curNumA, curElemA, curNumB, curElemB, curElemE);
							mergeLens.push_back(curNumA + curNumB);
							curMI++;
						}
					//wait for them to finish
						for(i = 0; i<curMI; i++){
							mergeMeths[i]->joinMerge();
						}
					//prepare for the next round
						char* tmpMerge = curMergeSrc;
						curMergeSrc = curMergeTgt;
						curMergeTgt = tmpMerge;
						mergeLens.erase(mergeLens.begin(), mergeLens.begin() + startNumP);
				}
			//dump and go again
				toDump->write(curMergeSrc, itemSize*mergeLens[0]);
			goto doAnotherLoad;
		}
		
		allGone:
		//clean up
			deleteAll(&openFiles); openFiles.clear();
			deleteAll(&mergeMeths); mergeMeths.clear();
			free(mergeArenaB); mergeArenaB = 0;
			free(mergeArenaA); mergeArenaA = 0;
	}
	catch(std::exception& errE){
		if(mergeArenaA){ free(mergeArenaA); }
		if(mergeArenaB){ free(mergeArenaB); }
		deleteAll(&openFiles);
		deleteAll(&mergeMeths);
	}
}

PODExternalFileSource::PODExternalFileSource(uintptr_t maxLoad, PODSortOptions* theOpts, InStream* fromFile){
	doMemOps = new StandardMemoryShuttler();
	arenaSize = maxLoad;
	opts = *theOpts;
	loadArena = (char*)malloc(arenaSize * opts.itemSize);
	loadOffset = 0;
	loadSize = 0;
	baseFile = fromFile;
	haveHitEnd = 0;
}
PODExternalFileSource::PODExternalFileSource(uintptr_t maxLoad, PODSortOptions* theOpts, InStream* fromFile, uintptr_t numThr, ThreadPool* mainPool){
	doMemOps = new ThreadedMemoryShuttler(numThr, mainPool);
	arenaSize = maxLoad;
	opts = *theOpts;
	loadArena = (char*)malloc(arenaSize * opts.itemSize);
	loadOffset = 0;
	loadSize = 0;
	baseFile = fromFile;
	haveHitEnd = 0;
}
PODExternalFileSource::~PODExternalFileSource(){
	delete(doMemOps);
	free(loadArena);
	baseFile->close();
	delete(baseFile);
}
void PODExternalFileSource::load(){
	uintptr_t itemSize = opts.itemSize;
	if(haveHitEnd){ return; }
	if(loadOffset > (arenaSize / 2)){
		doMemOps->memcpy(loadArena, loadArena + loadOffset*itemSize, loadSize*itemSize);
		loadOffset = 0;
		uintptr_t numEntToLoad = arenaSize - loadSize;
		uintptr_t numByteToLoad = numEntToLoad * itemSize;
		uintptr_t numByteLoad = baseFile->read(loadArena + loadSize*itemSize, numByteToLoad);
		uintptr_t numEntLoad;
		if(numByteLoad != numByteToLoad){
			haveHitEnd = 1;
			if(numByteLoad % itemSize){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Truncated file.", 0, 0); }
			numEntLoad = numByteLoad / itemSize;
		}
		else{
			numEntLoad = numEntToLoad;
		}
		loadSize += numEntLoad;
	}
}

///**Multiple sorted chunks of data.*/
//class PODSortedDataChunks{
//public:
//	/**
//	 * Set up an empty collection.
//	 * @param theOpts Information about the data.
//	 */
//	PODSortedDataChunks(PODSortOptions* theOpts);
//	/**Clean up.*/
//	~PODSortedDataChunks();
//	
//	/**
//	 * Calculate the number of things to each from each chunk to get to a target quantile.
//	 * @param tgtQuantile The number of things to eat.
//	 * @param saveEats The place to put the eat data.
//	 */
//	void quantile(uintptr_t tgtQuantile, uintptr_t* saveEats);
//	
//	/**The number of things in each thing.*/
//	std::vector<uintptr_t> sizes;
//	/**The things in each thing.*/
//	std::vector<char*> data;
//	/**Options for sorting.*/
//	PODSortOptions opts;
//	
//	/**Temporary storage for stuff.*/
//	std::vector<uintptr_t> workSizes;
//	/**Temporary storage for stuff.*/
//	std::vector<char*> workData;
//};

/*
Big-Oh doesn't pencil out for this.

PODSortedDataChunks::PODSortedDataChunks(PODSortOptions* theOpts){
	opts = *theOpts;
}
PODSortedDataChunks::~PODSortedDataChunks(){}
void PODSortedDataChunks::quantile(uintptr_t tgtQuantile, uintptr_t* saveEats){
	uintptr_t quantileLeft = tgtQuantile;
	uintptr_t itemSize = opts.itemSize;
	uintptr_t numChunk = sizes.size();
	workSizes = sizes;
	workData = data;
	memset(saveEats, 0, numChunk*sizeof(uintptr_t));
	for(uintptr_t baseCI = 0; baseCI<numChunk; baseCI++){
		//find where to cut in the current thing
			char* curAData = workData[baseCI];
			uintptr_t curASize = workSizes[baseCI];
			uintptr_t fromALow = 0;
			uintptr_t fromAHig = curASize;
			while(fromAHig - fromALow){
				uintptr_t fromAMid = fromALow + ((fromAHig - fromALow) >> 1);
				char* midVal = curAData + itemSize * fromAMid;
				uintptr_t totalNumEat = fromAMid;
				for(uintptr_t altCI = baseCI + 1; altCI < numChunk; altCI++){
					PODSortedDataChunk curSubC(&opts, workData[altCI], workSizes[altCI]);
					totalNumEat += curSubC.lowerBound(midVal);
				}
				if(totalNumEat < quantileLeft){
					fromALow = fromAMid + 1;
				}
				else{
					fromAHig = fromAMid;
				}
			}
		//add this cut to the eats
			uintptr_t totalNumEat = fromALow;
			saveEats[baseCI] += fromALow;
		//and limit the remainder
			for(uintptr_t altCI = baseCI + 1; altCI < numChunk; altCI++){
				uintptr_t newStartI = 0;
				uintptr_t newEndI = workSizes[altCI];
				PODSortedDataChunk curSubC(&opts, workData[altCI], newEndI);
				if(fromALow < curASize){
					//cannot go past this point
					newEndI = curSubC.upperBound(curAData + fromALow*itemSize);
				}
				if(fromALow){
					//cannot go before the previous point
					newStartI = curSubC.lowerBound(curAData + (fromALow - 1)*itemSize);
				}
				totalNumEat += newStartI;
				saveEats[altCI] += newStartI;
				workData[altCI] += (newStartI * itemSize);
				workSizes[altCI] = (newEndI - newStartI);
			}
			quantileLeft -= totalNumEat;
	}
}
*/

