#include "whodun_stat_data.h"

#include "whodun_compress.h"

namespace whodun {

/**Unpack read data.*/
class BinaryTableReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	BinaryTableReadTask();
	/**Clean up.*/
	~BinaryTableReadTask();
	void doTask();
	
	/**The number of rows this needs to mangle.*/
	uintptr_t numRow;
	/**The description of the table.*/
	DataTableDescription* tabDesc;
	/**The row(s) this needs to fill.*/
	DataTableEntry* myFill;
	/**The data this needs to fill from.*/
	char* myParse;
};

/**Pack read data.*/
class BinaryTableWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	BinaryTableWriteTask();
	/**Clean up.*/
	~BinaryTableWriteTask();
	void doTask();
	
	/**The number of rows this needs to mangle.*/
	uintptr_t numRow;
	/**The description of the table.*/
	DataTableDescription* tabDesc;
	/**The row(s) this needs to pack.*/
	DataTableEntry* myFill;
	/**The data this needs to pack to.*/
	char* myParse;
};

};

using namespace whodun;

DataTableDescription::DataTableDescription(){}
DataTableDescription::~DataTableDescription(){}

DataTable::DataTable(){}
DataTable::~DataTable(){}

DataTableReader::DataTableReader(){
	isClosed = 0;
}
DataTableReader::~DataTableReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}
void DataTableReader::getDescription(DataTableDescription* toFill){
	*toFill = tabDesc;
}

void RandacDataTableReader::readRange(DataTable* toStore, uintmax_t fromIndex, uintmax_t toIndex){
	uintmax_t numR = toIndex - fromIndex;
	seek(fromIndex);
	read(toStore, numR);
}

DataTableWriter::DataTableWriter(DataTableDescription* forData){
	isClosed = 0;
	tabDesc = *forData;
}
DataTableWriter::~DataTableWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

