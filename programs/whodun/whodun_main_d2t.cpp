#include "whodunmain_programs.h"

namespace whodun {

/**Actually do the conversion.*/
class WhodunDataToTableMainLoop : public ParallelForLoop{
public:
	/**Set up an empty.*/
	WhodunDataToTableMainLoop(uintptr_t numThread);
	/**Clean up.*/
	~WhodunDataToTableMainLoop();
	
	void doSingle(uintptr_t threadInd, uintptr_t ind);
	
	/**How the data table looks.*/
	DataTableDescription* dataLayout;
	/**The data to convert.*/
	DataTable* toConv;
	/**The place to put the conversion.*/
	TextTable* toSave;
	/**The amount of bytes between rows in the table.*/
	uintptr_t textPitch;
	/**Invert factor values to names.*/
	std::vector< std::map<uintptr_t,std::string> > invFacMap;
};

};

using namespace whodun;

WhodunDataToTableProgram::WhodunDataToTableProgram() : textHeader("--head"), textHeadSigil("--sigil"), optTabIn(0,"--in","The database to read in."), optDatOut(0,"--out","The table to write to.") {
	textHeader.summary = "Whether the table should have a header line with the column names.";
	textHeadSigil.summary = "Whether the column names have sigils attached.";
	name = "d2t";
	summary = "Convert databases to text tables.";
	version = "whodun d2t 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "d2t --in IN.bdat --out OUT.tsv";
	allOptions.push_back(&textHeader);
	allOptions.push_back(&textHeadSigil);
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optDatOut);
}
WhodunDataToTableProgram::~WhodunDataToTableProgram(){}
void WhodunDataToTableProgram::baseRun(){
	DataTableReader* inStr = 0;
	TextTableWriter* outStr = 0;
	try{
		//open the inputs
			uintptr_t numThr = optTC.value;
			ThreadPool usePool(optTC.value);
			uintptr_t chunkS = numThr * optChunky.value;
			inStr = new ExtensionDataTableReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
		//open the output
			outStr = new ExtensionTextTableWriter(optDatOut.value.c_str(), numThr, &usePool, useOut);
		//put out the header, if any
			uintptr_t numTabCols = inStr->tabDesc.colTypes.size();
			TextTable workTab;
			if(textHeader.value){
				std::vector<uintptr_t> headLens;
				std::vector<char> headText;
				for(uintptr_t i = 0; i<numTabCols; i++){
					uintptr_t startTL = headText.size();
					if(textHeadSigil.value){
						switch(inStr->tabDesc.colTypes[i]){
							case WHODUN_DATA_CAT:{
								headText.push_back('c');
								std::map<std::string,uintptr_t>* facCMap = &(inStr->tabDesc.factorColMap[i]);
								std::map<std::string,uintptr_t>::iterator facCIt = facCMap->begin();
								int needSep = 0;
								while(facCIt != facCMap->end()){
									if(needSep){ headText.push_back(31); }
									needSep = 1;
									headText.insert(headText.end(), facCIt->first.begin(), facCIt->first.end());
									facCIt++;
								}
							} break;
							case WHODUN_DATA_INT:{
								headText.push_back('i');
							} break;
							case WHODUN_DATA_REAL:{
								headText.push_back('r');
							} break;
							case WHODUN_DATA_STR:{
								char asciiBuff[8*sizeof(uintmax_t)+8];
								uintptr_t indLen = sprintf(asciiBuff, "%ju", inStr->tabDesc.strLengths[i]);
								headText.push_back('s');
								headText.insert(headText.end(), asciiBuff, asciiBuff + indLen);
							} break;
							default:
								throw std::runtime_error("Da fuq?");
						};
						headText.push_back(':');
					}
					std::string* curName = &(inStr->tabDesc.colNames[i]);
					headText.insert(headText.end(), curName->begin(), curName->end());
					headLens.push_back(headText.size() - startTL);
				}
				headText.push_back(0);
				workTab.saveRows.resize(1);
				workTab.saveStrs.resize(numTabCols);
				TextTableRow* curRow = workTab.saveRows[0];
				SizePtrString* curCols = workTab.saveStrs[0];
				char* curChars = &(headText[0]);
				curRow->numCols = numTabCols;
				curRow->texts = curCols;
				for(uintptr_t i = 0; i<numTabCols; i++){
					uintptr_t curL = headLens[i];
					curCols[i].len = curL;
					curCols[i].txt = curChars;
					curChars += curL;
				}
				outStr->write(&workTab);
			}
		//set up the conversion
			DataTable workDat;
			WhodunDataToTableMainLoop bigieDo(numThr);
			bigieDo.dataLayout = &(inStr->tabDesc);
			bigieDo.toConv = &workDat;
			bigieDo.toSave = &workTab;
			bigieDo.invFacMap.resize(numTabCols);
			uintptr_t textStrP = 0;
			for(uintptr_t i = 0; i<numTabCols; i++){
				switch(inStr->tabDesc.colTypes[i]){
					case WHODUN_DATA_CAT:{
						std::map<std::string,uintptr_t>* facCMap = &(inStr->tabDesc.factorColMap[i]);
						std::map<std::string,uintptr_t>::iterator facCIt = facCMap->begin();
						uintptr_t maxSize = 0;
						while(facCIt != facCMap->end()){
							maxSize = std::max(maxSize, (uintptr_t)(facCIt->first.size()));
							bigieDo.invFacMap[i][facCIt->second] = facCIt->first;
							facCIt++;
						}
						textStrP += maxSize;
					} break;
					case WHODUN_DATA_INT:{
						textStrP += (8*sizeof(uintmax_t)+8);
					} break;
					case WHODUN_DATA_REAL:{
						textStrP += (8*sizeof(uintmax_t)+8);
					} break;
					case WHODUN_DATA_STR:{
						textStrP += inStr->tabDesc.strLengths[i];
					} break;
					default:
						throw std::runtime_error("Da fuq?");
				};
			}
			bigieDo.textPitch = textStrP;
		//convert
			while(1){
				uintptr_t numGot = inStr->read(&workDat, chunkS);
				if(numGot == 0){ break; }
				uintptr_t numRealGot = workDat.saveData.size() / numTabCols;
				workTab.saveRows.clear(); workTab.saveRows.resize(numRealGot);
				workTab.saveStrs.clear(); workTab.saveStrs.resize(numRealGot * numTabCols);
				workTab.saveText.clear(); workTab.saveText.resize(numRealGot * textStrP);
				bigieDo.doIt(&usePool, 0, numRealGot);
				outStr->write(&workTab);
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

WhodunDataToTableMainLoop::WhodunDataToTableMainLoop(uintptr_t numThread) : ParallelForLoop(numThread){
	naturalStride = 128;
}
WhodunDataToTableMainLoop::~WhodunDataToTableMainLoop(){}
void WhodunDataToTableMainLoop::doSingle(uintptr_t threadInd, uintptr_t ind){
	uintptr_t numDatums = dataLayout->colTypes.size();
	DataTableEntry* curIn = toConv->saveData[ind * numDatums];
	TextTableRow* curOutR = toSave->saveRows[ind];
	SizePtrString* curOutS = toSave->saveStrs[ind * numDatums];
	char* curOutC = toSave->saveText[ind * textPitch];
	curOutR->numCols = numDatums;
	curOutR->texts = curOutS;
	for(uintptr_t i = 0; i<numDatums; i++){
		curOutS->txt = curOutC;
		if(curIn->isNA){
			curOutS->len = 0;
		}
		else{
			switch(dataLayout->colTypes[i]){
				case WHODUN_DATA_CAT:{
					std::map<uintptr_t,std::string>* catMap = &(invFacMap[i]);
					std::map<uintptr_t,std::string>::iterator catIt = catMap->find(curIn->valC);
					if(catIt == catMap->end()){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Unknown categoriacal index in data.", 0, 0); }
					curOutS->len = catIt->second.size();
					memcpy(curOutC, catIt->second.c_str(), curOutS->len);
				} break;
				case WHODUN_DATA_INT:{
					curOutS->len = sprintf(curOutC, "%d", (int)(curIn->valI));
				} break;
				case WHODUN_DATA_REAL:{
					curOutS->len = sprintf(curOutC, "%e", curIn->valR);
				} break;
				case WHODUN_DATA_STR:{
					curOutS->len = dataLayout->strLengths[i];
					memcpy(curOutC, curIn->valS, curOutS->len);
				} break;
				default:
					throw std::runtime_error("Da fuq?");
			};
			curOutC += curOutS->len;
		}
		curIn++;
		curOutS++;
	}
}




