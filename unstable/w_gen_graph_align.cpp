#include "whodun_gen_graph_align.h"

//read index, reference index, probability, contig ref, base index ref, contig read, base index read, entry offset
#define WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE 64

namespace whodun{

/**Bulk unpack the index data.*/
class ChunkySeqGraphAlignmentIndexConvertLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphAlignmentIndexConvertLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphAlignmentIndexConvertLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**The index data.*/
	char* annotData;
	/**The place to put the loaded data.*/
	JointAlignmentSet* toStore;
	/**The offset of the first entry*/
	uintmax_t baseEntO;
};

/**Bulk unpack the entry data.*/
class ChunkySeqGraphAlignmentEntryConvertLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphAlignmentEntryConvertLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphAlignmentEntryConvertLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**The index data.*/
	char* alignData;
	/**The place to store.*/
	JointAlignmentEntry* toStore;
};

/**Bulk pack the index data.*/
class ChunkySeqGraphAlignmentIndexPackLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphAlignmentIndexPackLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphAlignmentIndexPackLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**The index data.*/
	char* annotData;
	/**The place to put the loaded data.*/
	JointAlignmentSet* toStore;
	/**The offset of the first entry*/
	uintmax_t baseEntO;
};

/**Bulk pack the entry data.*/
class ChunkySeqGraphAlignmentEntryPackLoop : public ParallelForLoop{
public:
	/**
	 * Set up a loop.
	 * @param numThread The number of threads to do.
	 */
	ChunkySeqGraphAlignmentEntryPackLoop(uintptr_t numThread);
	/**Clean up.*/
	~ChunkySeqGraphAlignmentEntryPackLoop();
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**The index data.*/
	char* alignData;
	/**The place to store.*/
	JointAlignmentEntry* toStore;
};

};

using namespace whodun;

JointAlignmentSet::JointAlignmentSet(){}
JointAlignmentSet::~JointAlignmentSet(){}

SeqGraphAlignmentReader::SeqGraphAlignmentReader(){
	isClosed = 0;
}
SeqGraphAlignmentReader::~SeqGraphAlignmentReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

void RandacSeqGraphAlignmentReader::readRange(uintmax_t fromIndex, uintmax_t toIndex, JointAlignmentSet* toStore){
	seek(fromIndex);
	read(toIndex - fromIndex, toStore);
}

SeqGraphAlignmentWriter::SeqGraphAlignmentWriter(){
	isClosed = 0;
}
SeqGraphAlignmentWriter::~SeqGraphAlignmentWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

#define WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE 5

