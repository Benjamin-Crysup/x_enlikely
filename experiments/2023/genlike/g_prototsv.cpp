#include "genlike_progs.h"

#include <math.h>
#include <iostream>

#include "whodun_sort.h"
#include "whodun_stat_randoms.h"

#include "genlike_data.h"

using namespace whodun;

GenlikeReadProbToTsv::GenlikeReadProbToTsv() : 
	finPFileN("--in"),
	glMapFileN("--map"),
	outFileN(1, "--out", "The file to write to.")
{
	name = "prototsv";
	summary = "Dump out read probability results as tsv.";
	version = "genlike prototsv 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "prototsv --in READ.pros --map LG.glmap --out DUMP.tsv";
	allOptions.push_back(&finPFileN);
	allOptions.push_back(&glMapFileN);
	allOptions.push_back(&outFileN);
	
	finPFileN.summary = "The read probability results.";
	glMapFileN.summary = "The corresponding index map.";
	
	finPFileN.usage = "--in READ.pros";
	glMapFileN.usage = "--map LG.glmap";
	
	finPFileN.required = 1;
	finPFileN.validExts.push_back(".pros");
	glMapFileN.required = 1;
	glMapFileN.validExts.push_back(".glmap");
}
GenlikeReadProbToTsv::~GenlikeReadProbToTsv(){}
void GenlikeReadProbToTsv::baseRun(){
	char asciiBuff[8*sizeof(uintmax_t)+8];
	//load in the map
		uintptr_t numAllele;
		uintptr_t totalHaplo;
		std::vector<char> saveGenoText;
		std::vector<SizePtrString> allGenos;
		uintptr_t numPloid;
		std::vector<uintptr_t> hotPloidies;
		std::vector<char> saveLoadText;
		std::vector<SizePtrString> allLoads;
		std::vector<char> saveMargText;
		std::vector<SizePtrString> allMargs;
		std::vector<uintptr_t> margContOffsets;
		FileInStream mapIn(glMapFileN.value.c_str());
		try{
			uint64_t curLoadI;
			char* curLoadIP = (char*)&curLoadI;
			mapIn.forceRead(curLoadIP, 8); numAllele = curLoadI;
			//load genotype information
				mapIn.forceRead(curLoadIP, 8); totalHaplo = curLoadI;
				uintptr_t numGenotype;
				mapIn.forceRead(curLoadIP, 8); numGenotype = curLoadI;
				std::vector<uintptr_t> genoTxtStartIs;
				for(uintptr_t i = 0; i<numGenotype; i++){
					genoTxtStartIs.push_back(saveGenoText.size());
					for(uintptr_t j = 0; j<totalHaplo; j++){
						if(j){ saveGenoText.push_back('/'); }
						mapIn.forceRead(curLoadIP, 8);
						uintptr_t textLen = sprintf(asciiBuff, "%ld", (long)curLoadI);
						saveGenoText.insert(saveGenoText.end(), asciiBuff, asciiBuff + textLen);
					}
				}
				genoTxtStartIs.push_back(saveGenoText.size());
			//load loading information
				mapIn.forceRead(curLoadIP, 8); numPloid = curLoadI;
				for(uintptr_t i = 0; i<numPloid; i++){
					mapIn.forceRead(curLoadIP, 8);
					hotPloidies.push_back(curLoadI);
				}
				uintptr_t numLoading;
				mapIn.forceRead(curLoadIP, 8); numLoading = curLoadI;
				std::vector<uintptr_t> loadTxtStartIs;
				for(uintptr_t i = 0; i<numLoading; i++){
					loadTxtStartIs.push_back(saveLoadText.size());
					for(uintptr_t j = 0; j<numPloid; j++){
						if(j){ saveLoadText.push_back('/'); }
						mapIn.forceRead(curLoadIP, 8);
						uintptr_t textLen = sprintf(asciiBuff, "%ld", (long)curLoadI);
						saveLoadText.insert(saveLoadText.end(), asciiBuff, asciiBuff + textLen);
					}
				}
				loadTxtStartIs.push_back(saveLoadText.size());
			//marginal genotype information
				std::vector<uintptr_t> margTxtStartIs;
				for(uintptr_t gi = 0; gi<numPloid; gi++){
					margContOffsets.push_back(margTxtStartIs.size());
					uintptr_t curPloid = hotPloidies[gi];
					mapIn.forceRead(curLoadIP, 8);
					uintptr_t curNumG = curLoadI;
					for(uintptr_t i = 0; i<curNumG; i++){
						margTxtStartIs.push_back(saveMargText.size());
						for(uintptr_t j = 0; j<curPloid; j++){
							if(j){ saveMargText.push_back('/'); }
							mapIn.forceRead(curLoadIP, 8);
							uintptr_t textLen = sprintf(asciiBuff, "%ld", (long)curLoadI);
							saveMargText.insert(saveMargText.end(), asciiBuff, asciiBuff + textLen);
						}
					}
				}
				margTxtStartIs.push_back(saveMargText.size());
			//patch up the strings
				for(uintptr_t i = 1; i<genoTxtStartIs.size(); i++){
					SizePtrString curEnt;
					curEnt.txt = &(saveGenoText[genoTxtStartIs[i-1]]);
					curEnt.len = genoTxtStartIs[i] - genoTxtStartIs[i-1];
					allGenos.push_back(curEnt);
				}
				for(uintptr_t i = 1; i<loadTxtStartIs.size(); i++){
					SizePtrString curEnt;
					curEnt.txt = &(saveLoadText[loadTxtStartIs[i-1]]);
					curEnt.len = loadTxtStartIs[i] - loadTxtStartIs[i-1];
					allLoads.push_back(curEnt);
				}
				for(uintptr_t i = 1; i<margTxtStartIs.size(); i++){
					SizePtrString curEnt;
					curEnt.txt = &(saveMargText[margTxtStartIs[i-1]]);
					curEnt.len = margTxtStartIs[i] - margTxtStartIs[i-1];
					allMargs.push_back(curEnt);
				}
			mapIn.close();
		}
		catch(std::exception& errE){
			mapIn.close();
			throw;
		}
	//start dumping
		uintptr_t maxRChunk = 0x010000;
		ExtensionTextTableWriter tsvOut(outFileN.value.c_str(), useOut);
		try{
			//output a header
			{
				TextTable saveTabD;
					saveTabD.saveRows.resize(1);
					saveTabD.saveStrs.resize(numAllele + 18 + 3*numPloid);
					saveTabD.saveText.resize((numAllele + numPloid)*(8*sizeof(long)+16));
				char* curFill = saveTabD.saveText[0];
				SizePtrString* curFStr = saveTabD.saveStrs[0];
				saveTabD.saveRows[0]->numCols = numAllele + 18 + 3*numPloid;
				saveTabD.saveRows[0]->texts = curFStr;
				for(uintptr_t i = 0; i<numAllele; i++){
					uintptr_t curALen = sprintf(curFill, "RC%ld", (long)i);
					curFStr->txt = curFill;
					curFStr->len = curALen;
					curFill += curALen;
					curFStr++;
				}
				curFStr[0] = toSizePtr("RightLoading");
				curFStr[1] = toSizePtr("RightGenotype");
				curFStr[2] = toSizePtr("WrongJointLoading");
				curFStr[3] = toSizePtr("WrongJointGenotype");
				curFStr[4] = toSizePtr("WrongLoading");
				curFStr[5] = toSizePtr("WrongGenotype");
				curFStr[6] = toSizePtr("WrongGenotype|Load");
				curFStr[7] = toSizePtr("WrongLoading|Geno");
				curFStr[8] = toSizePtr("lnPJointRight");
				curFStr[9] = toSizePtr("lnPJointWrong");
				curFStr[10] = toSizePtr("lnPLoadRight");
				curFStr[11] = toSizePtr("lnPLoadWrong");
				curFStr[12] = toSizePtr("lnPGenoRight");
				curFStr[13] = toSizePtr("lnPGenoWrong");
				curFStr[14] = toSizePtr("lnPGenoRight|Load");
				curFStr[15] = toSizePtr("lnPGenoWrong|Load");
				curFStr[16] = toSizePtr("lnPLoadRight|Geno");
				curFStr[17] = toSizePtr("lnPLoadWrong|Geno");
				curFStr += 18;
				for(uintptr_t i = 0; i<numPloid; i++){
					#define DUMP_MARG_HEADER(form) \
						curFStr->len = sprintf(curFill, (form), (long)i);\
						curFStr->txt = curFill;\
						curFill += curFStr->len;\
						curFStr++;
					DUMP_MARG_HEADER("WrongG%ld")
					DUMP_MARG_HEADER("lnPRightG%ld")
					DUMP_MARG_HEADER("lnPWrongG%ld")
				}
				tsvOut.write(&saveTabD);
			}
			//run through the read probabilities
			FileInStream readpStr(finPFileN.value.c_str());
			try{
				//storage space
					uintptr_t saveIDNB = 8*(numAllele + 8 + numPloid);
					uintptr_t saveRDNB = 8*(10 + 2*numPloid);
					StructVector<uint64_t> saveIData; saveIData.resize(numAllele + 8 + numPloid);
					StructVector<double> saveDData; saveDData.resize(10 + 2*numPloid);
					uint64_t* saveIDataP = saveIData[0];
					double* saveDDataP = saveDData[0];
					TextTable saveTabD;
						saveTabD.saveRows.resize(maxRChunk);
						saveTabD.saveStrs.resize(maxRChunk * (numAllele + 18 + 3*numPloid));
						saveTabD.saveText.resize(maxRChunk * ((numAllele)*8*sizeof(long) + (10+2*numPloid)*16*sizeof(double)));
				//start walking
				uintptr_t numLoaded = maxRChunk;
				while(numLoaded == maxRChunk){
					//load and prep
					char* curFill = saveTabD.saveText[0];
					SizePtrString* curFStr = saveTabD.saveStrs[0];
					TextTableRow* curFRow = saveTabD.saveRows[0];
					numLoaded = 0;
					while(numLoaded < maxRChunk){
						uintptr_t curPLen;
						#define PACK_UP_INTEGER(toPack) \
							curPLen = sprintf(curFill, "%ld", (long)(toPack));\
							curFStr->len = curPLen;\
							curFStr->txt = curFill;\
							curFill += curPLen;\
							curFStr++;
						#define PACK_UP_DOUBLE(toPack) \
							curPLen = sprintf(curFill, "%e", (toPack));\
							curFStr->len = curPLen;\
							curFStr->txt = curFill;\
							curFill += curPLen;\
							curFStr++;
						//load it
							uintptr_t curNumR = readpStr.read((char*)saveIDataP, saveIDNB);
							if(curNumR == 0){ break; }
							if(curNumR != saveIDNB){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Truncated file.", 0, 0); }
							readpStr.forceRead((char*)saveDDataP, saveRDNB);
							curFRow->numCols = numAllele + 18 + 3*numPloid;
							curFRow->texts = curFStr;
						//read counts
							for(uintptr_t i = 0; i<numAllele; i++){
								PACK_UP_INTEGER(saveIDataP[i])
							}
						//loading/genotype markers
							SizePtrString nullPass = {0,0};
							#define SAFETY_DANCE(fromVec,useInd) (((useInd) < (fromVec).size()) ? (fromVec)[useInd] : nullPass)
							*curFStr = SAFETY_DANCE(allLoads,saveIDataP[numAllele+0]); curFStr++;
							*curFStr = SAFETY_DANCE(allGenos,saveIDataP[numAllele+1]); curFStr++;
							*curFStr = SAFETY_DANCE(allLoads,saveIDataP[numAllele+2]); curFStr++;
							*curFStr = SAFETY_DANCE(allGenos,saveIDataP[numAllele+3]); curFStr++;
							*curFStr = SAFETY_DANCE(allLoads,saveIDataP[numAllele+4]); curFStr++;
							*curFStr = SAFETY_DANCE(allGenos,saveIDataP[numAllele+5]); curFStr++;
							*curFStr = SAFETY_DANCE(allGenos,saveIDataP[numAllele+6]); curFStr++;
							*curFStr = SAFETY_DANCE(allLoads,saveIDataP[numAllele+7]); curFStr++;
						//probabilities
							for(uintptr_t i = 0; i<10; i++){
								PACK_UP_DOUBLE(saveDDataP[i])
							}
						//marginals
							for(uintptr_t i = 0; i<numPloid; i++){
								*curFStr = SAFETY_DANCE(allMargs,saveIDataP[numAllele+8+i]+margContOffsets[i]); curFStr++;
								PACK_UP_DOUBLE(saveDDataP[10+2*i])
								PACK_UP_DOUBLE(saveDDataP[10+2*i+1])
							}
						//move along
							numLoaded++;
							curFRow++;
					}
					//dump
					saveTabD.saveRows.resize(numLoaded);
					tsvOut.write(&saveTabD);
				}
				readpStr.close();
			}
			catch(std::exception& errE){
				readpStr.close();
				throw;
			}
			tsvOut.close();
		}
		catch(std::exception& errE){
			tsvOut.close();
			throw;
		}
}

