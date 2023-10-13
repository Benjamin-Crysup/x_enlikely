#include "whodun_gen_graph.h"

#include <math.h>

#include "whodun_compress.h"

namespace whodun {

/**Bulk convert integers.*/
class ChunkySeqGraphIntConvertLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphIntConvertLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphIntConvertLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	
	/**
	 * Load data and convert to integer.
	 * @param loadFrom The file to load from.
	 * @param tmpLoad Temporary storage for bytes.
	 * @param loadTo The place to put the converted values.
	 * @param numLoad The number of bytes to load.
	 * @param usePool The pool to use, if any.
	 */
	void loadAndConvert(InStream* loadFrom, StructVector<char>* tmpLoad, StructVector<uintptr_t>* loadTo, intptr_t numLoad, ThreadPool* usePool);
	
	/**The data to convert from.*/
	char* convFrom;
	/**The place to put the converted data.*/
	uintptr_t* convTo;
};

/**Bulk convert floats.*/
class ChunkySeqGraphFloatConvertLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphFloatConvertLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphFloatConvertLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	
	/**
	 * Load data and convert to float.
	 * @param loadFrom The file to load from.
	 * @param tmpLoad Temporary storage for bytes.
	 * @param loadTo The place to put the converted values.
	 * @param numLoad The number of bytes to load.
	 * @param usePool The pool to use, if any.
	 */
	void loadAndConvert(InStream* loadFrom, StructVector<char>* tmpLoad, StructVector<float>* loadTo, intptr_t numLoad, ThreadPool* usePool);
	
	/**The data to convert from.*/
	char* convFrom;
	/**The place to put the converted data.*/
	float* convTo;
};

/**Bulk convert integers.*/
class ChunkySeqGraphIntPackLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphIntPackLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphIntPackLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	
	/**
	 * Convert integers and dump.
	 * @param loadTo The integers to convert.
	 * @param tmpLoad The place to store bytes in the interim.
	 * @param dumpTo The place to write.
	 * @param usePool The threads to use, or null if none.
	 */
	void convertAndDump(StructVector<uintptr_t>* loadTo, StructVector<char>* tmpLoad, OutStream* dumpTo, ThreadPool* usePool);
	
	/**The data to convert from.*/
	char* convFrom;
	/**The place to put the converted data.*/
	uintptr_t* convTo;
};

/**Bulk convert floats.*/
class ChunkySeqGraphFloatPackLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphFloatPackLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphFloatPackLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	void doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI);
	
	/**
	 * Convert floats and dump.
	 * @param loadTo The integers to convert.
	 * @param tmpLoad The place to store bytes in the interim.
	 * @param dumpTo The place to write.
	 * @param usePool The threads to use, or null if none.
	 */
	void convertAndDump(StructVector<float>* loadTo, StructVector<char>* tmpLoad, OutStream* dumpTo, ThreadPool* usePool);
	
	/**The data to convert from.*/
	char* convFrom;
	/**The place to put the converted data.*/
	float* convTo;
};

/**Unpack graph data.*/
class ChunkySeqGraphReadLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphReadLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphReadLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**The index data.*/
	char* annotData;
	/**The topology to work over.*/
	SeqGraphDataSet* toStore;
	/**The offset of the first name.*/
	uintmax_t baseNameO;
	/**The offset of the first size.*/
	uintmax_t baseConSO;
	/**The offset of the first link input.*/
	uintmax_t baseLInO;
	/**The offset of the first link output*/
	uintmax_t baseLOutO;
	/**The offset of the first contig probability.*/
	uintmax_t baseCProO;
	/**The offset of the first link probability.*/
	uintmax_t baseLProO;
	/**The offset of the first representative sequence.*/
	uintmax_t baseSeqO;
	/**The offset of the first suffix.*/
	uintmax_t baseSuffO;
	/**The offset of the first extra thing.*/
	uintmax_t baseExtraO;
	/**The number of unique bases in play.*/
	uintptr_t numBaseM;
};

//TODO

};


using namespace whodun;

