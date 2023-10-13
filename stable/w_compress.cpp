#include "whodun_compress.h"

namespace whodun {

/**Perform raw compression.*/
class RawCompressionMethod : public CompressionMethod{
public:
	void compressData(SizePtrString theData);
};

/**Perform raw decompression.*/
class RawDecompressionMethod : public DecompressionMethod{
public:
	void expandData(SizePtrString theComp);
};

/**Perform deflate compression.*/
class DeflateCompressionMethod : public CompressionMethod{
public:
	void compressData(SizePtrString theData);
};

/**Perform deflate decompression.*/
class DeflateDecompressionMethod : public DecompressionMethod{
public:
	void expandData(SizePtrString theComp);
};

/**Perform gzip compression.*/
class GZipCompressionMethod : public CompressionMethod{
public:
	void compressData(SizePtrString theData);
	/**Add block compression metadata.*/
	int addBlockComp;
	/**Do bit fiddling*/
	BytePacker doPack;
};

/**Perform gzip decompression.*/
class GZipDecompressionMethod : public DecompressionMethod{
public:
	void expandData(SizePtrString theComp);
	/**Do bit fiddling*/
	ByteUnpacker doUPack;
};

/**Whether the table has been filled.*/
extern int gzipCRCTableFilled;
/**Precalculate some stuff for crc.*/
extern unsigned long gzipCRCTable[256];
/**Fill in said table.*/
void gzipFillInCRCTable();
/**Force running the above.*/
extern GZipCompressionFactory gzipForceFillCRCTable;

/**Compress a single block in a compression stream*/
class BlockCompOutStreamUniform : public JoinableThreadTask{
public:
	/**Set up clean.*/
	BlockCompOutStreamUniform();
	/**Clean up.*/
	~BlockCompOutStreamUniform();
	void doTask();
	/**The data to work over.*/
	SizePtrString theData;
	/**The compression method this uses.*/
	CompressionMethod* myComp;
};

/**Decompress a single block in a compression stream*/
class BlockCompInStreamUniform : public JoinableThreadTask{
public:
	/**Set up clean.*/
	BlockCompInStreamUniform();
	/**Clean up.*/
	~BlockCompInStreamUniform();
	void doTask();
	/**The compressed data to work over.*/
	SizePtrString theComp;
	/**The compression method this uses.*/
	DecompressionMethod* myComp;
	/**The place to copy the data to, if any.*/
	char* finalTgt;
	/**The offset to start copying from.*/
	uintptr_t copyOffset;
	/**The number of things to copy.*/
	uintptr_t copyCount;
	/**The expected decompressed length.*/
	uintmax_t expectDCLen;
};

};

using namespace whodun;

CompressionMethod::CompressionMethod(){
	allocSize = 1024;
	compData.txt = (char*)malloc(allocSize);
	compData.len = 0;
}
CompressionMethod::~CompressionMethod(){
	free(compData.txt);
}
DecompressionMethod::DecompressionMethod(){
	allocSize = 1024;
	theData.txt = (char*)malloc(allocSize);
	theData.len = 0;
}
DecompressionMethod::~DecompressionMethod(){
	free(theData.txt);
}
CompressionFactory::~CompressionFactory(){}

CompressionMethod* RawCompressionFactory::makeZip(){
	return new RawCompressionMethod();
}
DecompressionMethod* RawCompressionFactory::makeUnzip(){
	return new RawDecompressionMethod();
}

CompressionMethod* DeflateCompressionFactory::makeZip(){
	return new DeflateCompressionMethod();
}
DecompressionMethod* DeflateCompressionFactory::makeUnzip(){
	return new DeflateDecompressionMethod();
}

GZipCompressionFactory::GZipCompressionFactory(){
	gzipFillInCRCTable();
	addBlockComp = 0;
}
CompressionMethod* GZipCompressionFactory::makeZip(){
	GZipCompressionMethod* toRet = new GZipCompressionMethod();
	toRet->addBlockComp = addBlockComp;
	return toRet;
}
DecompressionMethod* GZipCompressionFactory::makeUnzip(){
	return new GZipDecompressionMethod();
}