ChunkySeqGraphAlignmentReader::ChunkySeqGraphAlignmentReader(RandaccInStream* fileInd, RandaccInStream* fileEntries){
	needSeek = 1;
	numGraphs = fileInd->size();
	if(numGraphs % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed index for sequence graph alignment data.", 0, 0); }
	numGraphs = numGraphs / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE;
	nextGraph = 0;
	doIConv = new ChunkySeqGraphAlignmentIndexConvertLoop(1);
	doEConv = new ChunkySeqGraphAlignmentEntryConvertLoop(1);
	usePool = 0;
	fInd = fileInd;
	fEnt = fileEntries;
}
ChunkySeqGraphAlignmentReader::ChunkySeqGraphAlignmentReader(RandaccInStream* fileInd, RandaccInStream* fileEntries, uintptr_t numThread, ThreadPool* mainPool){
	needSeek = 1;
	numGraphs = fileInd->size();
	if(numGraphs % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed index for sequence graph alignment data.", 0, 0); }
	numGraphs = numGraphs / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE;
	nextGraph = 0;
	doIConv = new ChunkySeqGraphAlignmentIndexConvertLoop(numThread);
	doEConv = new ChunkySeqGraphAlignmentEntryConvertLoop(numThread);
	usePool = mainPool;
	fInd = fileInd;
	fEnt = fileEntries;
}
ChunkySeqGraphAlignmentReader::~ChunkySeqGraphAlignmentReader(){
	delete(doIConv);
	delete(doEConv);
}
uintptr_t ChunkySeqGraphAlignmentReader::read(uintptr_t numAlign, JointAlignmentSet* toStore){
	if(numAlign == 0){ return 0; }
	//seek if necessary
		if(needSeek){
			if(nextGraph < numGraphs){
				fInd->seek(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*nextGraph);
				saveInd.resize(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE);
				fInd->forceRead(saveInd[0], WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE);
				ByteUnpacker getOffV(saveInd[0] + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE - 8);
				fEnt->seek(getOffV.unpackBE64());
			}
			needSeek = 0;
		}
	//figure out how much to read
		uintmax_t endFocInd = nextGraph + numAlign;
			endFocInd = std::min(endFocInd, numGraphs);
		uintptr_t numRealRead = endFocInd - nextGraph;
		int willHitEOF = endFocInd == numGraphs;
		if(numRealRead == 0){ return 0; }
	//load the annotation
		saveInd.resize(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*(numRealRead + 1));
		fInd->forceRead(saveInd[WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE], WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*(numRealRead - willHitEOF));
		if(willHitEOF){
			BytePacker packOffV(saveInd[WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*numRealRead] + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE - 8);
			packOffV.packBE64(fEnt->size());
		}
	//load the entries
	uintmax_t startOff;
	{
		ByteUnpacker getStartO(saveInd[0] + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE-8);
		ByteUnpacker getEndO(saveInd[WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*numRealRead] + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE-8);
		startOff = getStartO.unpackBE64();
		uintmax_t endOff = getStartO.unpackBE64();
		if((startOff < endOff) || (startOff % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE) || (endOff % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Bad entry offsets.", 0, 0);
		}
		uintptr_t numByteLoad = endOff - startOff;
		tmpText.clear(); tmpText.resize(numByteLoad);
		fEnt->forceRead(tmpText[0], numByteLoad);
		toStore->entries.clear(); toStore->entries.resize(numByteLoad / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE);
		ChunkySeqGraphAlignmentEntryConvertLoop* intConv = (ChunkySeqGraphAlignmentEntryConvertLoop*)doEConv;
		intConv->alignData = tmpText[0];
		intConv->toStore = toStore->entries[0];
		if(usePool){ intConv->doIt(usePool, 0, numByteLoad / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE); }
		else{ intConv->doIt(0, numByteLoad / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE); }
	}
	//make space
		toStore->alignments.clear(); toStore->alignments.resize(numRealRead);
	//prepare to pack
		ChunkySeqGraphAlignmentIndexConvertLoop* finalFill = (ChunkySeqGraphAlignmentIndexConvertLoop*)doIConv;
		finalFill->annotData = saveInd[0];
		finalFill->toStore = toStore;
		finalFill->baseEntO = startOff;
		if(usePool){ finalFill->doIt(usePool, 0, numRealRead); }
		else{ finalFill->doIt(0, numRealRead); }
	//and get ready for the next round
		memcpy(saveInd[0], saveInd[WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*numRealRead], WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE);
		nextGraph = endFocInd;
	return numRealRead;
}
void ChunkySeqGraphAlignmentReader::close(){
	isClosed = 1;
}
uintmax_t ChunkySeqGraphAlignmentReader::size(){
	return numGraphs;
}
void ChunkySeqGraphAlignmentReader::seek(uintmax_t index){
	if(index == nextGraph){ return; }
	needSeek = 1;
	nextGraph = index;
}

ChunkySeqGraphAlignmentIndexConvertLoop::ChunkySeqGraphAlignmentIndexConvertLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 1024;
}
ChunkySeqGraphAlignmentIndexConvertLoop::~ChunkySeqGraphAlignmentIndexConvertLoop(){}
void ChunkySeqGraphAlignmentIndexConvertLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	//unpack the annotation stuff
		JointAlignment* curFill = toStore->alignments[ind];
		ByteUnpacker curUP(annotData + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*ind);
		curFill->readIndex = curUP.unpackBE64();
		curFill->refIndex = curUP.unpackBE64();
		curFill->probability = curUP.unpackBEDbl();
		curFill->startContigRef = curUP.unpackBE64();
		curFill->startIndexRef = curUP.unpackBE64();
		curFill->startContigRead = curUP.unpackBE64();
		curFill->startIndexRead = curUP.unpackBE64();
		uintmax_t curEntO = curUP.unpackBE64();
		curUP.skip(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE - 8);
		uintmax_t nextEntO = curUP.unpackBE64();
	//idiot check
		if((curEntO % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE) || (nextEntO % WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Entry offsets not aligned.", 0, 0);
		}
		if((nextEntO < curEntO) || (curEntO < baseEntO) || ((nextEntO - baseEntO) > (WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE * toStore->entries.size()))){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Entry offsets not monotonic.", 0, 0);
		}
		curFill->numEntries = (nextEntO - curEntO) / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE;
		curFill->entryOffset = (curEntO - baseEntO) / WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE;
}

ChunkySeqGraphAlignmentEntryConvertLoop::ChunkySeqGraphAlignmentEntryConvertLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 1024;
}
ChunkySeqGraphAlignmentEntryConvertLoop::~ChunkySeqGraphAlignmentEntryConvertLoop(){}
void ChunkySeqGraphAlignmentEntryConvertLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	char* curLook = alignData + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE*ind;
	JointAlignmentEntry* curFill = toStore + ind;
	curFill->type = 0x00FF & *curLook;
	ByteUnpacker curUP(curLook + 1);
	switch(curFill->type){
		case WHODUN_JOINT_TYPE_MATCH:
			curFill->matchLength = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_INSERT_REF:
			curFill->insertRefLength = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_INSERT_READ:
			curFill->insertReadLength = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_DELETE_REF:
			curFill->deleteRefLength = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_DELETE_READ:
			curFill->deleteReadLength = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_LINK_REF:
			curFill->linkRefIndex = curUP.unpackBE32();
			break;
		case WHODUN_JOINT_TYPE_LINK_READ:
			curFill->linkReadIndex = curUP.unpackBE32();
			break;
		default:
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Unknown action in alignment.", 0, 0);
	};
}

