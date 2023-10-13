#include "whodun_gen_seqdata.h"

#include "whodun_compress.h"

namespace whodun {

/**Parse a fasta file.*/
class FastaReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	FastaReadTask();
	/**Clean up.*/
	~FastaReadTask();
	void doTask();
	
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - find lines that begin with >
	/**The first token to look at.*/
	uintptr_t tokenSI;
	/**The token to stop looking at.*/
	uintptr_t tokenEI;
	/**The full set of tokens to look at.*/
	Token* theToken;
	/**The indices of the tokens that begin with >*/
	std::vector<uintptr_t> nameTokenIs;
	
	//phase 2 - collect the tokens into one list
	/**The place to put token indices.*/
	uintptr_t* fillTokenTgt;
	
	//phase 3 - pack the pieces together
	/**The first name this works on.*/
	uintptr_t nameTSI;
	/**The last name this works on.*/
	uintptr_t nameTEI;
	/**The name token indices.*/
	uintptr_t* allNameTIs;
	/**The place to fix up.*/
	SequenceSet* toStore;
};

/**Pack a fasta file for output.*/
class FastaWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	FastaWriteTask();
	/**Clean up.*/
	~FastaWriteTask();
	void doTask();
	
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - figure total length needed
	/**The place to fix up.*/
	SequenceSet* toStore;
	/**The sequence index to start at.*/
	uintptr_t seqSI;
	/**The sequence index to end at.*/
	uintptr_t seqEI;
	/**The space this chunk of sequences needs.*/
	uintptr_t totalNeededS;
	
	//phase 2 - pack it
	/**The place this should pack to.*/
	char* packLoc;
};

/**Link things up after a chunky read.*/
class ChunkySequenceReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	ChunkySequenceReadTask();
	/**Clean up.*/
	~ChunkySequenceReadTask();
	void doTask();
	
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	/**The full set of name annotation data.*/
	char* annotNData;
	/**The full set of packed name data.*/
	SizePtrString nameData;
	/**The full set of sequence annotation data.*/
	char* annotSData;
	/**The full set of packed sequence data.*/
	SizePtrString seqData;
	/**The thing to fix up.*/
	SequenceSet* toStore;
};

/**Pack things up for a chunky write.*/
class ChunkySequenceWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	ChunkySequenceWriteTask();
	/**Clean up.*/
	~ChunkySequenceWriteTask();
	void doTask();
	
	/**The phase to run*/
	uintptr_t phase;
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	/**The table this works on.*/
	SequenceSet* toStore;
	
	//phase 1 - figure out how many bytes this eats
	/**The number of bytes this chunk will use for names.*/
	uintptr_t numEatBytesN;
	/**The number of bytes this chunk will use for sequence.*/
	uintptr_t numEatBytesS;
	
	//phase 2 - pack up names
	/**The file offset this chunk of names starts at.*/
	uintmax_t startByteIN;
	/**Storage for annotation data for this piece.*/
	char* annotData;
	/**The table data to pack this piece into.*/
	char* tableData;
	
	//phase 3 - pack up sequence
	/**The file offset this chunk of sequence starts at.*/
	uintmax_t startByteIS;
};

};

using namespace whodun;

SequenceSet::SequenceSet(){}
SequenceSet::~SequenceSet(){}

SequenceReader::SequenceReader(){
	isClosed = 0;
}
SequenceReader::~SequenceReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

void RandacSequenceReader::readRange(SequenceSet* toStore, uintmax_t fromIndex, uintmax_t toIndex){
	uintmax_t numR = toIndex - fromIndex;
	seek(fromIndex);
	read(toStore, numR);
}

SequenceWriter::SequenceWriter(){
	isClosed = 0;
}
SequenceWriter::~SequenceWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

