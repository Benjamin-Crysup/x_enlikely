#include "whodun_gen_weird.h"

#include "whodun_compress.h"

namespace whodun {

/**Parse a fastq file.*/
class FastqReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	FastqReadTask();
	/**Clean up.*/
	~FastqReadTask();
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
	/**The indices of the tokens that actually have something (not purely whitespace)*/
	std::vector<uintptr_t> nameTokenIs;
	
	//phase 2 - collect the tokens into one list
	/**The place to put token indices.*/
	uintptr_t* fillTokenTgt;
	
	//phase 3 - pack the pieces together
	/**The first line this works on.*/
	uintptr_t nameTSI;
	/**The last line this works on.*/
	uintptr_t nameTEI;
	/**The line token indices.*/
	uintptr_t* allNameTIs;
	/**The place to fix up.*/
	FastqSet* toStore;
};

/**Pack a fasta file for output.*/
class FastqWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	FastqWriteTask();
	/**Clean up.*/
	~FastqWriteTask();
	void doTask();
	
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - figure total length needed
	/**The place to fix up.*/
	FastqSet* toStore;
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

};

using namespace whodun;

FastqSet::FastqSet(){}
FastqSet::~FastqSet(){}

FastqReader::FastqReader(){
	isClosed = 0;
}
FastqReader::~FastqReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

FastqWriter::FastqWriter(){
	isClosed = 0;
}
FastqWriter::~FastqWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