ChunkySeqGraphAlignmentWriter::ChunkySeqGraphAlignmentWriter(OutStream* fileInd, OutStream* fileEntries){
	fInd = fileInd;
	fEnt = fileEntries;
	numEnt = 0;
	doIConv = new ChunkySeqGraphAlignmentIndexConvertLoop(1);
	doEConv = new ChunkySeqGraphAlignmentEntryConvertLoop(1);
	usePool = 0;
}
ChunkySeqGraphAlignmentWriter::ChunkySeqGraphAlignmentWriter(OutStream* fileInd, OutStream* fileEntries, uintptr_t numThread, ThreadPool* mainPool){
	fInd = fileInd;
	fEnt = fileEntries;
	numEnt = 0;
	doIConv = new ChunkySeqGraphAlignmentIndexConvertLoop(numThread);
	doEConv = new ChunkySeqGraphAlignmentEntryConvertLoop(numThread);
	usePool = mainPool;
}
ChunkySeqGraphAlignmentWriter::~ChunkySeqGraphAlignmentWriter(){
	delete(doIConv);
	delete(doEConv);
}
void ChunkySeqGraphAlignmentWriter::write(JointAlignmentSet* toStore){
	//index data
		saveInd.clear(); saveInd.resize(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE * toStore->alignments.size());
		ChunkySeqGraphAlignmentIndexPackLoop* packInd = (ChunkySeqGraphAlignmentIndexPackLoop*)doIConv;
		packInd->annotData = saveInd[0];
		packInd->toStore = toStore;
		packInd->baseEntO = numEnt;
		if(usePool){ packInd->doIt(usePool, 0, toStore->alignments.size()); }
		else{ packInd->doIt(0, toStore->alignments.size()); }
		fInd->write(saveInd[0], saveInd.size());
	//entry data
		tmpText.clear(); tmpText.resize(WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE * toStore->entries.size());
		ChunkySeqGraphAlignmentEntryPackLoop* packEnt = (ChunkySeqGraphAlignmentEntryPackLoop*)doEConv;
		packEnt->alignData = tmpText[0];
		packEnt->toStore = toStore->entries[0];
		if(usePool){ packEnt->doIt(usePool, 0, toStore->entries.size()); }
		else{ packEnt->doIt(0, toStore->entries.size()); }
		fEnt->write(tmpText[0], tmpText.size());
}
void ChunkySeqGraphAlignmentWriter::close(){
	isClosed = 1;
}

ChunkySeqGraphAlignmentIndexPackLoop::ChunkySeqGraphAlignmentIndexPackLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 1024;
}
ChunkySeqGraphAlignmentIndexPackLoop::~ChunkySeqGraphAlignmentIndexPackLoop(){}
void ChunkySeqGraphAlignmentIndexPackLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	//unpack the annotation stuff
		JointAlignment* curFill = toStore->alignments[ind];
		BytePacker curUP(annotData + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_INDEX_SIZE*ind);
		curUP.packBE64(curFill->readIndex);
		curUP.packBE64(curFill->refIndex);
		curUP.packBEDbl(curFill->probability);
		curUP.packBE64(curFill->startContigRef);
		curUP.packBE64(curFill->startIndexRef);
		curUP.packBE64(curFill->startContigRead);
		curUP.packBE64(curFill->startIndexRead);
		uintmax_t curEntO = WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE * curFill->entryOffset + baseEntO;
		curUP.packBE64(curEntO);
}

ChunkySeqGraphAlignmentEntryPackLoop::ChunkySeqGraphAlignmentEntryPackLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 1024;
}
ChunkySeqGraphAlignmentEntryPackLoop::~ChunkySeqGraphAlignmentEntryPackLoop(){}
void ChunkySeqGraphAlignmentEntryPackLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	char* curLook = alignData + WHODUN_CHUNKY_SEQ_GRAPH_ALIGN_ENTRY_SIZE*ind;
	JointAlignmentEntry* curFill = toStore + ind;
	*curLook = curFill->type;
	BytePacker curUP(curLook + 1);
	switch(curFill->type){
		case WHODUN_JOINT_TYPE_MATCH:
			curUP.packBE32(curFill->matchLength);
			break;
		case WHODUN_JOINT_TYPE_INSERT_REF:
			curUP.packBE32(curFill->insertRefLength);
			break;
		case WHODUN_JOINT_TYPE_INSERT_READ:
			curUP.packBE32(curFill->insertReadLength);
			break;
		case WHODUN_JOINT_TYPE_DELETE_REF:
			curUP.packBE32(curFill->deleteRefLength);
			break;
		case WHODUN_JOINT_TYPE_DELETE_READ:
			curUP.packBE32(curFill->deleteReadLength);
			break;
		case WHODUN_JOINT_TYPE_LINK_REF:
			curUP.packBE32(curFill->linkRefIndex);
			break;
		case WHODUN_JOINT_TYPE_LINK_READ:
			curUP.packBE32(curFill->linkReadIndex);
			break;
		default:
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Unknown action in alignment.", 0, 0);
	};
}