FastaSequenceReader::FastaSequenceReader(InStream* mainFrom){
	theStr = mainFrom;
	rowSplitter = new CharacterSplitTokenizer('\n');
	charMove = new StandardMemoryShuttler();
	usePool = 0;
	{
		passUnis.push_back(new FastaReadTask());
	}
}
FastaSequenceReader::FastaSequenceReader(InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	rowSplitter = new CharacterSplitTokenizer('\n');
	charMove = new StandardMemoryShuttler();
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new FastaReadTask());
	}
}
FastaSequenceReader::~FastaSequenceReader(){
	delete(rowSplitter);
	delete(charMove);
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t FastaSequenceReader::read(SequenceSet* toStore, uintptr_t numSeqs){
	if(numSeqs == 0){
		toStore->saveNames.clear();
		toStore->saveStrs.clear();
		return 0;
	}
	uintptr_t typeDelim = 0;
	uintptr_t typeText = 1;
	//initialize the text
		toStore->saveText.clear();
		if(saveTexts.size()){
			toStore->saveText.resize(saveTexts.size());
			charMove->memcpy(toStore->saveText[0], saveTexts[0], saveTexts.size());
			saveTexts.clear();
		}
	//find all lines that begin with >: read more until enough had
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT;
		uintptr_t numET;
		uintptr_t totNumSeqs;
		SizePtrString remText;
		int haveHitEOF = 0;
		while(1){
			//find the tokens
				saveRowS.clear();
				remText = rowSplitter->tokenize(toSizePtr(&(toStore->saveText)), &saveRowS);
			//if have hit eof, add some tokens
				if(haveHitEOF){
					Token pushT;
					pushT.text = remText;
						pushT.numTypes = 1;
						pushT.types = &typeText;
						saveRowS.push_back(&pushT);
					pushT.text.len = 0;
						pushT.types = &typeDelim;
						saveRowS.push_back(&pushT);
				}
			//and hunt for lines that begin with '>'
				numPT = saveRowS.size() / numThread;
				numET = saveRowS.size() % numThread;
				uintptr_t curTI = 0;
				for(uintptr_t i = 0; i<numThread; i++){
					FastaReadTask* curT = (FastaReadTask*)(passUnis[i]);
					curT->phase = 1;
					curT->tokenSI = curTI;
					curTI += (numPT + (i<numET));
					curT->tokenEI = curTI;
					curT->theToken = saveRowS[0];
				}
				if(usePool){
					usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
					joinTasks(numThread, &(passUnis[0]));
				}
				else{
					passUnis[0]->doTask();
				}
			//figure out how many there are
				totNumSeqs = 0;
				for(uintptr_t i = 0; i<numThread; i++){
					FastaReadTask* curT = (FastaReadTask*)(passUnis[i]);
					totNumSeqs += curT->nameTokenIs.size();
				}
			//figure out if more neads to be read
				if(haveHitEOF || (totNumSeqs > numSeqs)){
					break;
				}
			//load more and try again
				uintptr_t origTS = toStore->saveText.size();
				uintptr_t newTS = std::max((uintptr_t)80, 2*origTS);
				toStore->saveText.resize(newTS);
				uintptr_t numWantR = newTS - origTS;
				uintptr_t numGotR = theStr->read(toStore->saveText[origTS], numWantR);
				if(numGotR != numWantR){
					toStore->saveText.resize(origTS + numGotR);
					haveHitEOF = 1;
				}
		}
	//pack up the token indices
		saveSeqHS.clear();
		saveSeqHS.resize(totNumSeqs + 1);
		uintptr_t* curSeqFTgt = saveSeqHS[0];
		for(uintptr_t i = 0; i<numThread; i++){
			FastaReadTask* curT = (FastaReadTask*)(passUnis[i]);
			curT->phase = 2;
			curT->fillTokenTgt = curSeqFTgt;
			curSeqFTgt += curT->nameTokenIs.size();
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
		*curSeqFTgt = saveRowS.size();
	//pack down the pieces and let the threads build
		if(!haveHitEOF){ totNumSeqs--; }
		numPT = totNumSeqs / numThread;
		numET = totNumSeqs % numThread;
		uintptr_t curSTI = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			FastaReadTask* curT = (FastaReadTask*)(passUnis[i]);
			curT->phase = 3;
			curT->nameTSI = curSTI;
			curSTI += (numPT + (i<numET));
			curT->nameTEI = curSTI;
			curT->allNameTIs = saveSeqHS[0];
			curT->toStore = toStore;
		}
		toStore->saveNames.clear();
		toStore->saveNames.resize(totNumSeqs);
		toStore->saveStrs.clear();
		toStore->saveStrs.resize(totNumSeqs);
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//store any overhang for later
		uintptr_t overStartTokI = *(saveSeqHS[totNumSeqs]);
		if(overStartTokI < saveRowS.size()){
			char* curOverTS = saveRowS[overStartTokI]->text.txt;
			char* curOverTE = toStore->saveText[0] + toStore->saveText.size();
			saveTexts.resize(curOverTE - curOverTS);
			charMove->memcpy(saveTexts[0], curOverTS, saveTexts.size());
		}
	return totNumSeqs;
}
void FastaSequenceReader::close(){
	isClosed = 1;
}

FastaReadTask::FastaReadTask(){}
FastaReadTask::~FastaReadTask(){}
void FastaReadTask::doTask(){
	if(phase == 1){
		nameTokenIs.clear();
		for(uintptr_t i = tokenSI; i<tokenEI; i++){
			Token* curLTok = theToken + i;
			if(curLTok->types[0] == 0){ continue; }
			if(curLTok->text.len == 0){ continue; }
			if(curLTok->text.txt[0] != '>'){ continue; }
			nameTokenIs.push_back(i);
		}
	}
	else if(phase == 2){
		if(nameTokenIs.size()){
			memcpy(fillTokenTgt, &(nameTokenIs[0]), nameTokenIs.size()*sizeof(uintptr_t));
		}
	}
	else{
		StandardMemorySearcher doTrim;
		for(uintptr_t si = nameTSI; si<nameTEI; si++){
			uintptr_t nameTokSI = allNameTIs[si];
			uintptr_t seqTokEI = allNameTIs[si+1];
			Token* nameTok = theToken + nameTokSI;
				SizePtrString winName = nameTok->text;
				winName.txt++;
				winName.len--;
				winName = doTrim.trim(winName);
				*(toStore->saveNames[si]) = winName;
			char* curFillTxt = nameTok[1].text.txt;
			char* curSeqTS = curFillTxt;
				for(uintptr_t ti = nameTokSI + 1; ti<seqTokEI; ti++){
					Token* curSTok = theToken + ti;
					if(curSTok->types[0] == 0){ continue; }
					SizePtrString curTextC = curSTok->text;
					for(uintptr_t i = 0; i<curTextC.len; i++){
						char curTC = curTextC.txt[i];
						if((curTC != ' ') && (curTC != '\r') && (curTC != '\t')){
							*curFillTxt = curTC;
							curFillTxt++;
						}
					}
				}
				SizePtrString winSeq;
				winSeq.txt = curSeqTS;
				winSeq.len = curFillTxt - curSeqTS;
				*(toStore->saveStrs[si]) = winSeq;
		}
	}
}

FastaSequenceWriter::FastaSequenceWriter(OutStream* mainFrom){
	theStr = mainFrom;
	usePool = 0;
	{
		passUnis.push_back(new FastaWriteTask());
	}
}
FastaSequenceWriter::FastaSequenceWriter(OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new FastaWriteTask());
	}
}
FastaSequenceWriter::~FastaSequenceWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void FastaSequenceWriter::write(SequenceSet* toStore){
	//figure out the needed size
		uintptr_t numSeqs = toStore->saveNames.size();
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numSeqs / numThread;
		uintptr_t numET = numSeqs % numThread;
		uintptr_t curSI = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			FastaWriteTask* curT = (FastaWriteTask*)(passUnis[i]);
			curT->phase = 1;
			curT->toStore = toStore;
			curT->seqSI = curSI;
			curSI += (numPT + (i<numET));
			curT->seqEI = curSI;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//pack em up
		uintptr_t totalNeedS = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			FastaWriteTask* curT = (FastaWriteTask*)(passUnis[i]);
			totalNeedS += curT->totalNeededS;
		}
		saveTexts.clear();
		saveTexts.resize(totalNeedS);
		char* curTgtT = saveTexts[0];
		for(uintptr_t i = 0; i<numThread; i++){
			FastaWriteTask* curT = (FastaWriteTask*)(passUnis[i]);
			curT->phase = 2;
			curT->packLoc = curTgtT;
			curTgtT += curT->totalNeededS;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//dump em out
		theStr->write(saveTexts[0], totalNeedS);
}
void FastaSequenceWriter::close(){
	isClosed = 1;
}