void RawCompressionMethod::compressData(SizePtrString theData){
	if(theData.len > allocSize){
		free(compData.txt);
		allocSize = theData.len;
		compData.txt = (char*)malloc(allocSize);
	}
	memcpy(compData.txt, theData.txt, theData.len);
	compData.len = theData.len;
}
void RawDecompressionMethod::expandData(SizePtrString theComp){
	if(theComp.len > allocSize){
		free(theData.txt);
		allocSize = theComp.len;
		theData.txt = (char*)malloc(allocSize);
	}
	memcpy(theData.txt, theComp.txt, theComp.len);
	theData.len = theComp.len;
}

void DeflateCompressionMethod::compressData(SizePtrString theData){
	unsigned long bufEndSStore = allocSize;
	int compRes = 0;
	while((compRes = compress(((unsigned char*)(compData.txt)), &bufEndSStore, ((const unsigned char*)(theData.txt)), theData.len)) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Error compressing deflate data.", 0, 0);
		}
		allocSize = allocSize << 1;
		free(compData.txt);
		compData.txt = (char*)malloc(allocSize);
		bufEndSStore = allocSize;
	}
	compData.len = bufEndSStore;
}
void DeflateDecompressionMethod::expandData(SizePtrString theComp){
	unsigned long bufEndSStore = allocSize;
	int compRes = 0;
	while((compRes = uncompress(((unsigned char*)(theData.txt)), &bufEndSStore, ((const unsigned char*)(theComp.txt)), theComp.len)) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Error decompressing deflate data.", 0, 0);
		}
		allocSize = allocSize << 1;
		free(theData.txt);
		theData.txt = (char*)malloc(allocSize);
		bufEndSStore = allocSize;
	}
	theData.len = bufEndSStore;
}

int whodun::gzipCRCTableFilled = 0;
unsigned long whodun::gzipCRCTable[256];
void whodun::gzipFillInCRCTable(){
	if(gzipCRCTableFilled){ return; }
	unsigned long c;
	int n, k;
	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++) {
			if (c & 1) {
				c = 0xedb88320L ^ (c >> 1);
			} else {
				c = c >> 1;
			}
		}
		gzipCRCTable[n] = c;
	}
	gzipCRCTableFilled = 1;
}
GZipCompressionFactory whodun::gzipForceFillCRCTable;