SeqGraphHeader::SeqGraphHeader(){
	name.len = 0;
	version = 0;
	baseMap.len = 0;
}
SeqGraphHeader::~SeqGraphHeader(){}
SeqGraphHeader::SeqGraphHeader(const SeqGraphHeader& toCopy){
	saveText.insert(saveText.size(), toCopy.name.txt, toCopy.name.txt + toCopy.name.len);
	saveText.insert(saveText.size(), toCopy.baseMap.txt, toCopy.baseMap.txt + toCopy.baseMap.len);
	name.txt = saveText[0];
	name.len = toCopy.name.len;
	baseMap.txt = saveText[name.len];
	baseMap.len = toCopy.baseMap.len;
	version = toCopy.version;
}
SeqGraphHeader& SeqGraphHeader::operator=(const SeqGraphHeader& newVal){
	saveText.clear();
	saveText.insert(saveText.size(), newVal.name.txt, newVal.name.txt + newVal.name.len);
	saveText.insert(saveText.size(), newVal.baseMap.txt, newVal.baseMap.txt + newVal.baseMap.len);
	name.txt = saveText[0];
	name.len = newVal.name.len;
	baseMap.txt = saveText[name.len];
	baseMap.len = newVal.baseMap.len;
	return *this;
}
void SeqGraphHeader::read(InStream* readFrom){
	saveText.clear();
	char lenStore[8];
	ByteUnpacker doUPack(lenStore);
	//name
		readFrom->forceRead(lenStore,8);
		doUPack.retarget(lenStore);
		name.len = doUPack.unpackBE64();
		uintptr_t nameSTL = saveText.size();
		saveText.resize(nameSTL + name.len);
		readFrom->forceRead(saveText[nameSTL], name.len);
	//version
		readFrom->forceRead(lenStore,8);
		doUPack.retarget(lenStore);
		version = doUPack.unpackBE64();
	//basemap
		readFrom->forceRead(lenStore,8);
		doUPack.retarget(lenStore);
		baseMap.len = doUPack.unpackBE64();
		uintptr_t bmapSTL = saveText.size();
		saveText.resize(bmapSTL + baseMap.len);
		readFrom->forceRead(saveText[bmapSTL], baseMap.len);
	//link things up
		name.txt = saveText[nameSTL];
		baseMap.txt = saveText[bmapSTL];
}
void SeqGraphHeader::write(OutStream* dumpTo){
	char lenStore[8];
	BytePacker doPack(lenStore);
	//name
		doPack.retarget(lenStore);
		doPack.packBE64(name.len);
		dumpTo->write(lenStore, 8);
		dumpTo->write(name.txt, name.len);
	//version
		doPack.retarget(lenStore);
		doPack.packBE64(version);
		dumpTo->write(lenStore, 8);
	//basemap
		doPack.retarget(lenStore);
		doPack.packBE64(baseMap.len);
		dumpTo->write(lenStore, 8);
		dumpTo->write(baseMap.txt, baseMap.len);
}

SeqGraphDataSet::SeqGraphDataSet(){}
SeqGraphDataSet::~SeqGraphDataSet(){}

SeqGraphReader::SeqGraphReader(){
	isClosed = 0;
}
SeqGraphReader::~SeqGraphReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

void RandacSeqGraphReader::readRange(uintmax_t fromIndex, uintmax_t toIndex, SeqGraphDataSet* toStore){
	seek(fromIndex);
	read(toIndex - fromIndex, toStore);
}

SeqGraphWriter::SeqGraphWriter(){
	isClosed = 0;
}
SeqGraphWriter::~SeqGraphWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

ChunkySeqGraphIntConvertLoop::ChunkySeqGraphIntConvertLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 4096;
}
ChunkySeqGraphIntConvertLoop::~ChunkySeqGraphIntConvertLoop(){}
void ChunkySeqGraphIntConvertLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	ByteUnpacker curUP(convFrom + 8*ind);
	convTo[ind] = curUP.unpackBE64();
}
void ChunkySeqGraphIntConvertLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	
	ByteUnpacker curUP(convFrom + 8*fromI);
	for(uintptr_t i = fromI; i<toI; i++){
		convTo[i] = curUP.unpackBE64();
	}
	
	doRangeEnd(threadInd, fromI, toI);
}
void ChunkySeqGraphIntConvertLoop::loadAndConvert(InStream* loadFrom, StructVector<char>* tmpLoad, StructVector<uintptr_t>* loadTo, intptr_t numLoad, ThreadPool* usePool){
	if(numLoad < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Trying to load a negative number of integers.", 0, 0); }
	if(numLoad % 8){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Truncated integer.", 0, 0); }
	tmpLoad->clear(); tmpLoad->resize(numLoad);
	loadTo->clear(); loadTo->resize(numLoad/8);
	convFrom = tmpLoad->at(0);
	loadFrom->forceRead(convFrom,numLoad);
	convTo = loadTo->at(0);
	if(usePool){
		doIt(usePool, 0, numLoad / 8);
	}
	else{
		doIt(0, numLoad / 8);
	}
}

ChunkySeqGraphFloatConvertLoop::ChunkySeqGraphFloatConvertLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 4096;
}
ChunkySeqGraphFloatConvertLoop::~ChunkySeqGraphFloatConvertLoop(){}
void ChunkySeqGraphFloatConvertLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	ByteUnpacker curUP(convFrom + 4*ind);
	convTo[ind] = curUP.unpackBEFlt();
}
void ChunkySeqGraphFloatConvertLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	
	ByteUnpacker curUP(convFrom + 4*fromI);
	for(uintptr_t i = fromI; i<toI; i++){
		convTo[i] = curUP.unpackBEFlt();
	}
	
	doRangeEnd(threadInd, fromI, toI);
}
void ChunkySeqGraphFloatConvertLoop::loadAndConvert(InStream* loadFrom, StructVector<char>* tmpLoad, StructVector<float>* loadTo, intptr_t numLoad, ThreadPool* usePool){
	if(numLoad < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Trying to load a negative number of floats.", 0, 0); }
	if(numLoad % 4){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Truncated float.", 0, 0); }
	tmpLoad->clear(); tmpLoad->resize(numLoad);
	loadTo->clear(); loadTo->resize(numLoad/4);
	convFrom = tmpLoad->at(0);
	loadFrom->forceRead(convFrom,numLoad);
	convTo = loadTo->at(0);
	if(usePool){
		doIt(usePool, 0, numLoad / 4);
	}
	else{
		doIt(0, numLoad / 4);
	}
}

ChunkySeqGraphIntPackLoop::ChunkySeqGraphIntPackLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 4096;
}
ChunkySeqGraphIntPackLoop::~ChunkySeqGraphIntPackLoop(){}
void ChunkySeqGraphIntPackLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	BytePacker curP(convFrom + 8*ind);
	curP.packBE64(convTo[ind]);
}
void ChunkySeqGraphIntPackLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	
	BytePacker curP(convFrom + 8*fromI);
	for(uintptr_t i = fromI; i<toI; i++){
		curP.packBE64(convTo[i]);
	}
	
	doRangeEnd(threadInd, fromI, toI);
}
void ChunkySeqGraphIntPackLoop::convertAndDump(StructVector<uintptr_t>* loadTo, StructVector<char>* tmpLoad, OutStream* dumpTo, ThreadPool* usePool){
	uintptr_t numLoad = loadTo->size();
	tmpLoad->clear(); tmpLoad->resize(8*numLoad);
	convFrom = tmpLoad->at(0);
	convTo = loadTo->at(0);
	if(usePool){
		doIt(usePool, 0, numLoad);
	}
	else{
		doIt(0, numLoad);
	}
	dumpTo->write(convFrom, 8*numLoad);
}
ChunkySeqGraphFloatPackLoop::ChunkySeqGraphFloatPackLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 4096;
}
ChunkySeqGraphFloatPackLoop::~ChunkySeqGraphFloatPackLoop(){}
void ChunkySeqGraphFloatPackLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	BytePacker curP(convFrom + 4*ind);
	curP.packBEFlt(convTo[ind]);
}
void ChunkySeqGraphFloatPackLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	doRangeStart(threadInd, fromI, toI);
	
	BytePacker curP(convFrom + 4*fromI);
	for(uintptr_t i = fromI; i<toI; i++){
		curP.packBEFlt(convTo[i]);
	}
	
	doRangeEnd(threadInd, fromI, toI);
}
void ChunkySeqGraphFloatPackLoop::convertAndDump(StructVector<float>* loadTo, StructVector<char>* tmpLoad, OutStream* dumpTo, ThreadPool* usePool){
	uintptr_t numLoad = loadTo->size();
	tmpLoad->clear(); tmpLoad->resize(4*numLoad);
	convFrom = tmpLoad->at(0);
	convTo = loadTo->at(0);
	if(usePool){
		doIt(usePool, 0, numLoad);
	}
	else{
		doIt(0, numLoad);
	}
	dumpTo->write(convFrom, 4*numLoad);
}