FastaWriteTask::FastaWriteTask(){}
FastaWriteTask::~FastaWriteTask(){}
void FastaWriteTask::doTask(){
	if(phase == 1){
		SizePtrString* curName = toStore->saveNames[seqSI];
		SizePtrString* curSeq = toStore->saveStrs[seqSI];
		totalNeededS = 0;
		for(uintptr_t i = seqSI; i<seqEI; i++){
			totalNeededS += 3;
			totalNeededS += curName->len;
			totalNeededS += curSeq->len;
			curName++;
			curSeq++;
		}
	}
	else{
		SizePtrString* curName = toStore->saveNames[seqSI];
		SizePtrString* curSeq = toStore->saveStrs[seqSI];
		char* curFill = packLoc;
		for(uintptr_t i = seqSI; i<seqEI; i++){
			*curFill = '>'; curFill++;
			memcpy(curFill, curName->txt, curName->len); curFill += curName->len;
			*curFill = '\n'; curFill++;
			memcpy(curFill, curSeq->txt, curSeq->len); curFill += curSeq->len;
			*curFill = '\n'; curFill++;
			curName++;
			curSeq++;
		}
	}
}

#define BLOCKCOMP_ANNOT_ENTLEN 8

ChunkySequenceReader::ChunkySequenceReader(RandaccInStream* nameAnnotationFile, RandaccInStream* nameFile, RandaccInStream* sequenceAnnotationFile, RandaccInStream* sequenceFile){
	nAtt = nameAnnotationFile;
	nDat = nameFile;
	sAtt = sequenceAnnotationFile;
	sDat = sequenceFile;
	needSeek = 1;
	focusInd = 0;
	totalNInd = nAtt->size();
		if(totalNInd % BLOCKCOMP_ANNOT_ENTLEN){
			isClosed = 1;
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not even.", 0, 0);
		}
		if(sAtt->size() != totalNInd){
			isClosed = 1;
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index files do not match.", 0, 0);
		}
		totalNInd = totalNInd / BLOCKCOMP_ANNOT_ENTLEN;
	totalNByte = nDat->size();
	totalSByte = sDat->size();
	usePool = 0;
	{
		passUnis.push_back(new ChunkySequenceReadTask());
	}
}
ChunkySequenceReader::ChunkySequenceReader(RandaccInStream* nameAnnotationFile, RandaccInStream* nameFile, RandaccInStream* sequenceAnnotationFile, RandaccInStream* sequenceFile, uintptr_t numThread, ThreadPool* mainPool){
	nAtt = nameAnnotationFile;
	nDat = nameFile;
	sAtt = sequenceAnnotationFile;
	sDat = sequenceFile;
	needSeek = 1;
	focusInd = 0;
	totalNInd = nAtt->size();
		if(totalNInd % BLOCKCOMP_ANNOT_ENTLEN){
			isClosed = 1;
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not even.", 0, 0);
		}
		if(sAtt->size() != totalNInd){
			isClosed = 1;
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index files do not match.", 0, 0);
		}
		totalNInd = totalNInd / BLOCKCOMP_ANNOT_ENTLEN;
	totalNByte = nDat->size();
	totalSByte = sDat->size();
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new ChunkySequenceReadTask());
	}
}
ChunkySequenceReader::~ChunkySequenceReader(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t ChunkySequenceReader::read(SequenceSet* toStore, uintptr_t numSeqs){
	if(numSeqs == 0){ return 0; }
	//seek if necessary
		if(needSeek){
			if(focusInd < totalNInd){
				{
					nAtt->seek(BLOCKCOMP_ANNOT_ENTLEN*focusInd);
					saveNARB.resize(BLOCKCOMP_ANNOT_ENTLEN);
					nAtt->forceRead(saveNARB[0], BLOCKCOMP_ANNOT_ENTLEN);
					ByteUnpacker getOffV(saveNARB[0]);
					nDat->seek(getOffV.unpackBE64());
				}
				{
					sAtt->seek(BLOCKCOMP_ANNOT_ENTLEN*focusInd);
					saveSARB.resize(BLOCKCOMP_ANNOT_ENTLEN);
					sAtt->forceRead(saveSARB[0], BLOCKCOMP_ANNOT_ENTLEN);
					ByteUnpacker getOffV(saveSARB[0]);
					sDat->seek(getOffV.unpackBE64());
				}
			}
			needSeek = 0;
		}
	//figure out how many rows will actually be read
		uintmax_t endFocInd = focusInd + numSeqs;
			endFocInd = std::min(endFocInd, totalNInd);
		uintptr_t numRealRead = endFocInd - focusInd;
		int willHitEOF = endFocInd == totalNInd;
		if(numRealRead == 0){ return 0; }
	//load the annotation
		saveNARB.resize(BLOCKCOMP_ANNOT_ENTLEN*(numRealRead + 1));
		nAtt->forceRead(saveNARB[BLOCKCOMP_ANNOT_ENTLEN], BLOCKCOMP_ANNOT_ENTLEN*(numRealRead - willHitEOF));
		saveSARB.resize(BLOCKCOMP_ANNOT_ENTLEN*(numRealRead + 1));
		sAtt->forceRead(saveSARB[BLOCKCOMP_ANNOT_ENTLEN], BLOCKCOMP_ANNOT_ENTLEN*(numRealRead - willHitEOF));
		if(willHitEOF){
			BytePacker packOffV(saveNARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead]);
			packOffV.packBE64(totalNByte);
			packOffV.retarget(saveSARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead]);
			packOffV.packBE64(totalSByte);
		}
	//load the text
		uintmax_t startTAddrN;
		uintmax_t endTAddrN;
		uintmax_t startTAddrS;
		uintmax_t endTAddrS;
		{
			ByteUnpacker getOffV(saveNARB[0]);
			startTAddrN = getOffV.unpackBE64();
			getOffV.retarget(saveNARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead]);
			endTAddrN = getOffV.unpackBE64();
		}
		{
			ByteUnpacker getOffV(saveSARB[0]);
			startTAddrS = getOffV.unpackBE64();
			getOffV.retarget(saveSARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead]);
			endTAddrS = getOffV.unpackBE64();
		}
		uintmax_t loadedNSize = (endTAddrN - startTAddrN);
		uintmax_t loadedSSize = (endTAddrS - startTAddrS);
		toStore->saveText.clear();
		toStore->saveText.resize(loadedNSize + loadedSSize);
		char* loadedNDat = toStore->saveText[0];
		char* loadedSDat = toStore->saveText[loadedNSize];
		nDat->forceRead(loadedNDat, loadedNSize);
		sDat->forceRead(loadedSDat, loadedSSize);
	//chop up the text
		toStore->saveNames.clear();
		toStore->saveStrs.clear();
		toStore->saveNames.resize(numRealRead);
		toStore->saveStrs.resize(numRealRead);
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numRealRead / numThread;
		uintptr_t numET = numRealRead % numThread;
		uintptr_t curSeqI = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkySequenceReadTask* curT = (ChunkySequenceReadTask*)(passUnis[i]);
			curT->fromRI = curSeqI;
			curSeqI += (numPT + (i<numET));
			curT->toRI = curSeqI;
			curT->annotNData = saveNARB[0];
			curT->annotSData = saveSARB[0];
			curT->nameData.len = loadedNSize;
			curT->nameData.txt = loadedNDat;
			curT->seqData.len = loadedSSize;
			curT->seqData.txt = loadedSDat;
			curT->toStore = toStore;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//and prepare for the next round
		memcpy(saveNARB[0], saveNARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead], BLOCKCOMP_ANNOT_ENTLEN);
		memcpy(saveSARB[0], saveSARB[BLOCKCOMP_ANNOT_ENTLEN*numRealRead], BLOCKCOMP_ANNOT_ENTLEN);
		focusInd = endFocInd;
	return numRealRead;
}
void ChunkySequenceReader::close(){
	isClosed = 1;
}
uintmax_t ChunkySequenceReader::size(){
	return totalNInd;
}
void ChunkySequenceReader::seek(uintmax_t index){
	if(index != focusInd){
		focusInd = index;
		needSeek = 1;
	}
}