void GZipCompressionMethod::compressData(SizePtrString theData){
	//figure out the offset for the header (allocation starts with 1k, so should be good)
	uintptr_t headerOff = 10 + (addBlockComp ? 8 : 0);
	uintptr_t reserveBytes = headerOff + 8;
	//compress directly
	unsigned long bufEndSStore = allocSize - reserveBytes;
	int compRes = 0;
	while((compRes = compress(((unsigned char*)(compData.txt + headerOff)), &bufEndSStore, ((const unsigned char*)(theData.txt)), theData.len)) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Error compressing gzip data.", 0, 0);
		}
		allocSize = allocSize << 1;
		free(compData.txt);
		compData.txt = (char*)malloc(allocSize);
		bufEndSStore = allocSize - reserveBytes;
	}
	compData.len = bufEndSStore + reserveBytes;
	//add in the basic header
	char* compDataT = compData.txt;
	compDataT[0] = 0x1F;
	compDataT[1] = 0x8B;
	compDataT[2] = 8;
	compDataT[3] = 0;
	compDataT[4] = 0;
	compDataT[5] = 0;
	compDataT[6] = 0;
	compDataT[7] = 0;
	compDataT[8] = 0;
	compDataT[9] = 255;
	//calculate the crc
	uint32_t crcV = 0xffffffffL;
	for(uintptr_t n = 0; n<theData.len; n++){
		crcV = gzipCRCTable[0x00FF & (crcV ^ theData.txt[n])] ^ (crcV >> 8);
	}
	crcV = crcV ^ 0xffffffffL;
	doPack.retarget(compDataT + (compData.len - 8));
	doPack.packLE32(crcV);
	//add in the length
	doPack.packLE32(theData.len);
	//add in any block comp info
	if(addBlockComp){
		compDataT[3] = compDataT[3] | 4;
		compDataT[10] = 6;
		compDataT[11] = 0;
		compDataT[12] = 66;
		compDataT[13] = 67;
		compDataT[14] = 2;
		compDataT[15] = 0;
		doPack.retarget(compDataT + 16);
		doPack.packLE16(compData.len-1);
	}
}
void GZipDecompressionMethod::expandData(SizePtrString theComp){
	if(theComp.len == 0){ theData.len = 0; return; }
	char* theCompT = theComp.txt;
	//skip any header data
	uintptr_t curHI;
	#define GZIP_HEADER_SIZE_CHECK if(curHI >= theComp.len){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "GZip data truncated.", 0, 0); }
	curHI = 3; GZIP_HEADER_SIZE_CHECK
	int flagD = theCompT[curHI];
	curHI = 10; GZIP_HEADER_SIZE_CHECK
	if(flagD & 4){
		 curHI += 2; GZIP_HEADER_SIZE_CHECK
		 doUPack.retarget(theCompT + curHI - 2);
		 uintptr_t xtraLen = doUPack.unpackLE16();
		 curHI += xtraLen;
	}
	if(flagD & 8){
		stillMoreFileName:
		GZIP_HEADER_SIZE_CHECK
		if(theCompT[curHI]){
			curHI++;
			goto stillMoreFileName;
		}
		curHI++;
	}
	if(flagD & 16){
		stillMoreComment:
		GZIP_HEADER_SIZE_CHECK
		if(theCompT[curHI]){
			curHI++;
			goto stillMoreComment;
		}
		curHI++;
	}
	if(flagD & 2){
		curHI += 2;
	}
	GZIP_HEADER_SIZE_CHECK
	//trim off the 8 bytes at the end (does not currently check CRC)
	uintptr_t endDI = theComp.len - 8;
	if(endDI < curHI){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "GZip data truncated.", 0, 0); }
	char* compDataB = theCompT + curHI;
	uintptr_t compDLen = endDI - curHI;
	//and decompress
	unsigned long bufEndSStore = allocSize;
	int compRes = 0;
	while((compRes = uncompress(((unsigned char*)(theData.txt)), &bufEndSStore, ((const unsigned char*)compDataB), compDLen)) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Error decompressing gzip data.", 0, 0);
		}
		allocSize = allocSize << 1;
		free(theData.txt);
		theData.txt = (char*)malloc(allocSize);
		bufEndSStore = allocSize;
	}
	theData.len = bufEndSStore;
}

GZipOutStream::GZipOutStream(int append, const char* fileName){
	myName = fileName;
	if(append){
		baseFile = gzopen(fileName, "ab");
	}
	else{
		baseFile = gzopen(fileName, "wb");
	}
	if(baseFile == 0){
		isClosed = 1;
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Could not open file.", 1, extras);
	}
}
GZipOutStream::~GZipOutStream(){}
void GZipOutStream::write(int toW){
	if(gzputc(baseFile, toW) < 0){
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem writing to file.", 1, extras);
	}
}
void GZipOutStream::write(const char* toW, uintptr_t numW){
	if(gzwrite(baseFile, toW, numW)!=((int)numW)){
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem writing to file.", 1, extras);
	}
}
void GZipOutStream::close(){
	if(isClosed){ return; }
	isClosed = 1;
	if(gzclose(baseFile)){
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem closing file.", 1, extras);
	}
}

GZipInStream::GZipInStream(const char* fileName){
	myName = fileName;
	baseFile = gzopen(fileName, "rb");
	if(baseFile == 0){
		isClosed = 1;
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Could not open file.", 1, extras);
	}
}
GZipInStream::~GZipInStream(){}
int GZipInStream::read(){
	int toR = gzgetc(baseFile);
	if(toR < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		if(gzerrcode && (gzerrcode != Z_STREAM_END)){
			const char* extras[] = {myName.c_str(),errMess};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem reading file.", 2, extras);
		}
	}
	return toR;
}
uintptr_t GZipInStream::read(char* toR, uintptr_t numR){
	int toRet = gzread(baseFile, toR, numR);
	if(toRet < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		const char* extras[] = {myName.c_str(),errMess};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem reading file.", 2, extras);
	}
	return toRet;
}
void GZipInStream::close(){
	if(isClosed){ return; }
	isClosed = 1;
	if(gzclose(baseFile)){
		const char* extras[] = {myName.c_str()};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem closing file.", 1, extras);
	}
}