/**name, contig size, link in, link out, contig probs, link probs, seq, suffix arr, extra, num links*/
#define CHUNKY_GRAPH_HEAD_SIZE 80

ChunkySeqGraphReader::ChunkySeqGraphReader(RandaccInStream* fileInd, RandaccInStream* fileName, RandaccInStream* fileCSize, RandaccInStream* fileILnk, RandaccInStream* fileOLnk, RandaccInStream* fileCPro, RandaccInStream* fileLPro, RandaccInStream* fileRSeq, RandaccInStream* fileSuff, RandaccInStream* fileExtra){
	thisHead.read(fileInd);
	needSeek = 1;
	headOffset = fileInd->tell();
	numGraphs = fileInd->size() - headOffset;
	if(numGraphs % CHUNKY_GRAPH_HEAD_SIZE){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed header for sequence graph data.", 0, 0); }
	numGraphs = numGraphs / CHUNKY_GRAPH_HEAD_SIZE;
	nextGraph = 0;
	doConv = new ChunkySeqGraphReadLoop(1);
	doIConv = new ChunkySeqGraphIntConvertLoop(1);
	doFConv = new ChunkySeqGraphFloatConvertLoop(1);
	usePool = 0;
	fInd = fileInd;
	fName = fileName;
	fCSize = fileCSize;
	fILnk = fileILnk;
	fOLnk = fileOLnk;
	fCPro = fileCPro;
	fLPro = fileLPro;
	fRSeq = fileRSeq;
	fSuff = fileSuff;
	fExtra = fileExtra;
}
ChunkySeqGraphReader::ChunkySeqGraphReader(RandaccInStream* fileInd, RandaccInStream* fileName, RandaccInStream* fileCSize, RandaccInStream* fileILnk, RandaccInStream* fileOLnk, RandaccInStream* fileCPro, RandaccInStream* fileLPro, RandaccInStream* fileRSeq, RandaccInStream* fileSuff, RandaccInStream* fileExtra, uintptr_t numThread, ThreadPool* mainPool){
	thisHead.read(fileInd);
	needSeek = 1;
	headOffset = fileInd->tell();
	numGraphs = fileInd->size() - headOffset;
	if(numGraphs % CHUNKY_GRAPH_HEAD_SIZE){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed header for sequence graph data.", 0, 0); }
	numGraphs = numGraphs / CHUNKY_GRAPH_HEAD_SIZE;
	nextGraph = 0;
	doConv = new ChunkySeqGraphReadLoop(numThread);
	doIConv = new ChunkySeqGraphIntConvertLoop(numThread);
	doFConv = new ChunkySeqGraphFloatConvertLoop(numThread);
	usePool = mainPool;
	fInd = fileInd;
	fName = fileName;
	fCSize = fileCSize;
	fILnk = fileILnk;
	fOLnk = fileOLnk;
	fCPro = fileCPro;
	fLPro = fileLPro;
	fRSeq = fileRSeq;
	fSuff = fileSuff;
	fExtra = fileExtra;
}
ChunkySeqGraphReader::~ChunkySeqGraphReader(){
	delete(doConv);
	delete(doIConv);
	delete(doFConv);
}
uintptr_t ChunkySeqGraphReader::read(uintptr_t numSeqs, SeqGraphDataSet* toStore){
	if(numSeqs == 0){ return 0; }
	//seek if necessary
		if(needSeek){
			if(nextGraph < numGraphs){
				fInd->seek(CHUNKY_GRAPH_HEAD_SIZE*nextGraph + headOffset);
				saveInd.resize(CHUNKY_GRAPH_HEAD_SIZE);
				fInd->forceRead(saveInd[0], CHUNKY_GRAPH_HEAD_SIZE);
				ByteUnpacker getOffV(saveInd[0]);
				fName->seek(getOffV.unpackBE64());
				fCSize->seek(getOffV.unpackBE64());
				fILnk->seek(getOffV.unpackBE64());
				fOLnk->seek(getOffV.unpackBE64());
				fCPro->seek(getOffV.unpackBE64());
				fLPro->seek(getOffV.unpackBE64());
				fRSeq->seek(getOffV.unpackBE64());
				fSuff->seek(getOffV.unpackBE64());
				fExtra->seek(getOffV.unpackBE64());
			}
			needSeek = 0;
		}
	//figure out how much to read
		uintmax_t endFocInd = nextGraph + numSeqs;
			endFocInd = std::min(endFocInd, numGraphs);
		uintptr_t numRealRead = endFocInd - nextGraph;
		int willHitEOF = endFocInd == numGraphs;
		if(numRealRead == 0){ return 0; }
	//load the annotation
		saveInd.resize(CHUNKY_GRAPH_HEAD_SIZE*(numRealRead + 1));
		fInd->forceRead(saveInd[CHUNKY_GRAPH_HEAD_SIZE], CHUNKY_GRAPH_HEAD_SIZE*(numRealRead - willHitEOF));
		if(willHitEOF){
			BytePacker packOffV(saveInd[CHUNKY_GRAPH_HEAD_SIZE*numRealRead]);
			packOffV.packBE64(fName->size());
			packOffV.packBE64(fCSize->size());
			packOffV.packBE64(fILnk->size());
			packOffV.packBE64(fOLnk->size());
			packOffV.packBE64(fCPro->size());
			packOffV.packBE64(fLPro->size());
			packOffV.packBE64(fRSeq->size());
			packOffV.packBE64(fSuff->size());
			packOffV.packBE64(fExtra->size());
		}
	//load the data
	{
		ByteUnpacker getStartO(saveInd[0]);
		ByteUnpacker getEndO(saveInd[CHUNKY_GRAPH_HEAD_SIZE*numRealRead]);
		ChunkySeqGraphIntConvertLoop* intConv = (ChunkySeqGraphIntConvertLoop*)doIConv;
		ChunkySeqGraphFloatConvertLoop* fltConv = (ChunkySeqGraphFloatConvertLoop*)doFConv;
		//name
			intptr_t numNameD = getEndO.unpackBE64() - getStartO.unpackBE64();
			if(numNameD < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Trying to read a negative number of characters.", 0, 0); }
			toStore->nameTexts.clear(); toStore->nameTexts.resize(numNameD); fName->forceRead(toStore->nameTexts[0], numNameD);
		//contig sizes
			intConv->loadAndConvert(fCSize, &tmpText, &(toStore->contigSizes), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//input link data
			intConv->loadAndConvert(fILnk, &tmpText, &(toStore->inputLinkData), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//output link data
			intConv->loadAndConvert(fOLnk, &tmpText, &(toStore->outputLinkData), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//contig probs
			fltConv->loadAndConvert(fCPro, &tmpText, &(toStore->contigProbs), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//link probabilities
			fltConv->loadAndConvert(fLPro, &tmpText, &(toStore->linkProbs), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//sequence
			intptr_t numSeqD = getEndO.unpackBE64() - getStartO.unpackBE64();
			if(numSeqD < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Trying to read a negative number of characters.", 0, 0); }
			toStore->seqTexts.clear(); toStore->seqTexts.resize(numSeqD); fRSeq->forceRead(toStore->seqTexts[0], numSeqD);
		//suffix array data
			intConv->loadAndConvert(fSuff, &tmpText, &(toStore->suffixIndices), getEndO.unpackBE64() - getStartO.unpackBE64(), usePool);
		//extras
			intptr_t numExtD = getEndO.unpackBE64() - getStartO.unpackBE64();
			if(numExtD < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Trying to read a negative number of characters.", 0, 0); }
			toStore->extraTexts.clear(); toStore->extraTexts.resize(numExtD); fRSeq->forceRead(toStore->extraTexts[0], numExtD);
	}
	//make some space
		toStore->nameLengths.clear(); toStore->nameLengths.resize(numRealRead);
		toStore->numContigs.clear(); toStore->numContigs.resize(numRealRead);
		toStore->numLinks.clear(); toStore->numLinks.resize(numRealRead);
		toStore->numLinkInputs.clear(); toStore->numLinkInputs.resize(numRealRead);
		toStore->numLinkOutputs.clear(); toStore->numLinkOutputs.resize(numRealRead);
		toStore->numBases.clear(); toStore->numBases.resize(numRealRead);
		toStore->extraLengths.clear(); toStore->extraLengths.resize(numRealRead);
	//prepare to pack
		ByteUnpacker getStartO(saveInd[0]);
		ChunkySeqGraphReadLoop* finalFill = (ChunkySeqGraphReadLoop*)doConv;
		finalFill->annotData = saveInd[0];
		finalFill->toStore = toStore;
		finalFill->baseNameO = getStartO.unpackBE64();
		finalFill->baseConSO = getStartO.unpackBE64();
		finalFill->baseLInO = getStartO.unpackBE64();
		finalFill->baseLOutO = getStartO.unpackBE64();
		finalFill->baseCProO = getStartO.unpackBE64();
		finalFill->baseLProO = getStartO.unpackBE64();
		finalFill->baseSeqO = getStartO.unpackBE64();
		finalFill->baseSuffO = getStartO.unpackBE64();
		finalFill->baseExtraO = getStartO.unpackBE64();
		finalFill->numBaseM = thisHead.baseMap.len;
		if(usePool){ finalFill->doIt(usePool, 0, numRealRead); }
		else{ finalFill->doIt(0, numRealRead); }
	//and get ready for the next round
		memcpy(saveInd[0], saveInd[CHUNKY_GRAPH_HEAD_SIZE*numRealRead], CHUNKY_GRAPH_HEAD_SIZE);
		nextGraph = endFocInd;
	return numRealRead;
}
void ChunkySeqGraphReader::close(){
	isClosed = 1;
}
uintmax_t ChunkySeqGraphReader::size(){
	return numGraphs;
}
void ChunkySeqGraphReader::seek(uintmax_t index){
	if(index == nextGraph){ return; }
	needSeek = 1;
	nextGraph = index;
}

ChunkySeqGraphReadLoop::ChunkySeqGraphReadLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 1024;
}
ChunkySeqGraphReadLoop::~ChunkySeqGraphReadLoop(){}
void ChunkySeqGraphReadLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	//unpack the annotation stuff
		ByteUnpacker curUP(annotData + CHUNKY_GRAPH_HEAD_SIZE*ind);
		uintmax_t curNameO = curUP.unpackBE64();
		uintmax_t curConSO = curUP.unpackBE64();
		uintmax_t curLInO = curUP.unpackBE64();
		uintmax_t curLOutO = curUP.unpackBE64();
		uintmax_t curCProO = curUP.unpackBE64();
		uintmax_t curLProO = curUP.unpackBE64();
		uintmax_t curSeqO = curUP.unpackBE64();
		uintmax_t curSuffO = curUP.unpackBE64();
		uintmax_t curExtraO = curUP.unpackBE64();
		*(toStore->numLinks[ind]) = curUP.unpackBE64();
		uintmax_t nextNameO = curUP.unpackBE64();
		uintmax_t nextConSO = curUP.unpackBE64();
		uintmax_t nextLInO = curUP.unpackBE64();
		uintmax_t nextLOutO = curUP.unpackBE64();
		uintmax_t nextCProO = curUP.unpackBE64();
		uintmax_t nextLProO = curUP.unpackBE64();
		uintmax_t nextSeqO = curUP.unpackBE64();
		uintmax_t nextSuffO = curUP.unpackBE64();
		uintmax_t nextExtraO = curUP.unpackBE64();
	//names
		if((nextNameO < curNameO) || (curNameO < baseNameO) || ((nextNameO - baseNameO) > toStore->nameTexts.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Name offsets not monotonic.", 0, 0);
		}
		*(toStore->nameLengths[ind]) = nextNameO - curNameO;
	//number of contigs
		if((curConSO % 8) || (nextConSO % 8)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Size offsets not aligned.", 0, 0);
		}
		if((nextConSO < curConSO) || (curConSO < baseConSO) || ((nextConSO - baseConSO)/8 > toStore->contigSizes.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Size offsets not monotonic.", 0, 0);
		}
		uintptr_t numContig = (nextConSO - curConSO)/8;
		*(toStore->numContigs[ind]) = numContig;
	//number of link inputs
		if((curLInO % 16) || (nextLInO % 16)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Input link offsets not aligned.", 0, 0);
		}
		if((nextLInO < curLInO) || (curLInO < baseLInO) || ((nextLInO - baseLInO)/8 > toStore->inputLinkData.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Input link offsets not monotonic.", 0, 0);
		}
		*(toStore->numLinkInputs[ind]) = (nextLInO - curLInO)/16;
	//number of link outputs
		if((curLOutO % 16) || (nextLOutO % 16)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Output link offsets not aligned.", 0, 0);
		}
		if((nextLOutO < curLOutO) || (curLOutO < baseLOutO) || ((nextLOutO - baseLOutO)/8 > toStore->outputLinkData.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Output link offsets not monotonic.", 0, 0);
		}
		uintptr_t numLinkOut = (nextLOutO - curLOutO)/16;
		*(toStore->numLinkOutputs[ind]) = numLinkOut;
	//figure the number of bases
		if((nextSeqO < curSeqO) || (curSeqO < baseSeqO) || ((nextSeqO - baseSeqO) > toStore->seqTexts.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Sequence offsets not monotonic.", 0, 0);
		}
		uintptr_t numBase = nextSeqO - curSeqO;
		*(toStore->numBases[ind]) = numBase;
	//figure the extra crap
		if((nextExtraO < curExtraO) || (curExtraO < baseExtraO) || ((nextExtraO - baseExtraO) > toStore->extraTexts.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Extra offsets not monotonic.", 0, 0);
		}
		*(toStore->extraLengths[ind]) = nextExtraO - curExtraO;
	//make sure the contig probabilities make sense
		if((curCProO % 4) || (nextCProO % 4)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Straight-shot sequence probability offsets not aligned.", 0, 0);
		}
		if((nextCProO < curCProO) || (curCProO < baseCProO) || ((nextCProO - baseCProO)/4 > toStore->contigProbs.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Straight-shot sequence probability offsets not monotonic.", 0, 0);
		}
		uintptr_t numCProEnts = (nextCProO - curCProO) / 4;
		uintptr_t expCProEnts = (3+numBaseM)*numBase + 3*numContig;
		if(numCProEnts != expCProEnts){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Straight-shot sequence probabilities have incorrect size.", 0, 0);
		}
	//make sure the link probabilities make sense
		if((curLProO % 4) || (nextLProO % 4)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Link probability offsets not aligned.", 0, 0);
		}
		if((nextLProO < curLProO) || (curLProO < baseLProO) || ((nextLProO - baseLProO)/4 > toStore->linkProbs.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Link probability offsets not monotonic.", 0, 0);
		}
		if(numLinkOut != (nextLProO - curLProO)/4){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Link probabilities have incorrect size.", 0, 0);
		}
	//make sure the suffix array makes sense
		if((curSuffO % 8) || (nextSuffO % 8)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Suffix array offsets not aligned.", 0, 0);
		}
		if((nextSuffO < curSuffO) || (curSuffO < baseSuffO) || ((nextSuffO - baseSuffO)/8 > toStore->suffixIndices.size())){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Suffix array offsets not monotonic.", 0, 0);
		}
		if(((nextSuffO - curSuffO)/8) != numBase){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Suffix array does not match sequence.", 0, 0);
		}
}




ChunkySeqGraphWriter::ChunkySeqGraphWriter(SeqGraphHeader* theHead, OutStream* fileInd, OutStream* fileName, OutStream* fileCSize, OutStream* fileILnk, OutStream* fileOLnk, OutStream* fileCPro, OutStream* fileLPro, OutStream* fileRSeq, OutStream* fileSuff, OutStream* fileExtra){
	thisHead = *theHead;
	thisHead.write(fileInd);
	fInd = fileInd;
	fName = fileName;
	fCSize = fileCSize;
	fILnk = fileILnk;
	fOLnk = fileOLnk;
	fCPro = fileCPro;
	fLPro = fileLPro;
	fRSeq = fileRSeq;
	fSuff = fileSuff;
	fExtra = fileExtra;
	numName = 0;
	numCSize = 0;
	numILnk = 0;
	numOLnk = 0;
	numCPro = 0;
	numLPro = 0;
	numRSeq = 0;
	numSuff = 0;
	numExtra = 0;
	doIConv = new ChunkySeqGraphIntPackLoop(1);
	doFConv = new ChunkySeqGraphFloatPackLoop(1);
	usePool = 0;
}
ChunkySeqGraphWriter::ChunkySeqGraphWriter(SeqGraphHeader* theHead, OutStream* fileInd, OutStream* fileName, OutStream* fileCSize, OutStream* fileILnk, OutStream* fileOLnk, OutStream* fileCPro, OutStream* fileLPro, OutStream* fileRSeq, OutStream* fileSuff, OutStream* fileExtra, uintptr_t numThread, ThreadPool* mainPool){
	thisHead = *theHead;
	thisHead.write(fileInd);
	fInd = fileInd;
	fName = fileName;
	fCSize = fileCSize;
	fILnk = fileILnk;
	fOLnk = fileOLnk;
	fCPro = fileCPro;
	fLPro = fileLPro;
	fRSeq = fileRSeq;
	fSuff = fileSuff;
	fExtra = fileExtra;
	numName = 0;
	numCSize = 0;
	numILnk = 0;
	numOLnk = 0;
	numCPro = 0;
	numLPro = 0;
	numRSeq = 0;
	numSuff = 0;
	numExtra = 0;
	doIConv = new ChunkySeqGraphIntPackLoop(numThread);
	doFConv = new ChunkySeqGraphFloatPackLoop(numThread);
	usePool = mainPool;
}
ChunkySeqGraphWriter::~ChunkySeqGraphWriter(){
	delete(doIConv);
	delete(doFConv);
}
void ChunkySeqGraphWriter::write(SeqGraphDataSet* toStore){
	uintptr_t curNumOut = toStore->nameLengths.size();
	//write out the annotation data
		saveInd.clear(); saveInd.resize(CHUNKY_GRAPH_HEAD_SIZE*curNumOut);
		BytePacker curAP(saveInd[0]);
		uintptr_t* nameLenArr = toStore->nameLengths[0];
		uintptr_t* numContArr = toStore->numContigs[0];
		uintptr_t* numLinkArr = toStore->numLinks[0];
		uintptr_t* numLinIArr = toStore->numLinkInputs[0];
		uintptr_t* numLinOArr = toStore->numLinkOutputs[0];
		uintptr_t* numBaseArr = toStore->numBases[0];
		uintptr_t* numExtCArr = toStore->extraLengths[0];
		for(uintptr_t i = 0; i<curNumOut; i++){
			uintptr_t curNumB = numBaseArr[i];
			uintptr_t curNumL = numLinkArr[i];
			uintptr_t curNumCon = numContArr[i];
			uintptr_t curNumOL = numLinOArr[i];
			//pack the data
				curAP.packBE64(numName);
				curAP.packBE64(numCSize);
				curAP.packBE64(numILnk);
				curAP.packBE64(numOLnk);
				curAP.packBE64(numCPro);
				curAP.packBE64(numLPro);
				curAP.packBE64(numRSeq);
				curAP.packBE64(numSuff);
				curAP.packBE64(numExtra);
				curAP.packBE64(curNumL);
			//update the offsets
				numName += nameLenArr[i];
				numCSize += (8 * curNumCon);
				numILnk += (16 * numLinIArr[i]);
				numOLnk += (16 * curNumOL);
				numCPro += (4 * ((thisHead.baseMap.len+3)*curNumB + 3*curNumCon));
				numLPro += (4 * curNumOL);
				numRSeq += curNumB;
				numSuff += (8 * curNumB);
				numExtra += numExtCArr[i];
		}
		fInd->write(saveInd[0], saveInd.size());
	//write out the pieces
		ChunkySeqGraphIntPackLoop* doIP = (ChunkySeqGraphIntPackLoop*)doIConv;
		ChunkySeqGraphFloatPackLoop* doFP = (ChunkySeqGraphFloatPackLoop*)doFConv;
		fName->write(toStore->nameTexts[0], toStore->nameTexts.size());
		doIP->convertAndDump(&(toStore->contigSizes), &tmpText, fCSize, usePool);
		doIP->convertAndDump(&(toStore->inputLinkData), &tmpText, fILnk, usePool);
		doIP->convertAndDump(&(toStore->outputLinkData), &tmpText, fOLnk, usePool);
		doFP->convertAndDump(&(toStore->contigProbs), &tmpText, fCPro, usePool);
		doFP->convertAndDump(&(toStore->linkProbs), &tmpText, fLPro, usePool);
		fRSeq->write(toStore->seqTexts[0], toStore->seqTexts.size());
		doIP->convertAndDump(&(toStore->suffixIndices), &tmpText, fSuff, usePool);
		fExtra->write(toStore->extraTexts[0], toStore->extraTexts.size());
}
void ChunkySeqGraphWriter::close(){
	isClosed = 1;
}

ExtensionSeqGraphReader::ExtensionSeqGraphReader(const char* fileName, InStream* useStdin){
	openUp(fileName, 1, 0, useStdin);
}
ExtensionSeqGraphReader::ExtensionSeqGraphReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	openUp(fileName, numThread, mainPool, useStdin);
}
ExtensionSeqGraphReader::~ExtensionSeqGraphReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionSeqGraphReader::read(uintptr_t numSeqs, SeqGraphDataSet* toStore){
	return wrapStr->read(numSeqs, toStore);
}
void ExtensionSeqGraphReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionSeqGraphReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Raw text sequence graph not yet supported.", 0, 0);
		}
		//anything block compressed
		{
			int isBCseq = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bsgrap"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bsgrap"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bsgrap"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bsgrap"));
			if(isBCseq){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indexFN(fileName);
				std::string curFN;
				std::string curBN;
				std::vector<RandaccInStream*> saveStrs;
				RandaccInStream* curStr;
				#define EXTENIS_OPEN_FILE(nameSuff) \
					curFN.clear(); curFN.append(fileName); curFN.append(nameSuff);\
					curBN.clear(); curBN.append(curFN); curBN.append(".blk");\
					curStr = mainPool ? new BlockCompInStream(curFN.c_str(), curBN.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(curFN.c_str(), curBN.c_str(), compMeth);\
					baseStrs.push_back(curStr); saveStrs.push_back(curStr);
				EXTENIS_OPEN_FILE("")
				EXTENIS_OPEN_FILE(".name")
				EXTENIS_OPEN_FILE(".csize")
				EXTENIS_OPEN_FILE(".ilink")
				EXTENIS_OPEN_FILE(".olink")
				EXTENIS_OPEN_FILE(".cpro")
				EXTENIS_OPEN_FILE(".lpro")
				EXTENIS_OPEN_FILE(".rseq")
				EXTENIS_OPEN_FILE(".sa")
				EXTENIS_OPEN_FILE(".ext")
				wrapStr = mainPool ? new ChunkySeqGraphReader(saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9],numThread,mainPool) : new ChunkySeqGraphReader(saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9]);
				delete(compMeth);
				return;
			}
		}
		//try to treat it as text
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Raw text sequence graph not yet supported.", 0, 0);
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}

ExtensionRandacSeqGraphReader::ExtensionRandacSeqGraphReader(const char* fileName){
	openUp(fileName, 1, 0);
}
ExtensionRandacSeqGraphReader::ExtensionRandacSeqGraphReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	openUp(fileName, numThread, mainPool);
}
ExtensionRandacSeqGraphReader::~ExtensionRandacSeqGraphReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionRandacSeqGraphReader::read(uintptr_t numSeqs, SeqGraphDataSet* toStore){
	return wrapStr->read(numSeqs, toStore);
}
void ExtensionRandacSeqGraphReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
uintmax_t ExtensionRandacSeqGraphReader::size(){
	return wrapStr->size();
}
void ExtensionRandacSeqGraphReader::seek(uintmax_t index){
	wrapStr->seek(index);
}
void ExtensionRandacSeqGraphReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Cannot jump on stdins.", 0, 0);
		}
		//anything block compressed
		{
			int isBCseq = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bsgrap"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bsgrap"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bsgrap"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bsgrap"));
			if(isBCseq){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indexFN(fileName);
				std::string curFN;
				std::string curBN;
				std::vector<RandaccInStream*> saveStrs;
				RandaccInStream* curStr;
				EXTENIS_OPEN_FILE("")
				EXTENIS_OPEN_FILE(".name")
				EXTENIS_OPEN_FILE(".csize")
				EXTENIS_OPEN_FILE(".ilink")
				EXTENIS_OPEN_FILE(".olink")
				EXTENIS_OPEN_FILE(".cpro")
				EXTENIS_OPEN_FILE(".lpro")
				EXTENIS_OPEN_FILE(".rseq")
				EXTENIS_OPEN_FILE(".sa")
				EXTENIS_OPEN_FILE(".ext")
				wrapStr = mainPool ? new ChunkySeqGraphReader(saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9],numThread,mainPool) : new ChunkySeqGraphReader(saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9]);
				delete(compMeth);
				return;
			}
		}
		//try to treat it as text
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Sequence type not supported for random access.", 0, 0);
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}

#define TABLE_PREFER_CHUNK_SIZE 0x010000

ExtensionSeqGraphWriter::ExtensionSeqGraphWriter(SeqGraphHeader* theHead, const char* fileName, OutStream* useStdout){
	openUp(theHead, fileName, 1, 0, useStdout);
}
ExtensionSeqGraphWriter::ExtensionSeqGraphWriter(SeqGraphHeader* theHead, const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	openUp(theHead, fileName, numThread, mainPool, useStdout);
}
ExtensionSeqGraphWriter::~ExtensionSeqGraphWriter(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionSeqGraphWriter::write(SeqGraphDataSet* toStore){
	wrapStr->write(toStore);
}
void ExtensionSeqGraphWriter::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionSeqGraphWriter::openUp(SeqGraphHeader* theHead, const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Raw text sequence graph not yet supported.", 0, 0);
		}
		//anything block compressed
		{
			int isBCseq = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bsgrap"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bsgrap"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bsgrap"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bsgrap"));
			if(isBCseq){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indexFN(fileName);
				std::string curFN;
				std::string curBN;
				std::vector<OutStream*> saveStrs;
				OutStream* curStr;
				#define EXTENOS_OPEN_FILE(nameSuff) \
					curFN.clear(); curFN.append(fileName); curFN.append(nameSuff);\
					curBN.clear(); curBN.append(curFN); curBN.append(".blk");\
					curStr = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, curFN.c_str(), curBN.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, curFN.c_str(), curBN.c_str(), compMeth);\
					baseStrs.push_back(curStr); saveStrs.push_back(curStr);
				EXTENOS_OPEN_FILE("")
				EXTENOS_OPEN_FILE(".name")
				EXTENOS_OPEN_FILE(".csize")
				EXTENOS_OPEN_FILE(".ilink")
				EXTENOS_OPEN_FILE(".olink")
				EXTENOS_OPEN_FILE(".cpro")
				EXTENOS_OPEN_FILE(".lpro")
				EXTENOS_OPEN_FILE(".rseq")
				EXTENOS_OPEN_FILE(".sa")
				EXTENOS_OPEN_FILE(".ext")
				wrapStr = mainPool ? new ChunkySeqGraphWriter(theHead,saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9],numThread,mainPool) : new ChunkySeqGraphWriter(theHead,saveStrs[0],saveStrs[1],saveStrs[2],saveStrs[3],saveStrs[4],saveStrs[5],saveStrs[6],saveStrs[7],saveStrs[8],saveStrs[9]);
				delete(compMeth);
				return;
			}
		}
		//try to treat it as text
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Raw text sequence graph not yet supported.", 0, 0);
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}