BinaryDataTableReader::BinaryDataTableReader(InStream* dataFile){
	tsvStr = dataFile;
	usePool = 0;
	readHeader();
	{
		BinaryTableReadTask* curT = new BinaryTableReadTask();
		curT->tabDesc = &tabDesc;
		passUnis.push_back(curT);
	}
}
BinaryDataTableReader::BinaryDataTableReader(InStream* dataFile, uintptr_t numThread, ThreadPool* mainPool){
	tsvStr = dataFile;
	usePool = mainPool;
	readHeader();
	for(uintptr_t i = 0; i<numThread; i++){
		BinaryTableReadTask* curT = new BinaryTableReadTask();
		curT->tabDesc = &tabDesc;
		passUnis.push_back(curT);
	}
}
BinaryDataTableReader::~BinaryDataTableReader(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t BinaryDataTableReader::read(DataTable* toStore, uintptr_t numRows){
	if(numRows == 0){ return 0; }
	//read the stuff in
		toStore->saveText.clear();
		toStore->saveText.resize(numRows * rowBytes);
		uintptr_t numReadB = tsvStr->read(toStore->saveText[0], numRows*rowBytes);
		if(numReadB % rowBytes){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Entry truncated.", 0, 0);
		}
		uintptr_t numRealRead = numReadB / rowBytes;
	//break it into pieces
		uintptr_t numCols = tabDesc.colTypes.size();
		toStore->saveData.clear();
		toStore->saveData.resize(numRealRead * numCols);
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numRealRead / numThread;
		uintptr_t numET = numRealRead % numThread;
		char* curRD = toStore->saveText[0];
		DataTableEntry* curRH = toStore->saveData[0];
		for(uintptr_t i = 0; i<numThread; i++){
			BinaryTableReadTask* curT = (BinaryTableReadTask*)(passUnis[i]);
			uintptr_t curNum = numPT + (i<numET);
			curT->numRow = curNum;
			curT->myFill = curRH;
			curT->myParse = curRD;
			curRD += (rowBytes * curNum);
			curRH += (numCols * curNum);
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	return numRealRead;
}
void BinaryDataTableReader::close(){
	isClosed = 1;
}
void BinaryDataTableReader::readHeader(){
	try{
		char stageB[8];
		StructVector<char> altV; //friggin string
		//number of columns
			tsvStr->forceRead(stageB, 8);
			ByteUnpacker getNCol(stageB);
			uintptr_t numCol = getNCol.unpackBE64();
			tabDesc.colTypes.resize(numCol);
			tabDesc.colNames.resize(numCol);
			tabDesc.factorColMap.resize(numCol);
			tabDesc.strLengths.resize(numCol);
		//column types
			for(uintptr_t i = 0; i<numCol; i++){
				int curType = tsvStr->read();
				if(curType < 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Header data truncated.", 0, 0); }
				if((curType == 0) || (curType > 4)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Unknown data type encountered.", 0, 0); }
				tabDesc.colTypes[i] = curType;
			}
		//names
			for(uintptr_t i = 0; i<numCol; i++){
				tsvStr->forceRead(stageB, 8);
				ByteUnpacker getSLen(stageB);
				uintptr_t curNLen = getSLen.unpackBE64();
				altV.clear(); altV.resize(curNLen);
				tsvStr->forceRead(altV[0], curNLen);
				std::string* curFillN = &(tabDesc.colNames[i]);
				curFillN->insert(curFillN->end(), altV[0], altV[curNLen]);
			}
		//categorical names
			for(uintptr_t i = 0; i<numCol; i++){
				if(tabDesc.colTypes[i] != WHODUN_DATA_CAT){ continue; }
				std::map<std::string,uintptr_t>* curFacs = &(tabDesc.factorColMap[i]);
				tsvStr->forceRead(stageB, 8);
				ByteUnpacker getNFac(stageB);
				uintptr_t curNFac = getNFac.unpackBE64();
				for(uintptr_t j = 0; j<curNFac; j++){
					tsvStr->forceRead(stageB, 8);
					ByteUnpacker getSLen(stageB);
					uintptr_t curNLen = getSLen.unpackBE64();
					altV.clear(); altV.resize(curNLen);
					tsvStr->forceRead(altV[0], curNLen);
					tsvStr->forceRead(stageB, 8);
					ByteUnpacker getFVal(stageB);
					uintptr_t curFVal = getFVal.unpackBE64();
					std::string curFacN(altV[0], altV[curNLen]);
					(*curFacs)[curFacN] = curFVal;
				}
			}
		//string lengths
			for(uintptr_t i = 0; i<numCol; i++){
				if(tabDesc.colTypes[i] != WHODUN_DATA_STR){ continue; }
				tsvStr->forceRead(stageB, 8);
				ByteUnpacker getSLen(stageB);
				uintptr_t curSLen = getSLen.unpackBE64();
				tabDesc.strLengths[i] = curSLen;
			}
		//get the offset of the actual data, the size of each row and the number of rows
			rowBytes = 0;
			for(uintptr_t i = 0; i<tabDesc.colTypes.size(); i++){
				switch(tabDesc.colTypes[i]){
				case WHODUN_DATA_CAT:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_INT:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_REAL:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_STR:{
					rowBytes += (1 + tabDesc.strLengths[i]);
				} break;
				default:
					throw std::runtime_error("Da fuq?");
				};
			}
	}
	catch(std::exception& errE){
		isClosed = 1;
		throw;
	}
}

BinaryTableReadTask::BinaryTableReadTask(){}
BinaryTableReadTask::~BinaryTableReadTask(){}
void BinaryTableReadTask::doTask(){
	uintptr_t numCol = tabDesc->colTypes.size();
	DataTableEntry* curFill = myFill;
	ByteUnpacker curData(myParse);
	for(uintptr_t ri = 0; ri<numRow; ri++){
		for(uintptr_t ci = 0; ci<numCol; ci++){
			curFill->isNA = *(curData.target) ? 1 : 0;
			curData.skip(1);
			switch(tabDesc->colTypes[ci]){
				case WHODUN_DATA_CAT:{
					curFill->valC = curData.unpackBE64();
				} break;
				case WHODUN_DATA_INT:{
					curFill->valI = curData.unpackBE64();
				} break;
				case WHODUN_DATA_REAL:{
					curFill->valR = curData.unpackBEDbl();
				} break;
				case WHODUN_DATA_STR:{
					curFill->valS = curData.target;
					curData.skip(tabDesc->strLengths[ci]);
				} break;
				default:
					throw std::runtime_error("Da fuq?");
			};
			curFill++;
		}
	}
}

BinaryRandacDataTableReader::BinaryRandacDataTableReader(RandaccInStream* dataFile){
	realRead = 0;
	try{
		tsvStr = dataFile;
		realRead = new BinaryDataTableReader(dataFile);
		baseLoc = tsvStr->tell();
		needSeek = 0;
		focusInd = 0;
		uintmax_t dataTSizeB = tsvStr->size();
			dataTSizeB = dataTSizeB - baseLoc;
		if(dataTSizeB % realRead->rowBytes){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Database file is ragged.", 0, 0);
		}
		totalNInd = dataTSizeB / realRead->rowBytes;
	}
	catch(std::exception& errE){
		if(realRead){ realRead->close(); delete(realRead); }
		isClosed = 1;
		throw;
	}
}
BinaryRandacDataTableReader::BinaryRandacDataTableReader(RandaccInStream* dataFile, uintptr_t numThread, ThreadPool* mainPool){
	realRead = 0;
	try{
		tsvStr = dataFile;
		realRead = new BinaryDataTableReader(dataFile, numThread, mainPool);
		baseLoc = tsvStr->tell();
		needSeek = 0;
		focusInd = 0;
		uintmax_t dataTSizeB = tsvStr->size();
			dataTSizeB = dataTSizeB - baseLoc;
		if(dataTSizeB % realRead->rowBytes){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Database file is ragged.", 0, 0);
		}
		totalNInd = dataTSizeB / realRead->rowBytes;
	}
	catch(std::exception& errE){
		if(realRead){ realRead->close(); delete(realRead); }
		isClosed = 1;
		throw;
	}
}
BinaryRandacDataTableReader::~BinaryRandacDataTableReader(){
	delete(realRead);
}
uintptr_t BinaryRandacDataTableReader::read(DataTable* toStore, uintptr_t numRows){
	//handle any seeking
		if(needSeek){
			if(focusInd < totalNInd){
				tsvStr->seek(focusInd*realRead->rowBytes + baseLoc);
			}
			needSeek = 0;
		}
	//figure out how many to load
		uintmax_t endIndex = focusInd + numRows;
			endIndex = std::min(endIndex, totalNInd);
		uintptr_t numRealRead = endIndex - focusInd;
	//load em, complain if short
		uintptr_t numLoaded = realRead->read(toStore, numRealRead);
		if(numLoaded != numRealRead){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Database data truncated.", 0, 0);
		}
	//and return
		focusInd = endIndex;
	return numLoaded;
}
void BinaryRandacDataTableReader::close(){
	realRead->close();
	isClosed = 1;
};
uintmax_t BinaryRandacDataTableReader::size(){
	return totalNInd;
};
void BinaryRandacDataTableReader::seek(uintmax_t index){
	if(index != focusInd){
		focusInd = index;
		needSeek = 1;
	}
}

BinaryDataTableWriter::BinaryDataTableWriter(DataTableDescription* forData, OutStream* dataFile) : DataTableWriter(forData){
	tsvStr = dataFile;
	usePool = 0;
	writeHeader();
	{
		BinaryTableWriteTask* curT = new BinaryTableWriteTask();
		curT->tabDesc = &tabDesc;
		passUnis.push_back(curT);
	}
}
BinaryDataTableWriter::BinaryDataTableWriter(DataTableDescription* forData, OutStream* dataFile, uintptr_t numThread, ThreadPool* mainPool) : DataTableWriter(forData){
	tsvStr = dataFile;
	usePool = mainPool;
	writeHeader();
	for(uintptr_t i = 0; i<numThread; i++){
		BinaryTableWriteTask* curT = new BinaryTableWriteTask();
		curT->tabDesc = &tabDesc;
		passUnis.push_back(curT);
	}
}
BinaryDataTableWriter::~BinaryDataTableWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void BinaryDataTableWriter::write(DataTable* toStore){
	uintptr_t numRows = toStore->saveData.size() / tabDesc.colTypes.size();
	//pack it up
		saveText.resize(numRows * rowBytes);
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numRows / numThread;
		uintptr_t numET = numRows % numThread;
		char* curRD = saveText[0];
		DataTableEntry* curRH = toStore->saveData[0];
		for(uintptr_t i = 0; i<numThread; i++){
			BinaryTableWriteTask* curT = (BinaryTableWriteTask*)(passUnis[i]);
			uintptr_t curNum = numPT + (i<numET);
			curT->numRow = curNum;
			curT->myFill = curRH;
			curT->myParse = curRD;
			curRD += (rowBytes * curNum);
			curRH += (tabDesc.colTypes.size() * curNum);
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//write it out
		tsvStr->write(saveText[0], numRows*rowBytes);
}
void BinaryDataTableWriter::close(){
	isClosed = 1;
}
void BinaryDataTableWriter::writeHeader(){
	try{
		char stageB[8];
		StructVector<char> altV; //friggin string
		//number of columns
			uintptr_t numCol = tabDesc.colTypes.size();
			BytePacker packNCol(stageB);
			packNCol.packBE64(numCol);
			tsvStr->write(stageB, 8);
		//column types
			for(uintptr_t i = 0; i<numCol; i++){
				int curType = tabDesc.colTypes[i];
				tsvStr->write(curType);
			}
		//names
			for(uintptr_t i = 0; i<numCol; i++){
				uintptr_t curNLen = tabDesc.colNames[i].size();
				BytePacker packSLen(stageB);
				packSLen.packBE64(curNLen);
				tsvStr->write(stageB, 8);
				tsvStr->write(tabDesc.colNames[i].c_str(), curNLen);
			}
		//categorical names
			for(uintptr_t i = 0; i<numCol; i++){
				if(tabDesc.colTypes[i] != WHODUN_DATA_CAT){ continue; }
				std::map<std::string,uintptr_t>* curFacs = &(tabDesc.factorColMap[i]);
				uintptr_t curNFac = curFacs->size();
				BytePacker packNFac(stageB);
				packNFac.packBE64(curNFac);
				tsvStr->write(stageB, 8);
				std::map<std::string,uintptr_t>::iterator facIt;
				for(facIt = curFacs->begin(); facIt != curFacs->end(); facIt++){
					uintptr_t curNLen = facIt->first.size();
					BytePacker packSLen(stageB);
					packSLen.packBE64(curNLen);
					tsvStr->write(stageB, 8);
					tsvStr->write(facIt->first.c_str(), curNLen);
					BytePacker packFVal(stageB);
					packFVal.packBE64(facIt->second);
					tsvStr->write(stageB, 8);
				}
			}
		//string lengths
			for(uintptr_t i = 0; i<numCol; i++){
				if(tabDesc.colTypes[i] != WHODUN_DATA_STR){ continue; }
				BytePacker packSLen(stageB);
				packSLen.packBE64(tabDesc.strLengths[i]);
				tsvStr->write(stageB, 8);
			}
		//get the size of each row
			rowBytes = 0;
			for(uintptr_t i = 0; i<tabDesc.colTypes.size(); i++){
				switch(tabDesc.colTypes[i]){
				case WHODUN_DATA_CAT:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_INT:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_REAL:{
					rowBytes += 9;
				} break;
				case WHODUN_DATA_STR:{
					rowBytes += (1 + tabDesc.strLengths[i]);
				} break;
				default:
					throw std::runtime_error("Da fuq?");
				};
			}
	}
	catch(std::exception& errE){
		isClosed = 1;
		throw;
	}
}

BinaryTableWriteTask::BinaryTableWriteTask(){}
BinaryTableWriteTask::~BinaryTableWriteTask(){}
void BinaryTableWriteTask::doTask(){
	BytePacker curPD(myParse);
	uintptr_t numCol = tabDesc->colTypes.size();
	DataTableEntry* curFill = myFill;
	for(uintptr_t ri = 0; ri<numRow; ri++){
		for(uintptr_t ci = 0; ci<numCol; ci++){
			*(curPD.target) = curFill->isNA ? 1 : 0;
			curPD.skip(1);
			switch(tabDesc->colTypes[ci]){
				case WHODUN_DATA_CAT:{
					curPD.packBE64(curFill->valC);
				} break;
				case WHODUN_DATA_INT:{
					curPD.packBE64(curFill->valI);
				} break;
				case WHODUN_DATA_REAL:{
					curPD.packBEDbl(curFill->valR);
				} break;
				case WHODUN_DATA_STR:{
					uintptr_t curSLen = tabDesc->strLengths[ci];
					memcpy(curPD.target, curFill->valS, curSLen);
					curPD.skip(curSLen);
				} break;
				default:
					throw std::runtime_error("Da fuq?");
			};
			curFill++;
		}
	}
}

ExtensionDataTableReader::ExtensionDataTableReader(const char* fileName, InStream* useStdin){
	openUp(fileName, 1, 0, useStdin);
	tabDesc = wrapStr->tabDesc;
}
ExtensionDataTableReader::ExtensionDataTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	openUp(fileName, numThread, mainPool, useStdin);
	tabDesc = wrapStr->tabDesc;
}
void ExtensionDataTableReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
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
			wrapStr = mainPool ? new BinaryDataTableReader(useBase, numThread, mainPool) : new BinaryDataTableReader(useBase);
			return;
		}
		//anything explicitly flat
		{
			int isTsv = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat"));
			int isTsvGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat.gz"));
			int isTsvGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat.gzip"));
			if(isTsv || isTsvGz || isTsvGzip){
				InStream* useBase;
				if(isTsv){
					useBase = new FileInStream(fileName);
				}
				else{
					useBase = new GZipInStream(fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new BinaryDataTableReader(useBase, numThread, mainPool) : new BinaryDataTableReader(useBase);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcdat"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcdat"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcdat"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcdat"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed data.", 1, packExt);
				}
				std::string bFileName(fileName); bFileName.append(".blk");
				RandaccInStream* dataS = mainPool ? new BlockCompInStream(fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new BinaryDataTableReader(dataS, numThread, mainPool) : new BinaryDataTableReader(dataS);
				delete(compMeth);
				return;
			}
		}
		//complain on anything weird
		{
			const char* packExt[] = {fileName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown file format for data table.", 1, packExt);
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
ExtensionDataTableReader::~ExtensionDataTableReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionDataTableReader::read(DataTable* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionDataTableReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}

ExtensionRandacDataTableReader::ExtensionRandacDataTableReader(const char* fileName){
	openUp(fileName, 1, 0);
	tabDesc = wrapStr->tabDesc;
}
ExtensionRandacDataTableReader::ExtensionRandacDataTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	openUp(fileName, numThread, mainPool);
	tabDesc = wrapStr->tabDesc;
}
ExtensionRandacDataTableReader::~ExtensionRandacDataTableReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionRandacDataTableReader::read(DataTable* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionRandacDataTableReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
uintmax_t ExtensionRandacDataTableReader::size(){
	return wrapStr->size();
}
void ExtensionRandacDataTableReader::seek(uintmax_t index){
	wrapStr->seek(index);
}
void ExtensionRandacDataTableReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//anything explicitly flat
		{
			int isTsv = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat"));
			if(isTsv){
				RandaccInStream* useBase = new FileInStream(fileName);
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new BinaryRandacDataTableReader(useBase, numThread, mainPool) : new BinaryRandacDataTableReader(useBase);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcdat"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcdat"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcdat"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcdat"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed data.", 1, packExt);
				}
				std::string bFileName(fileName); bFileName.append(".blk");
				RandaccInStream* dataS = mainPool ? new BlockCompInStream(fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new BinaryRandacDataTableReader(dataS, numThread, mainPool) : new BinaryRandacDataTableReader(dataS);
				delete(compMeth);
				return;
			}
		}
		//complain on anything weird
		{
			const char* packExt[] = {fileName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown file format for random access data table.", 1, packExt);
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

ExtensionDataTableWriter::ExtensionDataTableWriter(DataTableDescription* forData, const char* fileName, OutStream* useStdout) : DataTableWriter(forData){
	openUp(fileName, 1, 0, useStdout);
}
ExtensionDataTableWriter::ExtensionDataTableWriter(DataTableDescription* forData, const char* fileName, int numThread, ThreadPool* mainPool, OutStream* useStdout) : DataTableWriter(forData){
	openUp(fileName, numThread, mainPool, useStdout);
}
ExtensionDataTableWriter::~ExtensionDataTableWriter(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionDataTableWriter::write(DataTable* toStore){
	wrapStr->write(toStore);
}
void ExtensionDataTableWriter::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionDataTableWriter::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
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
			wrapStr = mainPool ? new BinaryDataTableWriter(&tabDesc, useBase, numThread, mainPool) : new BinaryDataTableWriter(&tabDesc, useBase);
			return;
		}
		//anything explicitly flat
		{
			int isTsv = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat"));
			int isTsvGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat.gz"));
			int isTsvGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bdat.gzip"));
			if(isTsv || isTsvGz || isTsvGzip){
				OutStream* useBase;
				if(isTsv){
					useBase = new FileOutStream(0,fileName);
				}
				else{
					useBase = new GZipOutStream(0,fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new BinaryDataTableWriter(&tabDesc, useBase, numThread, mainPool) : new BinaryDataTableWriter(&tabDesc, useBase);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bcdat"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bcdat"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bcdat"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bcdat"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ GZipCompressionFactory* compMethG = new GZipCompressionFactory(); compMethG->addBlockComp = 1; compMeth = compMethG; }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed data.", 1, packExt);
				}
				std::string bFileName(fileName); bFileName.append(".blk");
				OutStream* dataS = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new BinaryDataTableWriter(&tabDesc, dataS, numThread, mainPool) : new BinaryDataTableWriter(&tabDesc, dataS);
				delete(compMeth);
				return;
			}
		}
		//complain on anything weird
		{
			const char* packExt[] = {fileName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown file format for data table.", 1, packExt);
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

ArgumentOptionDataTableRead::ArgumentOptionDataTableRead(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" data.bdat");
	summary = useDesc;
	validExts.push_back(".bdat");
	validExts.push_back(".bdat.gz");
	validExts.push_back(".bdat.gzip");
	validExts.push_back(".raw.bcdat");
	//validExts.push_back(".gzip.bcdat");
	validExts.push_back(".zlib.bcdat");
}
ArgumentOptionDataTableRead::~ArgumentOptionDataTableRead(){}

ArgumentOptionDataTableRandac::ArgumentOptionDataTableRandac(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" data.bdat");
	summary = useDesc;
	validExts.push_back(".bdat");
	validExts.push_back(".raw.bcdat");
	//validExts.push_back(".gzip.bcdat");
	validExts.push_back(".zlib.bcdat");
}
ArgumentOptionDataTableRandac::~ArgumentOptionDataTableRandac(){}

ArgumentOptionDataTableWrite::ArgumentOptionDataTableWrite(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileWrite(theName){
	required = needed;
	usage = theName;
		usage.append(" data.bdat");
	summary = useDesc;
	validExts.push_back(".bdat");
	validExts.push_back(".bdat.gz");
	validExts.push_back(".bdat.gzip");
	validExts.push_back(".raw.bcdat");
	//validExts.push_back(".gzip.bcdat");
	validExts.push_back(".zlib.bcdat");
}
ArgumentOptionDataTableWrite::~ArgumentOptionDataTableWrite(){}