uintmax_t whodun::blockCompressedFileGetSize(const char* annotFN){
	uintmax_t toRet = 0;
	FileInStream annotF(annotFN);
	try{
		uintmax_t annotLen = annotF.size();
		if(annotLen % WHODUN_BLOCKCOMP_ANNOT_ENTLEN){
			const char* extras[] = {annotFN};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed annotation file.", 1, extras);
		}
		if(annotLen == 0){ goto gotRes; }
		annotF.seek(annotLen - WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
		char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
		annotF.forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
		ByteUnpacker doUPack(annotBuff);
			toRet = doUPack.unpackBE64();
			doUPack.skip(8);
			toRet += doUPack.unpackBE64();
	}
	catch(std::exception& errE){
		annotF.close();
		throw;
	}
	gotRes:
	annotF.close();
	return toRet;
}

BlockCompOutStream::BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth){
	mainF = 0;
	annotF = 0;
	chunkSize = blockSize;
	chunkMarshal = (char*)malloc(chunkSize);
	try{
		if(append && fileExists(annotFN)){
			intmax_t annotLen = fileGetSize(annotFN);
			if(annotLen < 0){
				const char* extras[] = {annotFN};
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem examining annotation file.", 1, extras);
			}
			if(annotLen % WHODUN_BLOCKCOMP_ANNOT_ENTLEN){
				const char* extras[] = {annotFN};
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed annotation file.", 1, extras);
			}
			FileInStream checkIt(annotFN);
			checkIt.seek(annotLen - WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
			checkIt.forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			checkIt.close();
			ByteUnpacker doUPack(annotBuff);
				preCompBS = doUPack.unpackBE64();
				postCompBS = doUPack.unpackBE64();
				preCompBS += doUPack.unpackBE64();
				postCompBS += doUPack.unpackBE64();
		}
		else{
			preCompBS = 0;
			postCompBS = 0;
		}
		mainF = new FileOutStream(append, mainFN);
		annotF = new FileOutStream(append, annotFN);
		totalWrite = preCompBS;
		compThreads = 0;
		numMarshal = 0;
		{
			BlockCompOutStreamUniform* curUni = new BlockCompOutStreamUniform();
			curUni->myComp = compMeth->makeZip();
			threadPass.push_back(curUni);
		}
	}
	catch(std::exception& errE){
		if(mainF){ try{mainF->close();}catch(std::exception& errB){} delete(mainF); }
		if(annotF){ try{annotF->close();}catch(std::exception& errB){} delete(annotF); }
		free(chunkMarshal);
		isClosed = 1;
		throw;
	}
}
BlockCompOutStream::BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth, uintptr_t numThreads, ThreadPool* useThreads){
	mainF = 0;
	annotF = 0;
	chunkSize = blockSize;
	chunkMarshal = (char*)malloc(chunkSize);
	try{
		if(append && fileExists(annotFN)){
			intmax_t annotLen = fileGetSize(annotFN);
			if(annotLen < 0){
				const char* extras[] = {annotFN};
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_IO, __FILE__, __LINE__, "Problem examining annotation file.", 1, extras);
			}
			if(annotLen % WHODUN_BLOCKCOMP_ANNOT_ENTLEN){
				const char* extras[] = {annotFN};
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed annotation file.", 1, extras);
			}
			FileInStream checkIt(annotFN);
			checkIt.seek(annotLen - WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
			checkIt.forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			checkIt.close();
			ByteUnpacker doUPack(annotBuff);
				preCompBS = doUPack.unpackBE64();
				postCompBS = doUPack.unpackBE64();
				preCompBS += doUPack.unpackBE64();
				postCompBS += doUPack.unpackBE64();
		}
		else{
			preCompBS = 0;
			postCompBS = 0;
		}
		mainF = new FileOutStream(append, mainFN);
		annotF = new FileOutStream(append, annotFN);
		totalWrite = preCompBS;
		compThreads = useThreads;
		numMarshal = 0;
		for(uintptr_t i = 0; i<numThreads; i++){
			BlockCompOutStreamUniform* curUni = new BlockCompOutStreamUniform();
			curUni->myComp = compMeth->makeZip();
			threadPass.push_back(curUni);
		}
	}
	catch(std::exception& errE){
		if(mainF){ try{mainF->close();}catch(std::exception& errB){} delete(mainF); }
		if(annotF){ try{annotF->close();}catch(std::exception& errB){} delete(annotF); }
		free(chunkMarshal);
		isClosed = 1;
		throw;
	}
}
BlockCompOutStream::~BlockCompOutStream(){
	for(uintptr_t i = 0; i<threadPass.size(); i++){
		delete(threadPass[i]);
	}
	delete(mainF);
	delete(annotF);
	free(chunkMarshal);
}
void BlockCompOutStream::write(int toW){
	chunkMarshal[numMarshal] = toW;
	numMarshal++;
	if(numMarshal == chunkSize){
		compressAndOut(chunkSize, chunkMarshal);
		numMarshal = 0;
	}
}
void BlockCompOutStream::write(const char* toW, uintptr_t numW){
	const char* nextW = toW;
	uintptr_t leftW = numW;
	
	tailRecurTgt:
	//check for dangling on the tail
	if(leftW < (chunkSize - numMarshal)){
		memcpy(chunkMarshal + numMarshal, nextW, leftW);
		numMarshal += leftW;
		return;
	}
	//check for dangling on the head
	if(numMarshal){
		uintptr_t numEat = chunkSize - numMarshal;
		memcpy(chunkMarshal + numMarshal, nextW, numEat);
		compressAndOut(chunkSize, chunkMarshal);
		numMarshal = 0;
		nextW += numEat;
		leftW -= numEat;
		goto tailRecurTgt;
	}
	//eat as many blocks as you can
	uintptr_t numEatBlocks = leftW / chunkSize;
		numEatBlocks = std::min(numEatBlocks, (uintptr_t)(threadPass.size()));
	uintptr_t numEatBytes = numEatBlocks * chunkSize;
	compressAndOut(numEatBytes, nextW);
	nextW += numEatBytes;
	leftW -= numEatBytes;
	if(leftW){ goto tailRecurTgt; }
}
void BlockCompOutStream::flush(){
	if(numMarshal){
		compressAndOut(numMarshal, chunkMarshal);
		numMarshal = 0;
	}
}
void BlockCompOutStream::close(){
	isClosed = 1;
	flush();
	mainF->close();
	annotF->close();
}
uintmax_t BlockCompOutStream::tell(){
	return totalWrite;
}
void BlockCompOutStream::compressAndOut(uintptr_t numDump, const char* dumpFrom){
	uintptr_t maxUseT = numDump / chunkSize;
		if(numDump % chunkSize){ maxUseT++; }
	//set up the compression
		uintptr_t numPT = numDump / maxUseT;
		uintptr_t numET = numDump % maxUseT;
		uintptr_t curOff = 0;
		for(uintptr_t i = 0; i<maxUseT; i++){
			uintptr_t curNum = numPT + (i < numET);
			BlockCompOutStreamUniform* curU = (BlockCompOutStreamUniform*)(threadPass[i]);
			curU->reset();
			curU->theData.txt = (char*)(dumpFrom + curOff);
			curU->theData.len = curNum;
			curOff += curNum;
		}
	//send them to the pool
		if(compThreads){
			compThreads->addTasks(maxUseT, (JoinableThreadTask**)&(threadPass[0]));
			joinTasks(maxUseT, &(threadPass[0]));
		}
		else{
			threadPass[0]->doTask();
		}
	//actually spit things out to file
		BytePacker doPack;
		for(uintptr_t ui = 0; ui < maxUseT; ui++){
			BlockCompOutStreamUniform* curUni = (BlockCompOutStreamUniform*)(threadPass[ui]);
			//write out the compressed data (and annotation)
			uintptr_t origLen = curUni->theData.len;
			uintptr_t compLen = curUni->myComp->compData.len;
			if(compLen){
				mainF->write(curUni->myComp->compData);
				char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
				doPack.retarget(annotBuff);
					doPack.packBE64(preCompBS);
					doPack.packBE64(postCompBS);
					doPack.packBE64(origLen);
					doPack.packBE64(compLen);
				annotF->write(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			}
			//and prepare for the next round
			preCompBS += origLen;
			postCompBS += compLen;
		}
}

BlockCompOutStreamUniform::BlockCompOutStreamUniform(){
	myComp = 0;
}
BlockCompOutStreamUniform::~BlockCompOutStreamUniform(){
	if(myComp){ delete(myComp); }
}
void BlockCompOutStreamUniform::doTask(){
	myComp->compressData(theData);
}

BlockCompInStream::BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth){
	mainF = 0;
	annotF = 0;
	numMarshal = 4096;
	chunkMarshal = (char*)malloc(numMarshal);
	try{
		const char* extraPass[] = {mainFN, annotFN};
		//simple stuff
			nextBlock = 0;
			totalReads = 0;
			compThreads = 0;
			blockRemainSize = 0;
			seekOutstanding = 0;
			knowSize = 0;
		//figure out the number of blocks
			intmax_t numBlocksT = fileGetSize(annotFN);
			if(numBlocksT < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem reading annotation file.", 2, extraPass); }
			if(numBlocksT % WHODUN_BLOCKCOMP_ANNOT_ENTLEN){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Annotation file not even.", 2, extraPass); }
			numBlocks = numBlocksT / WHODUN_BLOCKCOMP_ANNOT_ENTLEN;
		//the files themselves
			mainF = new FileInStream(mainFN);
			annotF = new FileInStream(annotFN);
		//threading stuff
		{
			BlockCompInStreamUniform* curUni = new BlockCompInStreamUniform();
			curUni->myComp = compMeth->makeUnzip();
			threadPass.push_back(curUni);
		}
	}
	catch(std::exception& errE){
		if(mainF){ try{mainF->close();}catch(std::exception& errB){} delete(mainF); }
		if(annotF){ try{annotF->close();}catch(std::exception& errB){} delete(annotF); }
		free(chunkMarshal);
		isClosed = 1;
		throw;
	}
}
BlockCompInStream::BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth, uintptr_t numThreads, ThreadPool* useThreads){
	mainF = 0;
	annotF = 0;
	numMarshal = 4096*numThreads;
	chunkMarshal = (char*)malloc(numMarshal);
	try{
		const char* extraPass[] = {mainFN, annotFN};
		//simple stuff
			nextBlock = 0;
			totalReads = 0;
			compThreads = useThreads;
			blockRemainSize = 0;
			seekOutstanding = 0;
			knowSize = 0;
		//figure out the number of blocks
			intmax_t numBlocksT = fileGetSize(annotFN);
			if(numBlocksT < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem reading annotation file.", 2, extraPass); }
			if(numBlocksT % WHODUN_BLOCKCOMP_ANNOT_ENTLEN){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Annotation file not even.", 2, extraPass); }
			numBlocks = numBlocksT / WHODUN_BLOCKCOMP_ANNOT_ENTLEN;
		//the files themselves
			mainF = new FileInStream(mainFN);
			annotF = new FileInStream(annotFN);
		//threading stuff
		for(uintptr_t i = 0; i<numThreads; i++){
			BlockCompInStreamUniform* curUni = new BlockCompInStreamUniform();
			curUni->myComp = compMeth->makeUnzip();
			threadPass.push_back(curUni);
		}
	}
	catch(std::exception& errE){
		if(mainF){ try{mainF->close();}catch(std::exception& errB){} delete(mainF); }
		if(annotF){ try{annotF->close();}catch(std::exception& errB){} delete(annotF); }
		free(chunkMarshal);
		isClosed = 1;
		throw;
	}
}
BlockCompInStream::~BlockCompInStream(){
	free(chunkMarshal);
	for(uintptr_t i = 0; i<threadPass.size(); i++){
		delete(threadPass[i]);
	}
	delete(mainF);
	delete(annotF);
}
int BlockCompInStream::read(){
	char tmpLoad;
	uintptr_t numRead = read(&tmpLoad, 1);
	if(numRead){
		return 0x00FF & tmpLoad;
	}
	return -1;
}
uintptr_t BlockCompInStream::read(char* toR, uintptr_t numR){
	char* nextR = toR;
	uintptr_t leftR = numR;
	
	//if any pieces left over, handle them (seeks will drop leftovers)
		if(blockRemainSize){
			BlockCompInStreamUniform* curGrab = (BlockCompInStreamUniform*)(threadPass[blockRemainIndex]);
			if(blockRemainSize >= leftR){
				memcpy(nextR, curGrab->myComp->theData.txt + blockRemainOffset, leftR);
				blockRemainOffset += leftR;
				blockRemainSize -= leftR;
				totalReads += leftR;
				return numR;
			}
			memcpy(nextR, curGrab->myComp->theData.txt + blockRemainOffset, blockRemainSize);
			nextR += blockRemainSize;
			leftR -= blockRemainSize;
			totalReads += blockRemainSize;
			blockRemainSize = 0;
		}
	//quit if no more blocks
		doAnotherLoad:
		if(nextBlock >= numBlocks){ return numR - leftR; }
	//load in blocks until the load is satisfied
		if(seekOutstanding){ annotF->seek(WHODUN_BLOCKCOMP_ANNOT_ENTLEN*nextBlock); }
		uintptr_t numLoadBlocks = threadPass.size();
		if((nextBlock + numLoadBlocks) > numBlocks){
			numLoadBlocks = numBlocks - nextBlock;
		}
		//handle any outstanding seeks
		uintptr_t i = 0;
		uintptr_t numLoadBytes = 0;
		uintptr_t totalPostLoad = 0;
		if(seekOutstanding){
			BlockCompInStreamUniform* curGrab = (BlockCompInStreamUniform*)(threadPass[i]);
			char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
			annotF->forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			ByteUnpacker doUPack(annotBuff);
			uintmax_t precomLowA = doUPack.unpackBE64();
			uintmax_t compLowA = doUPack.unpackBE64();
			uintmax_t precomHighA = doUPack.unpackBE64();
			uintmax_t compHighA = doUPack.unpackBE64();
			uintptr_t pcLen = precomHighA - precomLowA;
			uintptr_t comLen = compHighA - compLowA;
			uintptr_t skipFirst = totalReads - precomLowA;
			uintptr_t curLeft = leftR - totalPostLoad;
			mainF->seek(compLowA);
			curGrab->theComp.txt = chunkMarshal + numLoadBytes;
			curGrab->theComp.len = comLen;
			curGrab->expectDCLen = pcLen;
			curGrab->finalTgt = nextR + totalPostLoad;
			curGrab->copyOffset = skipFirst;
			if((pcLen - skipFirst) > curLeft){
				curGrab->copyCount = curLeft;
				blockRemainIndex = i;
				blockRemainSize = (pcLen - skipFirst) - curLeft;
				blockRemainOffset = skipFirst + curLeft;
			}
			else{
				curGrab->copyCount = pcLen - skipFirst;
			}
			numLoadBytes += comLen;
			totalPostLoad += curGrab->copyCount;
			i++;
			nextBlock++;
			seekOutstanding = 0;
		}
		//handle the rest
		if(totalPostLoad < leftR){
			while(i < numLoadBlocks){
				BlockCompInStreamUniform* curGrab = (BlockCompInStreamUniform*)(threadPass[i]);
				char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
				annotF->forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
				ByteUnpacker doUPack(annotBuff);
				uintmax_t precomLowA = doUPack.unpackBE64();
				uintmax_t compLowA = doUPack.unpackBE64();
				uintmax_t precomHighA = doUPack.unpackBE64();
				uintmax_t compHighA = doUPack.unpackBE64();
				uintptr_t pcLen = precomHighA - precomLowA;
				uintptr_t comLen = compHighA - compLowA;
				uintptr_t curLeft = leftR - totalPostLoad;
				curGrab->theComp.txt = chunkMarshal + numLoadBytes;
				curGrab->theComp.len = comLen;
				curGrab->expectDCLen = pcLen;
				curGrab->finalTgt = nextR + totalPostLoad;
				curGrab->copyOffset = 0;
				if(pcLen > curLeft){
					curGrab->copyCount = curLeft;
					blockRemainIndex = i;
					blockRemainSize = pcLen - curLeft;
					blockRemainOffset = curLeft;
				}
				else{
					curGrab->copyCount = pcLen;
				}
				numLoadBytes += comLen;
				totalPostLoad += curGrab->copyCount;
				i++;
				nextBlock++;
				if(totalPostLoad >= leftR){
					break;
				}
			}
		}
	//if a bigger buffer is needed, make it
		if(numLoadBytes > numMarshal){
			char* oldStage = chunkMarshal;
			free(chunkMarshal);
			numMarshal = numLoadBytes;
			chunkMarshal = (char*)malloc(numLoadBytes);
			for(uintptr_t j = 0; j<i; j++){
				BlockCompInStreamUniform* curGrab = (BlockCompInStreamUniform*)(threadPass[j]);
				curGrab->theComp.txt = chunkMarshal + (curGrab->theComp.txt - oldStage);
			}
		}
	//load
		mainF->forceRead(chunkMarshal, numLoadBytes);
	//run
		if(compThreads){
			compThreads->addTasks(i, (JoinableThreadTask**)&(threadPass[0]));
			joinTasks(i, (JoinableThreadTask**)&(threadPass[0]));
		}
		else{
			threadPass[0]->doTask();
		}
	//update and, if necessary, run again
		nextR += totalPostLoad;
		leftR -= totalPostLoad;
		totalReads += totalPostLoad;
		if(leftR){ goto doAnotherLoad; }
		return numR;
};
void BlockCompInStream::close(){
	isClosed = 1;
	mainF->close();
	annotF->close();
}
void BlockCompInStream::seek(uintmax_t toAddr){
	if(toAddr > size()){
		throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Seek beyond end of file.", 0, 0);
	}
	//clear some state (drop leftovers)
		totalReads = toAddr;
		blockRemainSize = 0;
	//figure out which block it is in
		uintmax_t fromBlock = 0;
		uintmax_t toBlock = numBlocks;
		while(toBlock - fromBlock){
			char annotBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
			uintmax_t midBlock = (fromBlock + toBlock)/2;
			annotF->seek(WHODUN_BLOCKCOMP_ANNOT_ENTLEN*midBlock);
			annotF->forceRead(annotBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
			ByteUnpacker doUPack(annotBuff);
			uintmax_t precomLI = doUPack.unpackBE64();
			doUPack.skip(8);
			uintmax_t precomHI = precomLI + doUPack.unpackBE64();
			if(toAddr < precomHI){
				toBlock = midBlock;
			}
			else{
				fromBlock = midBlock + 1;
			}
		}
	//quit early if it is the end
		nextBlock = fromBlock;
		if(nextBlock >= numBlocks){ return; }
	//note that there is a seek outstanding
		seekOutstanding = 1;
}
uintmax_t BlockCompInStream::tell(){
	return totalReads;
}
uintmax_t BlockCompInStream::size(){
	if(knowSize){ return saveSize; }
	uintmax_t retLoc = annotF->tell();
	annotF->seek(WHODUN_BLOCKCOMP_ANNOT_ENTLEN * (numBlocks - 1));
	char tmpBuff[WHODUN_BLOCKCOMP_ANNOT_ENTLEN];
	annotF->forceRead(tmpBuff, WHODUN_BLOCKCOMP_ANNOT_ENTLEN);
	annotF->seek(retLoc);
	ByteUnpacker doUPack(tmpBuff);
		uintmax_t totalFSize = doUPack.unpackBE64();
		doUPack.skip(8);
		totalFSize += doUPack.unpackBE64();
	knowSize = 1;
	saveSize = totalFSize;
	return totalFSize;
}

BlockCompInStreamUniform::BlockCompInStreamUniform(){
	myComp = 0;
}
BlockCompInStreamUniform::~BlockCompInStreamUniform(){
	if(myComp){ delete(myComp); }
}
void BlockCompInStreamUniform::doTask(){
	myComp->expandData(theComp);
	if(myComp->theData.len != expectDCLen){
		throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Compressed block does not decompress to expected size.", 0, 0);
	}
	if(finalTgt){
		memcpy(finalTgt, myComp->theData.txt + copyOffset, copyCount);
	}
}