AsciiFastqReader::AsciiFastqReader(InStream* mainFrom){
	theStr = mainFrom;
	rowSplitter = new CharacterSplitTokenizer('\n');
	charMove = new StandardMemoryShuttler();
	usePool = 0;
	{
		passUnis.push_back(new FastqReadTask());
	}
}
AsciiFastqReader::AsciiFastqReader(InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	rowSplitter = new CharacterSplitTokenizer('\n');
	charMove = new StandardMemoryShuttler();
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new FastqReadTask());
	}
}
AsciiFastqReader::~AsciiFastqReader(){
	delete(rowSplitter);
	delete(charMove);
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t AsciiFastqReader::read(FastqSet* toStore, uintptr_t numSeqs){
	if(numSeqs == 0){
		toStore->saveNames.clear();
		toStore->saveStrs.clear();
		toStore->savePhreds.clear();
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
	//read until enough lines encountered
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
			//figure out if more neads to be read
				if(haveHitEOF || (saveRowS.size() >= 8*numSeqs)){
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
	//figure out which rows have stuff
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = saveRowS.size() / numThread;
		uintptr_t numET = saveRowS.size() % numThread;
		uintptr_t curLineI = 0;
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			FastqReadTask* curT = (FastqReadTask*)passUnis[i];
			curT->phase = 1;
			curT->tokenSI = curLineI;
			uintptr_t curNum = numPT + (i<numET);
			curLineI += curNum;
			curT->tokenEI = curLineI;
			curT->theToken = saveRowS[0];
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//see how many have things
		uintptr_t totalNumL = 0;
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			FastqReadTask* curT = (FastqReadTask*)passUnis[i];
			totalNumL += curT->nameTokenIs.size();
		}
		saveSeqHS.clear();
		saveSeqHS.resize(totalNumL);
		uintptr_t* curTokTgt = saveSeqHS[0];
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			FastqReadTask* curT = (FastqReadTask*)passUnis[i];
			curT->phase = 2;
			curT->fillTokenTgt = curTokTgt;
			curTokTgt += curT->nameTokenIs.size();
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//cut back if not eof, complain if not properly divisible
		uintptr_t wholeNumS = (totalNumL / 4);
		uintptr_t numEatL;
		if(wholeNumS > numSeqs){
			wholeNumS = numSeqs;
			numEatL = 4*wholeNumS;
		}
		else{
			numEatL = 4*wholeNumS;
			if(haveHitEOF && (numEatL != totalNumL)){
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Truncated fastq file.", 0, 0);
			}
		}
	//and save any overhang
		if(numEatL != totalNumL){
			char* remTerm = toStore->saveText[toStore->saveText.size()];
			char* remStart = saveRowS[numEatL]->text.txt;
			saveTexts.resize(remTerm - remStart);
			charMove->memcpy(saveTexts[0], remStart, remTerm - remStart);
		}
	//pack the results
		toStore->saveNames.clear(); toStore->saveNames.resize(wholeNumS);
		toStore->saveStrs.clear(); toStore->saveStrs.resize(wholeNumS);
		toStore->savePhreds.clear(); toStore->savePhreds.resize(wholeNumS);
		uintptr_t numSPT = wholeNumS / numThread;
		uintptr_t numSET = wholeNumS % numThread;
		uintptr_t curLI = 0;
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			FastqReadTask* curT = (FastqReadTask*)passUnis[i];
			curT->phase = 3;
			uintptr_t curNum = numSPT + (i < numSET);
			curT->nameTSI = curLI;
			curLI += curNum;
			curT->nameTEI = curLI;
			curT->allNameTIs = saveSeqHS[0];
			curT->toStore = toStore;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	return wholeNumS;
}
void AsciiFastqReader::close(){
	isClosed = 1;
}

FastqReadTask::FastqReadTask(){}
FastqReadTask::~FastqReadTask(){}
void FastqReadTask::doTask(){
	if(phase == 1){
		StandardMemorySearcher doTrim;
		nameTokenIs.clear();
		for(uintptr_t i = tokenSI; i<tokenEI; i++){
			Token* curLTok = theToken + i;
			SizePtrString trimS = doTrim.trim(curLTok->text);
			if(trimS.len == 0){ continue; }
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
			SizePtrString nameL = doTrim.trim(theToken[allNameTIs[4*si]].text);
			SizePtrString seqL = doTrim.trim(theToken[allNameTIs[4*si+1]].text);
			SizePtrString splitL = doTrim.trim(theToken[allNameTIs[4*si+2]].text);
			SizePtrString qualL = doTrim.trim(theToken[allNameTIs[4*si+3]].text);
			if(nameL.txt[0] != '@'){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed name line in fastq file.", 0, 0); }
			nameL.txt++;
			nameL.len--;
			if(splitL.txt[0] != '+'){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed entry split line in fastq file.", 0, 0); }
			if(seqL.len != qualL.len){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Sequence and quality should have the same length.", 0, 0); }
			*(toStore->saveNames[si]) = nameL;
			*(toStore->saveStrs[si]) = seqL;
			*(toStore->savePhreds[si]) = qualL;
		}
	}
}

AsciiFastqWriter::AsciiFastqWriter(OutStream* mainFrom){
	theStr = mainFrom;
	usePool = 0;
	{
		passUnis.push_back(new FastqWriteTask());
	}
}
AsciiFastqWriter::AsciiFastqWriter(OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new FastqWriteTask());
	}
}
AsciiFastqWriter::~AsciiFastqWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void AsciiFastqWriter::write(FastqSet* toStore){
	//figure out the needed size
		uintptr_t numSeqs = toStore->saveNames.size();
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numSeqs / numThread;
		uintptr_t numET = numSeqs % numThread;
		uintptr_t curSI = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			FastqWriteTask* curT = (FastqWriteTask*)(passUnis[i]);
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
			FastqWriteTask* curT = (FastqWriteTask*)(passUnis[i]);
			totalNeedS += curT->totalNeededS;
		}
		saveTexts.clear();
		saveTexts.resize(totalNeedS);
		char* curTgtT = saveTexts[0];
		for(uintptr_t i = 0; i<numThread; i++){
			FastqWriteTask* curT = (FastqWriteTask*)(passUnis[i]);
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
void AsciiFastqWriter::close(){
	isClosed = 1;
}

FastqWriteTask::FastqWriteTask(){}
FastqWriteTask::~FastqWriteTask(){}
void FastqWriteTask::doTask(){
	if(phase == 1){
		SizePtrString* curName = toStore->saveNames[seqSI];
		SizePtrString* curSeq = toStore->saveStrs[seqSI];
		SizePtrString* curQual = toStore->savePhreds[seqSI];
		totalNeededS = 0;
		for(uintptr_t i = seqSI; i<seqEI; i++){
			totalNeededS += 6;
			totalNeededS += curName->len;
			totalNeededS += curSeq->len;
			totalNeededS += curQual->len;
			curName++;
			curSeq++;
			curQual++;
		}
	}
	else{
		SizePtrString* curName = toStore->saveNames[seqSI];
		SizePtrString* curSeq = toStore->saveStrs[seqSI];
		SizePtrString* curQual = toStore->savePhreds[seqSI];
		char* curFill = packLoc;
		for(uintptr_t i = seqSI; i<seqEI; i++){
			*curFill = '@'; curFill++;
			memcpy(curFill, curName->txt, curName->len); curFill += curName->len;
			*curFill = '\n'; curFill++;
			memcpy(curFill, curSeq->txt, curSeq->len); curFill += curSeq->len;
			*curFill = '\n'; curFill++;
			*curFill = '+'; curFill++;
			*curFill = '\n'; curFill++;
			memcpy(curFill, curQual->txt, curQual->len); curFill += curQual->len;
			*curFill = '\n'; curFill++;
			curName++;
			curSeq++;
			curQual++;
		}
	}
}

ExtensionFastqReader::ExtensionFastqReader(const char* fileName, InStream* useStdin){
	openUp(fileName, 1, 0, useStdin);
}
ExtensionFastqReader::ExtensionFastqReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	openUp(fileName, numThread, mainPool, useStdin);
}
ExtensionFastqReader::~ExtensionFastqReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionFastqReader::read(FastqSet* toStore, uintptr_t numSeqs){
	return wrapStr->read(toStore, numSeqs);
}
void ExtensionFastqReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionFastqReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
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
			wrapStr = mainPool ? new AsciiFastqReader(useBase, numThread, mainPool) : new AsciiFastqReader(useBase);
			return;
		}
		//anything explicitly fastq
		{
			int isFasta = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq"));
			int isFa = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq"));
			int isFastaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq.gz"));
			int isFaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq.gz"));
			int isFastaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq.gzip"));
			int isFaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq.gzip"));
			if(isFasta || isFa || isFastaGz || isFaGz || isFastaGzip || isFaGzip){
				InStream* useBase;
				if(isFasta || isFa){
					useBase = new FileInStream(fileName);
				}
				else{
					useBase = new GZipInStream(fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new AsciiFastqReader(useBase, numThread, mainPool) : new AsciiFastqReader(useBase);
				return;
			}
		}
		//fallback to fastq for anything weird
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = mainPool ? new AsciiFastqReader(baseStrs[0], numThread, mainPool) : new AsciiFastqReader(baseStrs[0]);
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

ExtensionFastqWriter::ExtensionFastqWriter(const char* fileName, OutStream* useStdout){
	openUp(fileName, 1, 0, useStdout);
}
ExtensionFastqWriter::ExtensionFastqWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	openUp(fileName, numThread, mainPool, useStdout);
}
ExtensionFastqWriter::~ExtensionFastqWriter(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionFastqWriter::write(FastqSet* toStore){
	wrapStr->write(toStore);
}
void ExtensionFastqWriter::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionFastqWriter::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
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
			wrapStr = mainPool ? new AsciiFastqWriter(useBase, numThread, mainPool) : new AsciiFastqWriter(useBase);
			return;
		}
		//anything explicitly fastq
		{
			int isFasta = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq"));
			int isFa = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq"));
			int isFastaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq.gz"));
			int isFaGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq.gz"));
			int isFastaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fastq.gzip"));
			int isFaGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".fq.gzip"));
			if(isFasta || isFa || isFastaGz || isFaGz || isFastaGzip || isFaGzip){
				OutStream* useBase;
				if(isFasta || isFa){
					useBase = new FileOutStream(0, fileName);
				}
				else{
					useBase = new GZipOutStream(0, fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new AsciiFastqWriter(useBase, numThread, mainPool) : new AsciiFastqWriter(useBase);
				return;
			}
		}
		//fallback to fastq for anything weird
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = mainPool ? new AsciiFastqWriter(baseStrs[0], numThread, mainPool) : new AsciiFastqWriter(baseStrs[0]);
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


ArgumentOptionFastqRead::ArgumentOptionFastqRead(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" seqs.fastq");
	summary = useDesc;
	validExts.push_back(".fastq");
	validExts.push_back(".fq");
	validExts.push_back(".fastq.gz");
	validExts.push_back(".fq.gz");
	validExts.push_back(".fastq.gzip");
	validExts.push_back(".fq.gzip");
}
ArgumentOptionFastqRead::~ArgumentOptionFastqRead(){}

ArgumentOptionFastqWrite::ArgumentOptionFastqWrite(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileWrite(theName){
	required = needed;
	usage = theName;
		usage.append(" seqs.fastq");
	summary = useDesc;
	validExts.push_back(".fastq");
	validExts.push_back(".fq");
	validExts.push_back(".fastq.gz");
	validExts.push_back(".fq.gz");
	validExts.push_back(".fastq.gzip");
	validExts.push_back(".fq.gzip");
}
ArgumentOptionFastqWrite::~ArgumentOptionFastqWrite(){}



