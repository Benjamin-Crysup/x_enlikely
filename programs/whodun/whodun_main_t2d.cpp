#include "whodunmain_programs.h"

namespace whodun {

/**Actually do the conversion.*/
class WhodunTableToDataMainLoop : public ParallelForLoop{
public:
	/**Set up an empty.*/
	WhodunTableToDataMainLoop(uintptr_t numThread);
	/**Clean up.*/
	~WhodunTableToDataMainLoop();
	
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**How the data table looks.*/
	DataTableDescription* dataLayout;
	/**The amount of characters between entries when saving text.*/
	uintptr_t dataStrPitch;
	/**The data to convert.*/
	TextTableView* toConv;
	/**The place to put the converted data.*/
	DataTable* toSave;
	/**Storage for string data.*/
	std::vector<std::string> saveStrs;
	/**Hunt for things.*/
	StandardMemorySearcher helpFind;
};

};

using namespace whodun;

#define DEFAULT_STR_LEN 256

WhodunTableToDataProgram::WhodunTableToDataProgram() : textHeader("--head"), textHeadSigil("--sigil"), optTabIn(0,"--in","The table to read in."), optDatOut(0,"--out","The database to write to.") {
	textHeader.summary = "Whether the table has a header line with the column names.";
	textHeadSigil.summary = "Whether the column names have sigils attached.";
	name = "t2d";
	summary = "Convert text tables to databases.";
	version = "whodun t2d 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "t2d --in IN.tsv --out OUT.bdat";
	allOptions.push_back(&textHeader);
	allOptions.push_back(&textHeadSigil);
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optDatOut);
}
WhodunTableToDataProgram::~WhodunTableToDataProgram(){}
void WhodunTableToDataProgram::baseRun(){
	TextTableReader* inStr = 0;
	DataTableWriter* outStr = 0;
	try{
		//open the inputs
			uintptr_t numThr = optTC.value;
			ThreadPool usePool(optTC.value);
			uintptr_t chunkS = numThr * optChunky.value;
			inStr = new ExtensionTextTableReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
		//figure out the layout of the database
			DataTableDescription dataLayout;
			TextTable workTab;
			//get the first row
				uintptr_t numGot = inStr->read(&workTab, 1);
				while(numGot && (workTab.saveRows.size() == 0)){ numGot = inStr->read(&workTab, 1); }
			//figure it out
				int replayFirst = !(textHeader.value);
				if(numGot){
					StandardMemorySearcher helpHunt;
					TextTableRow* headRow = workTab.saveRows[0];
					dataLayout.colTypes.resize(headRow->numCols);
					dataLayout.colNames.resize(headRow->numCols);
					dataLayout.factorColMap.resize(headRow->numCols);
					dataLayout.strLengths.resize(headRow->numCols);
					for(uintptr_t i = 0; i<headRow->numCols; i++){
						if(textHeader.value && textHeadSigil.value){
							SizePtrString curName = headRow->texts[i];
							if(curName.len == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Table has header column with empty name.", 0, 0); }
							char* colonL = (char*)helpHunt.memchr(curName.txt, ':', curName.len);
							if(colonL == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Header column missing sigil.", 0, 0); }
							dataLayout.colNames[i].insert(dataLayout.colNames[i].end(), colonL + 1, curName.txt + curName.len);
							switch(curName.txt[0]){
								case 'c':{
									dataLayout.colTypes[i] = WHODUN_DATA_CAT;
									uintptr_t numCats = 0;
									char* curFoc = curName.txt;
									while(curFoc != colonL){
										curFoc++;
										char* nextSlash = (char*)helpHunt.memchr(curFoc, '\x1F', colonL - curFoc);
										if(nextSlash == 0){ nextSlash = colonL; }
										std::string curCatN(curFoc, nextSlash);
										if(curCatN.size() == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Category names cannot be empty.", 0, 0); }
										const char* packEDat[] = {curCatN.c_str()};
										if(dataLayout.factorColMap[i].find(curCatN) != dataLayout.factorColMap[i].end()){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Duplicate categorical value.", 1, packEDat); }
										dataLayout.factorColMap[i][curCatN] = numCats;
										numCats++;
										curFoc = nextSlash;
									}
									if(numCats == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Categorical sigil needs to specify possible values.", 0, 0); }
								} break;
								case 'i':{
									dataLayout.colTypes[i] = WHODUN_DATA_INT;
									if(colonL != (curName.txt+1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed sigil for integer data.", 0, 0); }
								} break;
								case 'r':{
									dataLayout.colTypes[i] = WHODUN_DATA_REAL;
									if(colonL != (curName.txt+1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Malformed sigil for real data.", 0, 0); }
								} break;
								case 's':{
									dataLayout.colTypes[i] = WHODUN_DATA_STR;
									uintptr_t numDigs = helpHunt.memspn(curName.txt + 1, curName.len - 1, "0123456789", 10);
									if(numDigs == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Sigil for string data needs a length.", 0, 0); }
									uintptr_t numChars = atol(curName.txt + 1);
									if(numChars == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "String data needs at least one byte.", 0, 0); }
									dataLayout.strLengths[i] = numChars;
								} break;
								default:
									throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Unknown sigil in header column.", 0, 0);
							};
						}
						else if(textHeader.value){
							SizePtrString curName = headRow->texts[i];
							dataLayout.colTypes[i] = WHODUN_DATA_STR;
							dataLayout.colNames[i].insert(dataLayout.colNames[i].end(), curName.txt, curName.txt + curName.len);
							dataLayout.strLengths[i] = DEFAULT_STR_LEN;
						}
						else{
							char asciiBuff[8*sizeof(uintmax_t)+8];
							sprintf(asciiBuff, "%ju", (uintmax_t)i);
							dataLayout.colTypes[i] = WHODUN_DATA_STR;
							dataLayout.colNames[i].append("data");
							dataLayout.colNames[i].append(asciiBuff);
							dataLayout.strLengths[i] = DEFAULT_STR_LEN;
						}
					}
				}
		//open the outputs
			outStr = new ExtensionDataTableWriter(&dataLayout, optDatOut.value.c_str(), numThr, &usePool, useOut);
		//set up the loop
			uintptr_t numTabCols = dataLayout.colTypes.size();
			uintptr_t dataStrP = 0;
			for(uintptr_t i = 0; i<numTabCols; i++){
				if(dataLayout.colTypes[i] == WHODUN_DATA_STR){
					dataStrP += dataLayout.strLengths[i];
				}
			}
			DataTable workDat;
			WhodunTableToDataMainLoop bigieDo(numThr);
				bigieDo.dataLayout = &dataLayout;
				bigieDo.toConv = &workTab;
				bigieDo.toSave = &workDat;
				bigieDo.dataStrPitch = dataStrP;
		//convert
			while(1){
				if(replayFirst){ replayFirst = 0; }
				else{ numGot = inStr->read(&workTab, chunkS); }
				if(numGot == 0){ break; }
				uintptr_t numRealGot = workTab.saveRows.size();
				workDat.saveData.clear(); workDat.saveData.resize(numTabCols * numRealGot);
				workDat.saveText.clear(); workDat.saveText.resize(dataStrP * numRealGot);
				bigieDo.doIt(&usePool, 0, numRealGot);
				outStr->write(&workDat);
			}
		//close
			outStr->close(); delete(outStr); outStr = 0;
			inStr->close(); delete(inStr); inStr = 0;
	}
	catch(std::exception& errE){
		if(inStr){ inStr->close(); delete(inStr); }
		if(outStr){ outStr->close(); delete(outStr); }
		throw;
	}
}

WhodunTableToDataMainLoop::WhodunTableToDataMainLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 128;
	saveStrs.resize(numThread);
}
WhodunTableToDataMainLoop::~WhodunTableToDataMainLoop(){}
void WhodunTableToDataMainLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	std::string* tmpStr = &(saveStrs[threadInd]);
	uintptr_t numDatums = dataLayout->colTypes.size();
	TextTableRow* curIn = toConv->saveRows[ind];
	DataTableEntry* curOut = toSave->saveData[ind * numDatums];
	char* curOutT = toSave->saveText[ind * dataStrPitch];
	if(curIn->numCols != numDatums){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Text table has an inconsistent number of columns.", 0, 0); }
	for(uintptr_t i = 0; i<numDatums; i++){
		SizePtrString curCell = curIn->texts[i];
		switch(dataLayout->colTypes[i]){
			case WHODUN_DATA_CAT:{
				tmpStr->clear();
				tmpStr->insert(tmpStr->end(), curCell.txt, curCell.txt + curCell.len);
				std::map<std::string,uintptr_t>::iterator foundIt = dataLayout->factorColMap[i].find(*tmpStr);
				if(foundIt == dataLayout->factorColMap[i].end()){
					curOut->isNA = 1;
					curOut->valC = 0;
				}
				else{
					curOut->isNA = 0;
					curOut->valC = foundIt->second;
				}
			} break;
			case WHODUN_DATA_INT:{
				if((curCell.len == 0) || (helpFind.memspn(curCell.txt, curCell.len, "+-0123456789", 12) != curCell.len)){
					curOut->isNA = 1;
					curOut->valI = 0;
				}
				else{
					tmpStr->clear();
					tmpStr->insert(tmpStr->end(), curCell.txt, curCell.txt + curCell.len);
					curOut->isNA = 0;
					curOut->valI = atol(tmpStr->c_str());
				}
			} break;
			case WHODUN_DATA_REAL:{
				if((curCell.len == 0) || (helpFind.memspn(curCell.txt, curCell.len, "+-0123456789.eE", 15) != curCell.len)){
					curOut->isNA = 1;
					curOut->valR = 0;
				}
				else{
					tmpStr->clear();
					tmpStr->insert(tmpStr->end(), curCell.txt, curCell.txt + curCell.len);
					curOut->isNA = 0;
					curOut->valR = atof(tmpStr->c_str());
				}
			} break;
			case WHODUN_DATA_STR:{
				uintptr_t curLen = dataLayout->strLengths[i];
				curOut->valS = curOutT;
				if((curCell.len == 0) || (curCell.len > curLen)){
					curOut->isNA = 1;
					memset(curOutT, 0, curLen);
				}
				else{
					curOut->isNA = 0;
					memcpy(curOutT, curCell.txt, curCell.len);
					memset(curOutT + curCell.len, 0, curLen - curCell.len);
				}
				curOutT += curLen;
			} break;
			default:
				throw std::runtime_error("Da fuq?");
		};
		curOut++;
	}
}