ChunkySequenceReadTask::ChunkySequenceReadTask(){}
ChunkySequenceReadTask::~ChunkySequenceReadTask(){}
void ChunkySequenceReadTask::doTask(){
	if(fromRI == toRI){ return; }
	SizePtrString* fillName = toStore->saveNames[fromRI];
	SizePtrString* fillSeq = toStore->saveStrs[fromRI];
	ByteUnpacker getOffVN(annotNData);
	ByteUnpacker getOffVS(annotSData);
	uintmax_t baseAddrN = getOffVN.unpackBE64();
	uintmax_t baseAddrS = getOffVS.unpackBE64();
	getOffVN.retarget(annotNData + BLOCKCOMP_ANNOT_ENTLEN*fromRI);
	getOffVS.retarget(annotSData + BLOCKCOMP_ANNOT_ENTLEN*fromRI);
	uintmax_t curAddrN = getOffVN.unpackBE64();
	uintmax_t curAddrS = getOffVS.unpackBE64();
	for(uintptr_t i = fromRI; i<toRI; i++){
		uintmax_t nextAddrN = getOffVN.unpackBE64();
		uintmax_t nextAddrS = getOffVS.unpackBE64();
		if(nextAddrN < curAddrN){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Name index file not monotonic.", 0, 0); }
		if(nextAddrS < curAddrS){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Sequence index file not monotonic.", 0, 0); }
		if((nextAddrN - baseAddrN) > nameData.len){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index name file out of order.", 0, 0); }
		if((nextAddrS - baseAddrS) > seqData.len){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index sequence file out of order.", 0, 0); }
		fillName->txt = nameData.txt + (curAddrN - baseAddrN);
		fillName->len = nextAddrN - curAddrN;
		fillSeq->txt = seqData.txt + (curAddrS - baseAddrS);
		fillSeq->len = nextAddrS - curAddrS;
		fillName++;
		fillSeq++;
		curAddrN = nextAddrN;
		curAddrS = nextAddrS;
	}
}

ChunkySequenceWriter::ChunkySequenceWriter(OutStream* nameAnnotationFile, OutStream* nameFile, OutStream* sequenceAnnotationFile, OutStream* sequenceFile){
	nAtt = nameAnnotationFile;
	nDat = nameFile;
	sAtt = sequenceAnnotationFile;
	sDat = sequenceFile;
	totalNByte = 0;
	totalSByte = 0;
	usePool = 0;
	{
		passUnis.push_back(new ChunkySequenceWriteTask());
	}
}
ChunkySequenceWriter::ChunkySequenceWriter(OutStream* nameAnnotationFile, OutStream* nameFile, OutStream* sequenceAnnotationFile, OutStream* sequenceFile, uintptr_t numThread, ThreadPool* mainPool){
	nAtt = nameAnnotationFile;
	nDat = nameFile;
	sAtt = sequenceAnnotationFile;
	sDat = sequenceFile;
	totalNByte = 0;
	totalSByte = 0;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new ChunkySequenceWriteTask());
	}
}
ChunkySequenceWriter::~ChunkySequenceWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void ChunkySequenceWriter::write(SequenceSet* toStore){
	uintptr_t numThread = passUnis.size();
	uintptr_t numSeq = toStore->saveNames.size();
	uintptr_t numPT = numSeq / numThread;
	uintptr_t numET = numSeq % numThread;
	//figure out the number of bytes
		uintptr_t curSI = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkySequenceWriteTask* curT = (ChunkySequenceWriteTask*)(passUnis[i]);
			curT->phase = 1;
			curT->fromRI = curSI;
			curSI += (numPT + (i<numET));
			curT->toRI = curSI;
			curT->toStore = toStore;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
		uintptr_t totNumBN = 0;
		uintptr_t totNumBS = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkySequenceWriteTask* curT = (ChunkySequenceWriteTask*)(passUnis[i]);
			totNumBN += curT->numEatBytesN;
			totNumBS += curT->numEatBytesS;
		}
	//pack up the names
		packARB.clear();
		packARB.resize(numSeq * BLOCKCOMP_ANNOT_ENTLEN);
		packDatums.clear();
		packDatums.resize(totNumBN);
		char* curAnnot = packARB[0];
		char* curDatum = packDatums[0];
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkySequenceWriteTask* curT = (ChunkySequenceWriteTask*)(passUnis[i]);
			curT->phase = 2;
			curT->startByteIN = totalNByte;
			curT->annotData = curAnnot;
			curT->tableData = curDatum;
			curAnnot += ((curT->toRI - curT->fromRI) * BLOCKCOMP_ANNOT_ENTLEN);
			curDatum += curT->numEatBytesN;
			totalNByte += curT->numEatBytesN;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//dump the names
		nAtt->write(packARB[0], packARB.size());
		nDat->write(packDatums[0], totNumBN);
	//pack the sequence
		packDatums.clear();
		packDatums.resize(totNumBS);
		curAnnot = packARB[0];
		curDatum = packDatums[0];
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkySequenceWriteTask* curT = (ChunkySequenceWriteTask*)(passUnis[i]);
			curT->phase = 3;
			curT->startByteIS = totalSByte;
			curT->annotData = curAnnot;
			curT->tableData = curDatum;
			curAnnot += ((curT->toRI - curT->fromRI) * BLOCKCOMP_ANNOT_ENTLEN);
			curDatum += curT->numEatBytesS;
			totalSByte += curT->numEatBytesS;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//dump the sequence
		sAtt->write(packARB[0], packARB.size());
		sDat->write(packDatums[0], totNumBS);
}
void ChunkySequenceWriter::close(){
	isClosed = 1;
}

ChunkySequenceWriteTask::ChunkySequenceWriteTask(){}
ChunkySequenceWriteTask::~ChunkySequenceWriteTask(){}
void ChunkySequenceWriteTask::doTask(){
	if(phase == 1){
		numEatBytesN = 0;
		numEatBytesS = 0;
		for(uintptr_t i = fromRI; i<toRI; i++){
			numEatBytesN += toStore->saveNames[i]->len;
			numEatBytesS += toStore->saveStrs[i]->len;
		}
	}
	else{
		BytePacker curPackA(annotData);
		char* curFillD = tableData;
		uintmax_t curByteI;
		SizePtrString* curDumpS;
		if(phase == 2){
			curByteI = startByteIN;
			curDumpS = toStore->saveNames[fromRI];
		}
		else{
			curByteI = startByteIS;
			curDumpS = toStore->saveStrs[fromRI];
		}
		for(uintptr_t i = fromRI; i<toRI; i++){
			curPackA.packBE64(curByteI);
			uintptr_t curL = curDumpS->len;
			curByteI += curL;
			memcpy(curFillD, curDumpS->txt, curL);
			curFillD += curL;
			curDumpS++;
		}
	}
}

ExtensionSequenceReader::ExtensionSequenceReader(const char* fileName, InStream* useStdin){
	openUp(fileName, 1, 0, useStdin);
}
ExtensionSequenceReader::ExtensionSequenceReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	openUp(fileName, numThread, mainPool, useStdin);
}
ExtensionSequenceReader::~ExtensionSequenceReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionSequenceReader::read(SequenceSet* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionSequenceReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionSequenceReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			InStream* useBase = useStdin;
			if(!useBase){
				useBase = new ConsoleInStream();
				baseStrs.push_back(useBase);
			}
			wrapStr = mainPool ? new FastaSequenceReader(useBase, numThread, mainPool) : new FastaSequenceReader(useBase);
			return;
		}
		//anything explicitly fasta
		{
			int isFasta = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta"));
			int isFa = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa"));
			int isFastaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta.gz"));
			int isFaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa.gz"));
			int isFastaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta.gzip"));
			int isFaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa.gzip"));
			if(isFasta || isFa || isFastaGz || isFaGz || isFastaGzip || isFaGzip){
				InStream* useBase;
				if(isFasta || isFa){
					useBase = new FileInStream(fileName);
				}
				else{
					useBase = new GZipInStream(fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new FastaSequenceReader(useBase, numThread, mainPool) : new FastaSequenceReader(useBase);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcseq"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcseq"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcseq"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcseq"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed sequence data.", 1, packExt);
				}
				std::string nameFileName(fileName);
				std::string nameFileBlock(fileName); nameFileBlock.append(".blk");
				std::string nameIndName(fileName); nameIndName.append(".ind");
				std::string nameIndBlock(fileName); nameIndBlock.append(".ind.blk");
				std::string seqFileName(fileName); seqFileName.append(".seq");
				std::string seqFileBlock(fileName); seqFileBlock.append(".seq.blk");
				std::string seqIndName(fileName); seqIndName.append(".seq.ind");
				std::string seqIndBlock(fileName); seqIndBlock.append(".seq.ind.blk");
				RandaccInStream* nameS = mainPool ? new BlockCompInStream(nameFileName.c_str(), nameFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(nameFileName.c_str(), nameFileBlock.c_str(), compMeth);
				baseStrs.push_back(nameS);
				RandaccInStream* nameI = mainPool ? new BlockCompInStream(nameIndName.c_str(), nameIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(nameIndName.c_str(), nameIndBlock.c_str(), compMeth);
				baseStrs.push_back(nameI);
				RandaccInStream* seqS = mainPool ? new BlockCompInStream(seqFileName.c_str(), seqFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(seqFileName.c_str(), seqFileBlock.c_str(), compMeth);
				baseStrs.push_back(seqS);
				RandaccInStream* seqI = mainPool ? new BlockCompInStream(seqIndName.c_str(), seqIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(seqIndName.c_str(), seqIndBlock.c_str(), compMeth);
				baseStrs.push_back(seqI);
				wrapStr = mainPool ? new ChunkySequenceReader(nameI, nameS, seqI, seqS) : new ChunkySequenceReader(nameI, nameS, seqI, seqS, numThread, mainPool);
				delete(compMeth);
				return;
			}
		}
		//fallback to fasta for anything weird
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = mainPool ? new FastaSequenceReader(baseStrs[0], numThread, mainPool) : new FastaSequenceReader(baseStrs[0]);
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

ExtensionRandacSequenceReader::ExtensionRandacSequenceReader(const char* fileName){
	openUp(fileName, 1, 0);
}
ExtensionRandacSequenceReader::ExtensionRandacSequenceReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	openUp(fileName, numThread, mainPool);
}
ExtensionRandacSequenceReader::~ExtensionRandacSequenceReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionRandacSequenceReader::read(SequenceSet* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionRandacSequenceReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
uintmax_t ExtensionRandacSequenceReader::size(){
	return wrapStr->size();
}
void ExtensionRandacSequenceReader::seek(uintmax_t index){
	wrapStr->seek(index);
}
void ExtensionRandacSequenceReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcseq"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcseq"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcseq"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcseq"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed sequence data.", 1, packExt);
				}
				std::string nameFileName(fileName);
				std::string nameFileBlock(fileName); nameFileBlock.append(".blk");
				std::string nameIndName(fileName); nameIndName.append(".ind");
				std::string nameIndBlock(fileName); nameIndBlock.append(".ind.blk");
				std::string seqFileName(fileName); seqFileName.append(".seq");
				std::string seqFileBlock(fileName); seqFileBlock.append(".seq.blk");
				std::string seqIndName(fileName); seqIndName.append(".seq.ind");
				std::string seqIndBlock(fileName); seqIndBlock.append(".seq.ind.blk");
				RandaccInStream* nameS = mainPool ? new BlockCompInStream(nameFileName.c_str(), nameFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(nameFileName.c_str(), nameFileBlock.c_str(), compMeth);
				baseStrs.push_back(nameS);
				RandaccInStream* nameI = mainPool ? new BlockCompInStream(nameIndName.c_str(), nameIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(nameIndName.c_str(), nameIndBlock.c_str(), compMeth);
				baseStrs.push_back(nameI);
				RandaccInStream* seqS = mainPool ? new BlockCompInStream(seqFileName.c_str(), seqFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(seqFileName.c_str(), seqFileBlock.c_str(), compMeth);
				baseStrs.push_back(seqS);
				RandaccInStream* seqI = mainPool ? new BlockCompInStream(seqIndName.c_str(), seqIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(seqIndName.c_str(), seqIndBlock.c_str(), compMeth);
				baseStrs.push_back(seqI);
				wrapStr = mainPool ? new ChunkySequenceReader(nameI, nameS, seqI, seqS) : new ChunkySequenceReader(nameI, nameS, seqI, seqS, numThread, mainPool);
				delete(compMeth);
				return;
			}
		}
		//complain on anything weird
		{
			const char* packExt[] = {fileName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown file format for sequence data.", 1, packExt);
		}
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

ExtensionSequenceWriter::ExtensionSequenceWriter(const char* fileName, OutStream* useStdout){
	openUp(fileName, 1, 0, useStdout);
}
ExtensionSequenceWriter::ExtensionSequenceWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	openUp(fileName, numThread, mainPool, useStdout);
}
ExtensionSequenceWriter::~ExtensionSequenceWriter(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionSequenceWriter::write(SequenceSet* toStore){
	wrapStr->write(toStore);
}
void ExtensionSequenceWriter::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionSequenceWriter::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			OutStream* useBase = useStdout;
			if(!useBase){
				useBase = new ConsoleOutStream();
				baseStrs.push_back(useBase);
			}
			wrapStr = mainPool ? new FastaSequenceWriter(useBase, numThread, mainPool) : new FastaSequenceWriter(useBase);
			return;
		}
		//anything explicitly fasta
		{
			int isFasta = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta"));
			int isFa = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa"));
			int isFastaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta.gz"));
			int isFaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa.gz"));
			int isFastaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fasta.gzip"));
			int isFaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fa.gzip"));
			if(isFasta || isFa || isFastaGz || isFaGz || isFastaGzip || isFaGzip){
				OutStream* useBase;
				if(isFasta || isFa){
					useBase = new FileOutStream(0, fileName);
				}
				else{
					useBase = new GZipOutStream(0, fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new FastaSequenceWriter(useBase, numThread, mainPool) : new FastaSequenceWriter(useBase);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcseq"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcseq"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcseq"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcseq"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed sequence data.", 1, packExt);
				}
				std::string nameFileName(fileName);
				std::string nameFileBlock(fileName); nameFileBlock.append(".blk");
				std::string nameIndName(fileName); nameIndName.append(".ind");
				std::string nameIndBlock(fileName); nameIndBlock.append(".ind.blk");
				std::string seqFileName(fileName); seqFileName.append(".seq");
				std::string seqFileBlock(fileName); seqFileBlock.append(".seq.blk");
				std::string seqIndName(fileName); seqIndName.append(".seq.ind");
				std::string seqIndBlock(fileName); seqIndBlock.append(".seq.ind.blk");
				OutStream* nameS = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, nameFileName.c_str(), nameFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, nameFileName.c_str(), nameFileBlock.c_str(), compMeth);
				baseStrs.push_back(nameS);
				OutStream* nameI = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, nameIndName.c_str(), nameIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, nameIndName.c_str(), nameIndBlock.c_str(), compMeth);
				baseStrs.push_back(nameI);
				OutStream* seqS = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, seqFileName.c_str(), seqFileBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, seqFileName.c_str(), seqFileBlock.c_str(), compMeth);
				baseStrs.push_back(seqS);
				OutStream* seqI = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, seqIndName.c_str(), seqIndBlock.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, seqIndName.c_str(), seqIndBlock.c_str(), compMeth);
				baseStrs.push_back(seqI);
				wrapStr = mainPool ? new ChunkySequenceWriter(nameI, nameS, seqI, seqS) : new ChunkySequenceWriter(nameI, nameS, seqI, seqS, numThread, mainPool);
				delete(compMeth);
				return;
			}
		}
		//fallback to fasta for anything weird
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = mainPool ? new FastaSequenceWriter(baseStrs[0], numThread, mainPool) : new FastaSequenceWriter(baseStrs[0]);
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

ArgumentOptionSequenceRead::ArgumentOptionSequenceRead(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" seqs.fasta");
	summary = useDesc;
	validExts.push_back(".fasta");
	validExts.push_back(".fa");
	validExts.push_back(".fasta.gz");
	validExts.push_back(".fa.gz");
	validExts.push_back(".fasta.gzip");
	validExts.push_back(".fa.gzip");
	validExts.push_back(".raw.bcseq");
	//validExts.push_back(".gzip.bcseq");
	validExts.push_back(".zlib.bcseq");
}
ArgumentOptionSequenceRead::~ArgumentOptionSequenceRead(){}

ArgumentOptionSequenceRandac::ArgumentOptionSequenceRandac(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" seqs.fasta");
	summary = useDesc;
	validExts.push_back(".raw.bcseq");
	//validExts.push_back(".gzip.bcseq");
	validExts.push_back(".zlib.bcseq");
}
ArgumentOptionSequenceRandac::~ArgumentOptionSequenceRandac(){}

ArgumentOptionSequenceWrite::ArgumentOptionSequenceWrite(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileWrite(theName){
	required = needed;
	usage = theName;
		usage.append(" seqs.fasta");
	summary = useDesc;
	validExts.push_back(".fasta");
	validExts.push_back(".fa");
	validExts.push_back(".fasta.gz");
	validExts.push_back(".fa.gz");
	validExts.push_back(".fasta.gzip");
	validExts.push_back(".fa.gzip");
	validExts.push_back(".raw.bcseq");
	//validExts.push_back(".gzip.bcseq");
	validExts.push_back(".zlib.bcseq");
}
ArgumentOptionSequenceWrite::~ArgumentOptionSequenceWrite(){}