ArgumentOptionSeqGraphRead::ArgumentOptionSeqGraphRead(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" sgraph.zlib.bsgrap");
	summary = useDesc;
	validExts.push_back(".raw.bsgrap");
	//validExts.push_back(".gzip.bsgrap");
	validExts.push_back(".zlib.bsgrap");
}
ArgumentOptionSeqGraphRead::~ArgumentOptionSeqGraphRead(){}

ArgumentOptionSeqGraphRandac::ArgumentOptionSeqGraphRandac(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" sgraph.zlib.bsgrap");
	summary = useDesc;
	validExts.push_back(".raw.bsgrap");
	//validExts.push_back(".gzip.bsgrap");
	validExts.push_back(".zlib.bsgrap");
}
ArgumentOptionSeqGraphRandac::~ArgumentOptionSeqGraphRandac(){}

ArgumentOptionSeqGraphWrite::ArgumentOptionSeqGraphWrite(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileWrite(theName){
	required = needed;
	usage = theName;
		usage.append(" sgraph.zlib.bsgrap");
	summary = useDesc;
	validExts.push_back(".raw.bsgrap");
	//validExts.push_back(".gzip.bsgrap");
	validExts.push_back(".zlib.bsgrap");
}
ArgumentOptionSeqGraphWrite::~ArgumentOptionSeqGraphWrite(){}




