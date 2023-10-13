#include "genlike_progs.h"

#include <math.h>
#include <iostream>

#include "whodun_sort.h"
#include "whodun_vulkan.h"
#include "whodun_stat_randoms.h"

#include "genlike_data.h"

namespace whodun {

};

using namespace whodun;

GenlikeSampleReadProbs::GenlikeSampleReadProbs() : 
	errorModel("errmod"),
	errorModelFlat("--flat-error",&errorModel),
	errorRate("--error"),
	maxReadBI("--rcb"),
	tgtGPU("--gpu"),
	maxSimLoad("--chunk"),
	seedValue("--seed"),
	drawLoadFileN("--drawL"),
	compLoadFileN("--compL"),
	finPFileN("--out"),
	glMapFileN("--mapout")
{
	name = "drawread";
	summary = "Draw reads and evaluate probabilities (flat allele P).";
	version = "genlike drawread 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "drawread --drawL DRAW.load --compL EVAL.load --out OUT.pros";
	allOptions.push_back(&errorModelFlat);
	allOptions.push_back(&errorRate);
	allOptions.push_back(&tgtGPU);
	allOptions.push_back(&maxSimLoad);
	allOptions.push_back(&seedValue);
	allOptions.push_back(&maxReadBI);
	allOptions.push_back(&drawLoadFileN);
	allOptions.push_back(&compLoadFileN);
	allOptions.push_back(&finPFileN);
	allOptions.push_back(&glMapFileN);
	
	errorModelFlat.summary = "Errors are uniform between all alleles.";
	errorRate.summary = "The rate of error in sequencing.";
	tgtGPU.summary = "The gpu index to target.";
	maxSimLoad.summary = "The number of things to do at one time.";
	seedValue.summary = "The seed to use for RNG.";
	maxReadBI.summary = "The maximum (log2) number of reads.";
	drawLoadFileN.summary = "The loadings to draw reads from.";
	compLoadFileN.summary = "The loadings to compare with for probability.";
	finPFileN.summary = "The place to write the final results.";
	glMapFileN.summary = "The place to write an index map.";
	
	errorRate.usage = "--error 0.01";
	tgtGPU.usage = "--gpu 0";
	maxSimLoad.usage = "--chunk 512";
	seedValue.usage = "--seed 1234";
	maxReadBI.usage = "--rcb 8";
	drawLoadFileN.usage = "--drawL DRAW.load";
	compLoadFileN.usage = "--compL EVAL.load";
	finPFileN.usage = "--out OUT.pros";
	glMapFileN.usage = "--mapout OUT.glmap";
	
	drawLoadFileN.required = 1;
	drawLoadFileN.validExts.push_back(".load");
	compLoadFileN.required = 1;
	compLoadFileN.validExts.push_back(".load");
	finPFileN.required = 1;
	finPFileN.validExts.push_back(".pros");
	glMapFileN.required = 1;
	glMapFileN.validExts.push_back(".glmap");
}
GenlikeSampleReadProbs::~GenlikeSampleReadProbs(){}
void GenlikeSampleReadProbs::idiotCheck(){
	if((errorRate.value < 0.0) || (errorRate.value > 1.0)){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid error rate.", 0, 0);
	}
	if(maxSimLoad.value <= 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid working set size.", 0, 0);
	}
	if(maxReadBI.value < 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid bit shift.", 0, 0);
	}
}
void GenlikeSampleReadProbs::baseRun(){
	int verbose = 1;
	//load in the evaluation set, convert to proportions
		uintptr_t numAllele;
		uintptr_t initLoad;
		std::vector<uintptr_t> hotPloidies;
		uintptr_t evalCaseFan;
		uintptr_t totalNumGenotypes;
		uintptr_t totalNumLoadings;
		double* evalCases = 0;
		HaploPloidyCollapse baseCollapse;
		std::vector<HaploPloidyCollapse> ploidCollapses;
		FileInStream evalFile(compLoadFileN.value.c_str());
		try{
			//load/convert the evaluation loadings
				uint64_t curLoad;
				char* curLoadP = (char*)(&curLoad);
				evalFile.forceRead(curLoadP, 8);
				if(curLoad != 0x0102030405060708){
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "File invalid for this machine (endianess).", 0, 0);
				}
				evalFile.forceRead(curLoadP, 8); numAllele = curLoad;
				evalFile.forceRead(curLoadP, 8); initLoad = curLoad;
				uintptr_t numPloid;
				evalFile.forceRead(curLoadP, 8); numPloid = curLoad;
				for(uintptr_t i = 0; i<numPloid; i++){
					evalFile.forceRead(curLoadP, 8);
					hotPloidies.push_back(curLoad);
				}
				evalFile.forceRead(curLoadP, 8); evalCaseFan = curLoad;
				uintptr_t totalHaplo = 0;
				for(uintptr_t i = 0; i<numPloid; i++){ totalHaplo += hotPloidies[i]; }
				HaploGenotypeSet allGeno(numAllele, totalHaplo);
				PloidyLoadingSet allLoad(hotPloidies.size(), &(hotPloidies[0]), initLoad);
				totalNumGenotypes = allGeno.hotGenos.size() / totalHaplo;
				totalNumLoadings = allLoad.hotLoads.size() / numPloid;
				uintptr_t numValue = totalNumGenotypes * totalNumLoadings * evalCaseFan;
				evalCases = (double*)malloc(numValue * numAllele * sizeof(double));
				std::vector<uint64_t> tmpSet(numAllele);
				char* tmpSetP = (char*)(&(tmpSet[0]));
				double* curSetRat = evalCases;
				for(uintptr_t i = 0; i<numValue; i++){
					evalFile.forceRead(tmpSetP, 8*numAllele);
					uint64_t totalTemps = 0;
					for(uintptr_t j = 0; j<numAllele; j++){ totalTemps += tmpSet[j]; }
					if(totalTemps == 0){
						for(uintptr_t j = 0; j<numAllele; j++){ curSetRat[j] = 1.0 / numAllele; }
					}
					else{
						for(uintptr_t j = 0; j<numAllele; j++){ curSetRat[j] = tmpSet[j] * 1.0 / totalTemps; }
					}
					curSetRat += numAllele;
				}
				evalFile.close();
			//turn the genotypes into their canonical form
				baseCollapse.figure(&allGeno, numPloid, &(hotPloidies[0]));
			//figure out how to calculate the marginals
				ploidCollapses.resize(numPloid);
				std::vector<HaploGenotypeSet> copyGenos(numPloid);
				uintptr_t ploidOff = 0;
				for(uintptr_t gi = 0; gi<numPloid; gi++){
					uintptr_t curPloid = hotPloidies[gi];
					//copy the haplotype map
						HaploGenotypeSet* copyGeno = &(copyGenos[gi]);
						copyGeno->numAl = numAllele;
						copyGeno->numH = curPloid;
						for(uintptr_t i = 0; i<baseCollapse.collapseCount.size(); i++){
							unsigned short* curCopy = &(allGeno.hotGenos[allGeno.numH * baseCollapse.collapseInds[baseCollapse.collapseOffset[i]]]);
							for(uintptr_t j = 0; j<curPloid; j++){
								copyGeno->hotGenos.push_back(curCopy[ploidOff + j]);
							}
						}
					//collapse
						ploidCollapses[gi].figure(copyGeno, 1, &curPloid);
					ploidOff += curPloid;
				}
			//output the map
				FileOutStream dumpMStr(0, glMapFileN.value.c_str());
				try{
					uint64_t curDump;
					char* curDumpP = (char*)(&curDump);
					//number of alleles
					curDump = numAllele; dumpMStr.write(curDumpP, 8);
					//number of haplotypes
					curDump = totalHaplo; dumpMStr.write(curDumpP, 8);
					//number of genotypes
					curDump = baseCollapse.collapseCount.size(); dumpMStr.write(curDumpP, 8);
					//genotypes
					for(uintptr_t i = 0; i<baseCollapse.collapseCount.size(); i++){
						uintptr_t curRepG = baseCollapse.collapseInds[baseCollapse.collapseOffset[i]];
						unsigned short* curGeno = &(allGeno.hotGenos[curRepG * totalHaplo]);
						for(uintptr_t j = 0; j<totalHaplo; j++){
							curDump = curGeno[j];
							dumpMStr.write(curDumpP, 8);
						}
					}
					//number of ploidies
					curDump = numPloid; dumpMStr.write(curDumpP, 8);
					//ploidies
					for(uintptr_t i = 0; i<numPloid; i++){
						curDump = hotPloidies[i];
						dumpMStr.write(curDumpP, 8);
					}
					//number of loadings
					curDump = totalNumLoadings; dumpMStr.write(curDumpP, 8);
					//loadings
					int* curLoadOut = &(allLoad.hotLoads[0]);
					for(uintptr_t i = 0; i<totalNumLoadings; i++){
						for(uintptr_t j = 0; j<numPloid; j++){
							curDump = *curLoadOut;
							dumpMStr.write(curDumpP, 8);
							curLoadOut++;
						}
					}
					//marginal genotypes
					for(uintptr_t gi = 0; gi<numPloid; gi++){
						HaploGenotypeSet* curGenoS = &(copyGenos[gi]);
						HaploPloidyCollapse* curCol = &(ploidCollapses[gi]);
						curDump = curCol->collapseCount.size();
						dumpMStr.write(curDumpP, 8);
						for(uintptr_t i = 0; i<curCol->collapseCount.size(); i++){
							uintptr_t curRepG = curCol->collapseInds[curCol->collapseOffset[i]];
							unsigned short* curGeno = &(curGenoS->hotGenos[curRepG * curGenoS->numH]);
							for(uintptr_t j = 0; j<curGenoS->numH; j++){
								curDump = curGeno[j];
								dumpMStr.write(curDumpP, 8);
							}
						}
					}
					dumpMStr.close();
				}
				catch(std::exception& errE){
					dumpMStr.close();
					throw;
				}
		}
		catch(std::exception& errE){
			evalFile.close();
			if(evalCases){ free(evalCases); }
			throw;
		}
	//walk down the draw set
	FileOutStream dumpOStr(0, finPFileN.value.c_str());
	try{
		//round up maxSimLoad to a multiple of 32 (draw loading cases should be a multiple)
			#define CASE_GROUP_SIZE 32
			int needRoundUp = maxSimLoad.value % CASE_GROUP_SIZE;
			maxSimLoad.value = CASE_GROUP_SIZE * (maxSimLoad.value / CASE_GROUP_SIZE);
			if(needRoundUp){ maxSimLoad.value += CASE_GROUP_SIZE; }
		//build the draw read shader
			uint32_t grpSize[] = {CASE_GROUP_SIZE,1,1};
			VulkanComputeShaderBuilder mrShad(4, grpSize);
			{
				//constants
					VulkanCSValL iZero = mrShad.conL(0);
					VulkanCSValL iOne = mrShad.conL(1);
					VulkanCSValL iNumAl = mrShad.conL(numAllele);
					VulkanCSValL randMV = mrShad.conL(0x5851F42D4C957F2D);
					VulkanCSValL randAV = mrShad.conL(0x14057B7EF767814F);
					mrShad.consDone();
				//interface (seeds, draw probabilities (taking error into account), total read count, end read counts)
					mrShad.registerInterfaceL();
					mrShad.registerInterfaceL();
					mrShad.registerInterfaceL();
					mrShad.registerInterfaceL();
				//code
				mrShad.functionStart();
					//make some variables
						std::vector<VulkanCSVarL> runTots;
						for(uintptr_t i = 0; i<numAllele; i++){
							runTots.push_back(mrShad.defVarL());
						}
						VulkanCSVarL seedVar = mrShad.defVarL();
						VulkanCSVarL itVar = mrShad.defVarL();
					//get information on the loading to draw from
						VulkanCSValL drawI = convL(mrShad.invokeGID(0));
						VulkanCSValL numRC = mrShad.interfaceVarL(2).get(iZero);
						seedVar.set(mrShad.interfaceVarL(0).get(drawI));
						std::vector<VulkanCSValL> drawThresh;
						VulkanCSValL curThrGI = drawI * iNumAl;
						VulkanCSArrL threshArr = mrShad.interfaceVarL(1);
						for(uintptr_t i = 0; i<numAllele; i++){
							drawThresh.push_back(threshArr.get(curThrGI));
							curThrGI = curThrGI + iOne;
						}
					//get ready to draw
						for(uintptr_t i = 0; i<numAllele; i++){ runTots[i].set(iZero); }
						itVar.set(iZero);
					//draw
						mrShad.doStart();
							VulkanCSValB cycleLoopVar = itVar.get() < numRC;
						mrShad.doWhile(cycleLoopVar);
							VulkanCSValL curSeedV = seedVar.get();
							VulkanCSValL curTestVal = ursh(curSeedV, iOne);
							VulkanCSValB prevWasLess = mrShad.conFalse;
							for(uintptr_t i = 0; i<numAllele; i++){
								VulkanCSValB curIsLess = uclt(curTestVal, drawThresh[i]);
								VulkanCSValB curIsAdd = curIsLess & !prevWasLess;
								runTots[i].set(runTots[i].get() + select(curIsAdd, iOne, iZero));
								prevWasLess = curIsLess;
							}
							seedVar.set(randMV*curSeedV + randAV);
						mrShad.doNext();
							itVar.set(itVar.get() + iOne);
						mrShad.doEnd();
					//move counts to final location
						VulkanCSValL curCntGI = drawI * iNumAl;
						VulkanCSArrL makeCArr = mrShad.interfaceVarL(3);
						for(uintptr_t i = 0; i<numAllele; i++){
							makeCArr.set(curCntGI, runTots[i].get());
							curCntGI = curCntGI + iOne;
						}
				mrShad.functionEnd();
			}
		//build the single loading probability shader
			VulkanComputeShaderBuilder slShad(4, grpSize);
			{
				//constants
					VulkanCSValL iZero = slShad.conL(0);
					VulkanCSValL iOne = slShad.conL(1);
					VulkanCSValL iNumAl = slShad.conL(numAllele);
					VulkanCSValL iCaseStride = slShad.conL(evalCaseFan * numAllele);
					VulkanCSValL iNumCase = slShad.conL(evalCaseFan);
					VulkanCSValL iProbStride = slShad.conL(totalNumGenotypes * totalNumLoadings);
					VulkanCSValD dZero = slShad.conD(0.0);
					VulkanCSValD dMinf = slShad.conD(-1.0 / 0.0);
					VulkanCSValD dNumCase = slShad.conD(log(evalCaseFan));
					slShad.consDone();
				//interface (read count, error matrix P(Ri|Aj), eval array, end probs)
					slShad.registerInterfaceL();
					slShad.registerInterfaceD();
					slShad.registerInterfaceD();
					slShad.registerInterfaceD();
				//code
				slShad.functionStart();
					//make some variables
						VulkanCSVarL itVar = slShad.defVarL();
						VulkanCSVarD probSum = slShad.defVarD();
					//invocation
						VulkanCSValL drawI = convL(slShad.invokeGID(0));
						VulkanCSValL loadI = convL(slShad.invokeGID(1));
					//load the read counts
						std::vector<VulkanCSValL> readCnts;
						VulkanCSArrL rcSrc = slShad.interfaceVarL(0);
						VulkanCSValL curRI = drawI * iNumAl;
						for(uintptr_t i = 0; i<numAllele; i++){
							readCnts.push_back(rcSrc.get(curRI));
							curRI = curRI + iOne;
						}
					//load the error matrix
						std::vector<VulkanCSValD> errorMat;
						VulkanCSArrD errorSrc = slShad.interfaceVarD(1);
						VulkanCSValL curEI = iZero;
						for(uintptr_t i = 0; i < numAllele; i++){
							for(uintptr_t j = 0; j<numAllele; j++){
								errorMat.push_back(errorSrc.get(curEI));
								curEI = curEI + iOne;
							}
						}
					//loop through cases
						itVar.set(iZero);
						probSum.set(dMinf);
						VulkanCSValL loadI0 = loadI * iCaseStride;
						VulkanCSArrD evalSrc = slShad.interfaceVarD(2);
						slShad.doStart();
							VulkanCSValB cycleLoopVar = itVar.get() < iNumCase;
						slShad.doWhile(cycleLoopVar);
							//load the loadings
								std::vector<VulkanCSValD> curLoads;
								VulkanCSValL curELI = loadI0 + itVar.get() * iNumAl;
								for(uintptr_t i = 0; i<numAllele; i++){
									curLoads.push_back(evalSrc.get(curELI));
									curELI = curELI + iOne;
								}
							//get the probabilities on a per-read basis
								VulkanCSValD* curEMat = &(errorMat[0]);
								std::vector<VulkanCSValD> prProbs;
								for(uintptr_t i = 0; i<numAllele; i++){
									VulkanCSValD curPro = dZero;
									for(uintptr_t j = 0; j<numAllele; j++){
										curPro = curPro + (curEMat[j] * curLoads[j]);
									}
									prProbs.push_back(curPro);
									curEMat += numAllele;
								}
							//log of the probabilities
								for(uintptr_t i = 0; i<numAllele; i++){
									prProbs[i] = vlog(prProbs[i]);
								}
							//multiply by the number of reads and sum (skip zero reads to avoid nan)
								VulkanCSValD curProPro = dZero;
								for(uintptr_t i = 0; i<numAllele; i++){
									VulkanCSValD curEntProd = convD(readCnts[i]) * prProbs[i];
									curProPro = curProPro + select(readCnts[i] > iZero, curEntProd, dZero);
								}
							//fold into probSum
								VulkanCSValD prevSum = probSum.get();
								VulkanCSValD maxOp = select(prevSum > curProPro, prevSum, curProPro);
								VulkanCSValD allGoodSum = maxOp + vlog(vexp(curProPro - maxOp) + vexp(prevSum - maxOp));
								probSum.set(select(maxOp == dMinf, dMinf, allGoodSum));
						slShad.doNext();
							itVar.set(itVar.get() + iOne);
						slShad.doEnd();
					//output final result
						slShad.interfaceVarD(3).set(drawI * iProbStride + loadI, probSum.get() - dNumCase);
				slShad.functionEnd();
			}
		//build the genotype collapse shader
			VulkanComputeShaderBuilder gcShad(1, grpSize);
			{
				//constants
					VulkanCSValL iZero = gcShad.conL(0);
					VulkanCSValL iOne = gcShad.conL(1);
					VulkanCSValL iNumLoad = gcShad.conL(totalNumLoadings);
					VulkanCSValL iNumGenoS = gcShad.conL(baseCollapse.collapseInds.size());
					VulkanCSValL iNumGenoE = gcShad.conL(baseCollapse.collapseCount.size());
					std::vector<VulkanCSValL> srcJumpInds;
					std::vector<VulkanCSValL> srcColOffs;
					uintptr_t lastBaseI = 0;
					uintptr_t curGI = 0;
					for(uintptr_t i = 0; i<baseCollapse.collapseCount.size(); i++){
						uintptr_t curCC = baseCollapse.collapseCount[i];
						uintptr_t curBI = baseCollapse.collapseInds.size();
						for(uintptr_t j = 0; j<curCC; j++){
							uintptr_t altBI = baseCollapse.collapseInds[curGI + j];
							if(altBI < curBI){ curBI = altBI; }
						}
						srcJumpInds.push_back(gcShad.conL(curBI - lastBaseI));
						for(uintptr_t j = 0; j<curCC; j++){
							srcColOffs.push_back(gcShad.conL(baseCollapse.collapseInds[curGI] - curBI));
							curGI++;
						}
						lastBaseI = curBI;
					}
					VulkanCSValD dMinf = gcShad.conD(-1.0 / 0.0);
					gcShad.consDone();
				//interface (end probs)
					gcShad.registerInterfaceD();
				//code
				gcShad.functionStart();
					//make some variables
						VulkanCSVarL itVar = gcShad.defVarL();
					//invocation
						VulkanCSValL drawI = convL(gcShad.invokeGID(0));
						VulkanCSValL curReadOff = drawI * iNumLoad * iNumGenoS;
					//loop through the loadings
						VulkanCSArrD fixArr = gcShad.interfaceVarD(0);
						itVar.set(iZero);
						gcShad.doStart();
							VulkanCSValB cycleLoopVar = itVar.get() < iNumLoad;
						gcShad.doWhile(cycleLoopVar);
							VulkanCSValL curSrcOff = curReadOff + itVar.get() * iNumGenoS;
							VulkanCSValL curDstOff = curReadOff + itVar.get() * iNumGenoE;
							curGI = 0;
							for(uintptr_t i = 0; i<baseCollapse.collapseCount.size(); i++){
								uintptr_t curCC = baseCollapse.collapseCount[i];
								curSrcOff = curSrcOff + srcJumpInds[i];
								std::vector<VulkanCSValD> curAddSet;
								for(uintptr_t j = 0; j<curCC; j++){
									curAddSet.push_back(fixArr.get(curSrcOff + srcColOffs[curGI]));
									curGI++;
								}
								VulkanCSValD maxAddSet = curAddSet[0];
								for(uintptr_t j = 1; j<curAddSet.size(); j++){
									maxAddSet = select(curAddSet[j] > maxAddSet, curAddSet[j], maxAddSet);
								}
								VulkanCSValD endSum = vexp(curAddSet[0] - maxAddSet);
								for(uintptr_t j = 1; j<curAddSet.size(); j++){
									endSum = endSum + vexp(curAddSet[j] - maxAddSet);
								}
								VulkanCSValD endValue = maxAddSet + vlog(endSum);
								endValue = select(maxAddSet == dMinf, dMinf, endValue);
								fixArr.set(curDstOff, endValue);
								curDstOff = curDstOff + iOne;
							}
						gcShad.doNext();
							itVar.set(itVar.get() + iOne);
						gcShad.doEnd();
				gcShad.functionEnd();
			}
		//build the winner picker shader
			VulkanComputeShaderBuilder wpShad(6 + hotPloidies.size(), grpSize);
			{
				//constants
					VulkanCSValL iZero = wpShad.conL(0);
					VulkanCSValL iOne = wpShad.conL(1);
					VulkanCSValL iNOne = wpShad.conL(-1);
					VulkanCSValL iNumAl = wpShad.conL(numAllele);
					VulkanCSValL iNumLoad = wpShad.conL(totalNumLoadings);
					VulkanCSValL iNumGenoS = wpShad.conL(baseCollapse.collapseInds.size());
					VulkanCSValL iNumGenoE = wpShad.conL(baseCollapse.collapseCount.size());
					VulkanCSValL iStrideResI = wpShad.conL(numAllele + 8 + hotPloidies.size());
					VulkanCSValL iStrideResD = wpShad.conL(10 + 2*hotPloidies.size());
					VulkanCSValD dMinf = wpShad.conD(-1.0 / 0.0);
					std::vector<VulkanCSValL> baseOffsets;
					std::vector<VulkanCSValL> addOffsets;
					for(uintptr_t gi = 0; gi<hotPloidies.size(); gi++){
						HaploPloidyCollapse* curColD = &(ploidCollapses[gi]);
						uintptr_t prevBaseOff = 0;
						for(uintptr_t i = 0; i<curColD->collapseCount.size(); i++){
							//ready the index
								wpShad.conL(i);
							//get the data
								uintptr_t curCC = curColD->collapseCount[i];
								uintptr_t curCIOff = curColD->collapseOffset[i];
							//get the base index
								uintptr_t curBaseOff = curColD->collapseInds.size();
								for(uintptr_t j = 0; j<curCC; j++){
									curBaseOff = std::min(curBaseOff, curColD->collapseInds[curCIOff + j]);
								}
								baseOffsets.push_back(wpShad.conL(curBaseOff - prevBaseOff));
							//and note where everything is
								for(uintptr_t j = 0; j<curCC; j++){
									addOffsets.push_back(wpShad.conL(curColD->collapseInds[curCIOff + j] - curBaseOff));
								}
							prevBaseOff = curBaseOff;
						}
					}
					wpShad.consDone();
				//interface (collapse probs, right load, right geno, read counts, fin report int, fin report dbl, right marginal geno[])
					wpShad.registerInterfaceD();
					wpShad.registerInterfaceL();
					wpShad.registerInterfaceL();
					wpShad.registerInterfaceL();
					wpShad.registerInterfaceL();
					wpShad.registerInterfaceD();
					for(uintptr_t i = 0; i<hotPloidies.size(); i++){ wpShad.registerInterfaceL(); }
				//code
				wpShad.functionStart();
					//make some variables
						VulkanCSVarL itVar = wpShad.defVarL();
						VulkanCSVarL jtVar = wpShad.defVarL();
						VulkanCSVarD probRightLoGe = wpShad.defVarD();
						VulkanCSVarD probWrongLoGe = wpShad.defVarD();
						VulkanCSVarL indlWrongLoGe = wpShad.defVarL();
						VulkanCSVarL indgWrongLoGe = wpShad.defVarL();
						VulkanCSVarD probRightLo = wpShad.defVarD();
						VulkanCSVarD probWrongLo = wpShad.defVarD();
						VulkanCSVarL indlWrongLo = wpShad.defVarL();
						VulkanCSVarD probRightGe = wpShad.defVarD();
						VulkanCSVarD probWrongGe = wpShad.defVarD();
						VulkanCSVarL indgWrongGe = wpShad.defVarL();
						VulkanCSVarD probRightGeWL = wpShad.defVarD();
						VulkanCSVarD probWrongGeWL = wpShad.defVarD();
						VulkanCSVarL indgWrongGeWL = wpShad.defVarL();
						VulkanCSVarD probRightLoWG = wpShad.defVarD();
						VulkanCSVarD probWrongLoWG = wpShad.defVarD();
						VulkanCSVarL indlWrongLoWG = wpShad.defVarL();
						VulkanCSVarD accum = wpShad.defVarD();
						VulkanCSVarD grandTot = wpShad.defVarD();
						VulkanCSVarD grandTotWL = wpShad.defVarD();
						VulkanCSVarD grandTotWG = wpShad.defVarD();
						std::vector<VulkanCSVarD> probRightMarg;
						std::vector<VulkanCSVarD> probWrongMarg;
						std::vector<VulkanCSVarL> indgWrongMarg;
						uintptr_t maxMargCol = 0;
						for(uintptr_t i = 0; i<hotPloidies.size(); i++){
							probRightMarg.push_back(wpShad.defVarD());
							probWrongMarg.push_back(wpShad.defVarD());
							indgWrongMarg.push_back(wpShad.defVarL());
							maxMargCol = std::max(maxMargCol, ploidCollapses[i].collapseCount.size());
						}
						std::vector<VulkanCSVarD> workAccums;
						for(uintptr_t i = 0; i<maxMargCol; i++){
							workAccums.push_back(wpShad.defVarD());
						}
					//invocation
						VulkanCSValL drawI = convL(wpShad.invokeGID(0));
						VulkanCSValL curReadOff = drawI * iNumLoad * iNumGenoS;
					//get the right loading and genotype indices
						VulkanCSValL rightLI = wpShad.interfaceVarL(1).get(drawI);
						VulkanCSValL rightGI = wpShad.interfaceVarL(2).get(drawI);
						std::vector<VulkanCSValL> rightMI;
						for(uintptr_t i = 0; i<hotPloidies.size(); i++){
							rightMI.push_back(wpShad.interfaceVarL(6+i).get(drawI));
						}
					//walk through the loadings
						VulkanCSArrD probArr = wpShad.interfaceVarD(0);
						probRightLoGe.set(dMinf);
						probWrongLoGe.set(dMinf);
						probRightLo.set(dMinf);
						probWrongLo.set(dMinf);
						indlWrongLoGe.set(iNOne);
						indgWrongLoGe.set(iNOne);
						indlWrongLo.set(iNOne);
						grandTot.set(dMinf);
						itVar.set(iZero);
						wpShad.doStart();
							VulkanCSValB cycleLoopVar = itVar.get() < iNumLoad;
						wpShad.doWhile(cycleLoopVar);
						{
							VulkanCSValL curI = itVar.get();
							jtVar.set(iZero);
							accum.set(dMinf);
							wpShad.doStart();
								VulkanCSValB subLoopVar = jtVar.get() < iNumGenoE;
							wpShad.doWhile(subLoopVar);
							{
								VulkanCSValL curJ = jtVar.get();
								VulkanCSValD curPro = probArr.get(curReadOff + curI * iNumGenoE + curJ);
								VulkanCSValD curWinVal = probWrongLoGe.get();
								VulkanCSValD curRightVal = probRightLoGe.get();
								//figure out if it is the best wrong or the right answer
								VulkanCSValB curIsRightLG = (curI == rightLI) & (curJ == rightGI);
								VulkanCSValB curIsWinLG = (!curIsRightLG) & (curPro > curWinVal);
								probRightLoGe.set(select(curIsRightLG, curPro, curRightVal));
								probWrongLoGe.set(select(curIsWinLG, curPro, curWinVal));
								indlWrongLoGe.set(select(curIsWinLG, curI, indlWrongLoGe.get()));
								indgWrongLoGe.set(select(curIsWinLG, curJ, indgWrongLoGe.get()));
								//add to the accumulator
								VulkanCSValD curAccVal = accum.get();
								VulkanCSValD curMaxVal = select(curPro > curAccVal, curPro, curAccVal);
								VulkanCSValD curBCSSum = curMaxVal + vlog(vexp(curPro - curMaxVal) + vexp(curAccVal - curMaxVal));
								accum.set(select(curMaxVal == dMinf, dMinf, curBCSSum));
							}
							wpShad.doNext();
								jtVar.set(jtVar.get() + iOne);
							wpShad.doEnd();
							//see if this loading wins
							VulkanCSValD curPro = accum.get();
							VulkanCSValD curWinVal = probWrongLo.get();
							VulkanCSValD curRightVal = probRightLo.get();
							VulkanCSValB curIsRightL = (curI == rightLI);
							VulkanCSValB curIsWinL = (!curIsRightL) & (curPro > curWinVal);
							probRightLo.set(select(curIsRightL, curPro, curRightVal));
							probWrongLo.set(select(curIsWinL, curPro, curWinVal));
							indlWrongLo.set(select(curIsWinL, curI, indlWrongLo.get()));
							//add to the grand total
							VulkanCSValD curAccVal = grandTot.get();
							VulkanCSValD curMaxVal = select(curPro > curAccVal, curPro, curAccVal);
							VulkanCSValD curBCSSum = curMaxVal + vlog(vexp(curPro - curMaxVal) + vexp(curAccVal - curMaxVal));
							grandTot.set(select(curMaxVal == dMinf, dMinf, curBCSSum));
						}
						wpShad.doNext();
							itVar.set(itVar.get() + iOne);
						wpShad.doEnd();
					//walk through the genotypes
						probRightGe.set(dMinf);
						probWrongGe.set(dMinf);
						indgWrongGe.set(iNOne);
						jtVar.set(iZero);
						wpShad.doStart();
							cycleLoopVar = jtVar.get() < iNumGenoE;
						wpShad.doWhile(cycleLoopVar);
						{
							VulkanCSValL curJ = jtVar.get();
							itVar.set(iZero);
							accum.set(dMinf);
							wpShad.doStart();
								VulkanCSValB subLoopVar = itVar.get() < iNumLoad;
							wpShad.doWhile(subLoopVar);
							{
								VulkanCSValL curI = itVar.get();
								VulkanCSValD curPro = probArr.get(curReadOff + curI * iNumGenoE + curJ);
								VulkanCSValD curAccVal = accum.get();
								VulkanCSValD curMaxVal = select(curPro > curAccVal, curPro, curAccVal);
								VulkanCSValD curBCSSum = curMaxVal + vlog(vexp(curPro - curMaxVal) + vexp(curAccVal - curMaxVal));
								accum.set(select(curMaxVal == dMinf, dMinf, curBCSSum));
							}
							wpShad.doNext();
								itVar.set(itVar.get() + iOne);
							wpShad.doEnd();
							//see if this loading wins
							VulkanCSValD curPro = accum.get();
							VulkanCSValD curWinVal = probWrongGe.get();
							VulkanCSValD curRightVal = probRightGe.get();
							VulkanCSValB curIsRightG = (curJ == rightGI);
							VulkanCSValB curIsWinG = (!curIsRightG) & (curPro > curWinVal);
							probRightGe.set(select(curIsRightG, curPro, curRightVal));
							probWrongGe.set(select(curIsWinG, curPro, curWinVal));
							indgWrongGe.set(select(curIsWinG, curJ, indgWrongGe.get()));
						}
						wpShad.doNext();
							jtVar.set(jtVar.get() + iOne);
						wpShad.doEnd();
					//get the P(Geno) with a known loading
						probRightGeWL.set(dMinf);
						probWrongGeWL.set(dMinf);
						indgWrongGeWL.set(iNOne);
						grandTotWL.set(dMinf);
						jtVar.set(iZero);
						wpShad.doStart();
							cycleLoopVar = jtVar.get() < iNumGenoE;
						wpShad.doWhile(cycleLoopVar);
						{
							VulkanCSValL curJ = jtVar.get();
							VulkanCSValD curPro = probArr.get(curReadOff + rightLI * iNumGenoE + curJ);
							VulkanCSValD curWinVal = probWrongGeWL.get();
							VulkanCSValD curRightVal = probRightGeWL.get();
							VulkanCSValB curIsRightLG = (curJ == rightGI);
							VulkanCSValB curIsWinLG = (!curIsRightLG) & (curPro > curWinVal);
							probRightGeWL.set(select(curIsRightLG, curPro, curRightVal));
							probWrongGeWL.set(select(curIsWinLG, curPro, curWinVal));
							indgWrongGeWL.set(select(curIsWinLG, curJ, indgWrongGeWL.get()));
							VulkanCSValD curAccVal = grandTotWL.get();
							VulkanCSValD curMaxVal = select(curPro > curAccVal, curPro, curAccVal);
							VulkanCSValD curBCSSum = curMaxVal + vlog(vexp(curPro - curMaxVal) + vexp(curAccVal - curMaxVal));
							grandTotWL.set(select(curMaxVal == dMinf, dMinf, curBCSSum));
						}
						wpShad.doNext();
							jtVar.set(jtVar.get() + iOne);
						wpShad.doEnd();
					//get the P(Load) with a known genotype
						probRightLoWG.set(dMinf);
						probWrongLoWG.set(dMinf);
						indlWrongLoWG.set(iNOne);
						grandTotWG.set(dMinf);
						itVar.set(iZero);
						wpShad.doStart();
							cycleLoopVar = itVar.get() < iNumLoad;
						wpShad.doWhile(cycleLoopVar);
						{
							VulkanCSValL curI = itVar.get();
							VulkanCSValD curPro = probArr.get(curReadOff + curI * iNumGenoE + rightGI);
							VulkanCSValD curWinVal = probWrongLoWG.get();
							VulkanCSValD curRightVal = probRightLoWG.get();
							VulkanCSValB curIsRightLG = (curI == rightLI);
							VulkanCSValB curIsWinLG = (!curIsRightLG) & (curPro > curWinVal);
							probRightLoWG.set(select(curIsRightLG, curPro, curRightVal));
							probWrongLoWG.set(select(curIsWinLG, curPro, curWinVal));
							indlWrongLoWG.set(select(curIsWinLG, curI, indlWrongLoWG.get()));
							VulkanCSValD curAccVal = grandTotWG.get();
							VulkanCSValD curMaxVal = select(curPro > curAccVal, curPro, curAccVal);
							VulkanCSValD curBCSSum = curMaxVal + vlog(vexp(curPro - curMaxVal) + vexp(curAccVal - curMaxVal));
							grandTotWG.set(select(curMaxVal == dMinf, dMinf, curBCSSum));
						}
						wpShad.doNext();
							itVar.set(itVar.get() + iOne);
						wpShad.doEnd();
					//get the marginals
						uintptr_t curBOWorkI = 0;
						uintptr_t curAOWorkI = 0;
						for(uintptr_t gi = 0; gi<hotPloidies.size(); gi++){
							HaploPloidyCollapse* curColD = &(ploidCollapses[gi]);
							for(uintptr_t i = 0; i<curColD->collapseCount.size(); i++){
								workAccums[i].set(dMinf);
							}
							//loop through the loadings
							itVar.set(iZero);
							wpShad.doStart();
								cycleLoopVar = itVar.get() < iNumLoad;
							wpShad.doWhile(cycleLoopVar);
							{
								VulkanCSValL curI = itVar.get();
								VulkanCSValL nextReadI = curReadOff + curI * iNumGenoE;
								for(uintptr_t i = 0; i<curColD->collapseCount.size(); i++){
									//get data
										uintptr_t curCC = curColD->collapseCount[i];
										nextReadI = nextReadI + baseOffsets[curBOWorkI]; curBOWorkI++;
									//get the probs
										std::vector<VulkanCSValD> tmpGrabs;
										for(uintptr_t j = 0; j<curCC; j++){
											tmpGrabs.push_back(probArr.get(nextReadI + addOffsets[curAOWorkI])); curAOWorkI++;
										}
									//find the max
										VulkanCSValD maxGrab = tmpGrabs[0];
										for(uintptr_t j = 1; j<curCC; j++){
											maxGrab = select(tmpGrabs[j] > maxGrab, tmpGrabs[j], maxGrab);
										}
									//add em up
										VulkanCSValD totGrab = vexp(tmpGrabs[0] - maxGrab);
										for(uintptr_t j = 1; j<curCC; j++){
											totGrab = totGrab + vexp(tmpGrabs[j] - maxGrab);
										}
										totGrab = maxGrab + vlog(totGrab);
										totGrab = select(maxGrab == dMinf, dMinf, totGrab);
									//and add to the accumulator
										VulkanCSValD accumVal = workAccums[i].get();
										VulkanCSValD maxVal = select(totGrab > accumVal, totGrab, accumVal);
										VulkanCSValD endAcc = maxVal + vlog(vexp(totGrab - maxVal) + vexp(accumVal - maxVal));
										endAcc = select(maxVal == dMinf, dMinf, endAcc);
										workAccums[i].set(endAcc);
								}
							}
							wpShad.doNext();
								itVar.set(itVar.get() + iOne);
							wpShad.doEnd();
							//note the winner and the wronger
							VulkanCSValD curRightP = dMinf;
							VulkanCSValD curWrongP = dMinf;
							VulkanCSValL indWrongP = iNOne;
							for(uintptr_t i = 0; i<curColD->collapseCount.size(); i++){
								VulkanCSValD curPro = workAccums[i].get();
								VulkanCSValB curIsRightLG = (rightMI[gi] == wpShad.conL(i));
								VulkanCSValB curIsWinLG = (!curIsRightLG) & (curPro > curWrongP);
								curRightP = select(curIsRightLG, curPro, curRightP);
								curWrongP = select(curIsWinLG, curPro, curWrongP);
								indWrongP = select(curIsWinLG, wpShad.conL(i), indWrongP);
							}
							probRightMarg[gi].set(curRightP);
							probWrongMarg[gi].set(curWrongP);
							indgWrongMarg[gi].set(indWrongP);
						}
					//set the final result
						VulkanCSArrL rcArr = wpShad.interfaceVarL(3);
						VulkanCSArrL resIArr = wpShad.interfaceVarL(4);
						VulkanCSArrD resDArr = wpShad.interfaceVarD(5);
						VulkanCSValL resII = drawI * iStrideResI;
						VulkanCSValL resDI = drawI * iStrideResD;
						//copy read counts
						VulkanCSValL rcI = drawI * iNumAl;
						for(uintptr_t i = 0; i<numAllele; i++){
							resIArr.set(resII, rcArr.get(rcI));
							resII = resII + iOne;
							rcI = rcI + iOne;
						}
						//save indices
						resIArr.set(resII, rightLI); resII = resII + iOne;
						resIArr.set(resII, rightGI); resII = resII + iOne;
						resIArr.set(resII, indlWrongLoGe.get()); resII = resII + iOne;
						resIArr.set(resII, indgWrongLoGe.get()); resII = resII + iOne;
						resIArr.set(resII, indlWrongLo.get()); resII = resII + iOne;
						resIArr.set(resII, indgWrongGe.get()); resII = resII + iOne;
						resIArr.set(resII, indgWrongGeWL.get()); resII = resII + iOne;
						resIArr.set(resII, indlWrongLoWG.get());
						for(uintptr_t i = 0; i<hotPloidies.size(); i++){
							resII = resII + iOne;
							resIArr.set(resII, indgWrongMarg[i].get());
						}
						//save probabilities
						VulkanCSValD denom = grandTot.get();
						VulkanCSValD denomWL = grandTotWL.get();
						VulkanCSValD denomWG = grandTotWG.get();
						resDArr.set(resDI, probRightLoGe.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probWrongLoGe.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probRightLo.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probWrongLo.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probRightGe.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probWrongGe.get() - denom); resDI = resDI + iOne;
						resDArr.set(resDI, probRightGeWL.get() - denomWL); resDI = resDI + iOne;
						resDArr.set(resDI, probWrongGeWL.get() - denomWL); resDI = resDI + iOne;
						resDArr.set(resDI, probRightLoWG.get() - denomWG); resDI = resDI + iOne;
						resDArr.set(resDI, probWrongLoWG.get() - denomWG);
						for(uintptr_t i = 0; i<hotPloidies.size(); i++){
							resDI = resDI + iOne;
							resDArr.set(resDI, probRightMarg[i].get() - denom);
							resDI = resDI + iOne;
							resDArr.set(resDI, probWrongMarg[i].get() - denom);
						}
				wpShad.functionEnd();
			}
		/*
		{
			FileOutStream testSO(0, "tmp_shaderA");
			testSO.write((char*)(&(mrShad.mainBuild->spirOps[0])), 4*mrShad.mainBuild->spirOps.size());
			testSO.close();
		}
		{
			FileOutStream testSO(0, "tmp_shaderB");
			testSO.write((char*)(&(slShad.mainBuild->spirOps[0])), 4*slShad.mainBuild->spirOps.size());
			testSO.close();
		}
		{
			FileOutStream testSO(0, "tmp_shaderC");
			testSO.write((char*)(&(gcShad.mainBuild->spirOps[0])), 4*gcShad.mainBuild->spirOps.size());
			testSO.close();
		}
		{
			FileOutStream testSO(0, "tmp_shaderD");
			testSO.write((char*)(&(wpShad.mainBuild->spirOps[0])), 4*wpShad.mainBuild->spirOps.size());
			testSO.close();
		}
		*/
		//get vulkan
			VulkanInstance vulkI("genlike_sampload");
				vulkI.toWarn = useErr;
				vulkI.wantLayers.push_back("VK_LAYER_KHRONOS_validation");
			vulkI.create();
		//make the package spec
			VulkanComputePackageDescription doPack;
			//input buffers: seeds draw_loads(cdf) right_load_ind right_geno_ind read_count right_marginal_geno[]
				doPack.inputBufferSizes.push_back(maxSimLoad.value * 8);
				doPack.inputBufferSizes.push_back(maxSimLoad.value * numAllele * 8);
				doPack.inputBufferSizes.push_back(maxSimLoad.value * 8);
				doPack.inputBufferSizes.push_back(maxSimLoad.value * 8);
				doPack.inputBufferSizes.push_back(8);
				for(uintptr_t i = 0; i<hotPloidies.size(); i++){ doPack.inputBufferSizes.push_back(maxSimLoad.value * 8); }
			//data buffers: eval_loads error_mat
				doPack.dataBufferSizes.push_back(totalNumGenotypes * totalNumLoadings * evalCaseFan * numAllele * 8);
				doPack.dataBufferSizes.push_back(numAllele * numAllele * 8);
			//side buffers: read_counts ind_load_probs(reuse for collapse_load_probs)
				doPack.sideBufferSizes.push_back(maxSimLoad.value * numAllele * 8);
				doPack.sideBufferSizes.push_back(maxSimLoad.value * totalNumGenotypes * totalNumLoadings * 8);
			//output buffers: fin_report_int fin_report_dbl
				doPack.outputBufferSizes.push_back(maxSimLoad.value * (numAllele + 8 + hotPloidies.size()) * 8);
				doPack.outputBufferSizes.push_back(maxSimLoad.value * (10 + 2*hotPloidies.size()) * 8);
			//staging
				doPack.naturalStageSize = 8 * 0x010000;
			//programs
				doPack.allPrograms.push_back(&mrShad);
				doPack.progMaxSimulBind.push_back(1);
				doPack.allPrograms.push_back(&slShad);
				doPack.progMaxSimulBind.push_back(1);
				doPack.allPrograms.push_back(&gcShad);
				doPack.progMaxSimulBind.push_back(1);
				doPack.allPrograms.push_back(&wpShad);
				doPack.progMaxSimulBind.push_back(1);
		//and compile
			VulkanComputePackage* gotPack = doPack.prepare(&vulkI, tgtGPU.value);
		//run the reads
		try{
			//set up a run call
				VulkanComputeProgramCallSpec doSinRun;
					doSinRun.longhandSteps.resize(4);
					//run a draw
					VulkanComputeSingleStepCallSpec* sinRunS0 = &(doSinRun.longhandSteps[0]);
						sinRunS0->programIndex = 0;
						doSinRun.numX.push_back(maxSimLoad.value / CASE_GROUP_SIZE);
						doSinRun.numY.push_back(1);
						doSinRun.numZ.push_back(1);
						//seeds
							sinRunS0->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
							sinRunS0->assignIndices.push_back(0);
							sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//draw probability cdfs
							sinRunS0->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
							sinRunS0->assignIndices.push_back(1);
							sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//number of reads to draw
							sinRunS0->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
							sinRunS0->assignIndices.push_back(4);
							sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//output read counts
							sinRunS0->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS0->assignIndices.push_back(0);
							sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUT);
					//calculate probabilities on a per loading basis
					VulkanComputeSingleStepCallSpec* sinRunS1 = &(doSinRun.longhandSteps[1]);
						sinRunS1->programIndex = 1;
						doSinRun.numX.push_back(maxSimLoad.value / CASE_GROUP_SIZE);
						doSinRun.numY.push_back(totalNumGenotypes * totalNumLoadings);
						doSinRun.numZ.push_back(1);
						//read count
							sinRunS1->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS1->assignIndices.push_back(0);
							sinRunS1->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//error matrix
							sinRunS1->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_DATA);
							sinRunS1->assignIndices.push_back(1);
							sinRunS1->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//evaluation loadings
							sinRunS1->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_DATA);
							sinRunS1->assignIndices.push_back(0);
							sinRunS1->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//output probabilities
							sinRunS1->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS1->assignIndices.push_back(1);
							sinRunS1->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
					//collapse equivalent loadings
					VulkanComputeSingleStepCallSpec* sinRunS2 = &(doSinRun.longhandSteps[2]);
						sinRunS2->programIndex = 2;
						doSinRun.numX.push_back(maxSimLoad.value / CASE_GROUP_SIZE);
						doSinRun.numY.push_back(1);
						doSinRun.numZ.push_back(1);
						//work over probabilities
							sinRunS2->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS2->assignIndices.push_back(1);
							sinRunS2->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INOUT);
					//pick the winners and report
					VulkanComputeSingleStepCallSpec* sinRunS3 = &(doSinRun.longhandSteps[3]);
						sinRunS3->programIndex = 3;
						doSinRun.numX.push_back(maxSimLoad.value / CASE_GROUP_SIZE);
						doSinRun.numY.push_back(1);
						doSinRun.numZ.push_back(1);
						//probabilities
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS3->assignIndices.push_back(1);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//right loading
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
							sinRunS3->assignIndices.push_back(2);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//right genotype
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
							sinRunS3->assignIndices.push_back(3);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//read counts
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_SIDE);
							sinRunS3->assignIndices.push_back(0);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
						//report integers
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT);
							sinRunS3->assignIndices.push_back(0);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUT);
						//report doubles
							sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT);
							sinRunS3->assignIndices.push_back(1);
							sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUT);
						//right genotype (marginal)
							for(uintptr_t i = 0; i<hotPloidies.size(); i++){
								sinRunS3->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INPUT);
								sinRunS3->assignIndices.push_back(5+i);
								sinRunS3->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN);
							}
				doSinRun.prepare(gotPack);
			//upload the evaluation loadings
			{
				VulkanComputeGPUBuffer evalAcc;
				gotPack->openDataBuffer(0, &evalAcc);
				evalAcc.write(0, totalNumGenotypes * totalNumLoadings * evalCaseFan * numAllele * 8, (char*)evalCases);
			}
			//calculate the error matrix and upload
			std::vector<double> errorMatrix(numAllele * numAllele);
			{
				if(errorModelFlat.value()){
					pcrSequencerModelFlat(numAllele, errorRate.value, &(errorMatrix[0]));
				}
				else{
					throw std::runtime_error("Da fuq?");
				}
				VulkanComputeGPUBuffer errAcc;
				gotPack->openDataBuffer(1, &errAcc);
				errAcc.write(0, numAllele * numAllele * 8, (char*)&(errorMatrix[0]));
			}
			//run through the draw loadings
				VulkanComputeGPUBuffer seedAcc;
				VulkanComputeGPUBuffer loadAcc;
				VulkanComputeGPUBuffer rloadIAcc;
				VulkanComputeGPUBuffer rgenoIAcc;
				VulkanComputeGPUBuffer readCAcc;
				std::vector<VulkanComputeGPUBuffer> rmargIAccs(hotPloidies.size());
				gotPack->openInputBuffer(0, &seedAcc);
				gotPack->openInputBuffer(1, &loadAcc);
				gotPack->openInputBuffer(2, &rloadIAcc);
				gotPack->openInputBuffer(3, &rgenoIAcc);
				gotPack->openInputBuffer(4, &readCAcc);
				for(uintptr_t i = 0; i<hotPloidies.size(); i++){
					gotPack->openInputBuffer(5+i, &(rmargIAccs[i]));
				}
				VulkanComputeGPUBuffer resIAcc;
				VulkanComputeGPUBuffer resDAcc;
				gotPack->openOutputBuffer(0, &resIAcc);
				gotPack->openOutputBuffer(1, &resDAcc);
			FileInStream drawFile(drawLoadFileN.value.c_str());
			try{
				//idiot check the header
					uint64_t curLoad;
					char* curLoadP = (char*)(&curLoad);
					drawFile.forceRead(curLoadP, 8);
					if(curLoad != 0x0102030405060708){
						throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "File invalid for this machine (endianess).", 0, 0);
					}
					drawFile.forceRead(curLoadP, 8); if(curLoad != numAllele){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Read loadings have different number of alleles than evaluation loadings.", 0, 0); }
					drawFile.forceRead(curLoadP, 8); if(curLoad != initLoad){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Read loadings have different number of starting templates than evaluation loadings.", 0, 0); }
					drawFile.forceRead(curLoadP, 8); if(curLoad != hotPloidies.size()){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Read loadings have different number of contributors than evaluation loadings.", 0, 0); }
					for(uintptr_t i = 0; i<hotPloidies.size(); i++){
						drawFile.forceRead(curLoadP, 8);
						if(curLoad != hotPloidies[i]){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Read loadings contributor has different ploidy than evaluation loadings.", 0, 0); }
					}
					uintptr_t drawCaseFan;
					drawFile.forceRead(curLoadP, 8); drawCaseFan = curLoad;
				//start reading the crap
					uintptr_t curLoadI = 0;
					uintptr_t curGenoI = 0;
					uintptr_t curCaseI = 0;
					uint64_t tmpDumpI; char* tmpDumpIP = (char*)&tmpDumpI;
					StructVector<uint64_t> tmpLoad; tmpLoad.resize(numAllele);
					StructVector<double> tmpDProb; tmpDProb.resize(numAllele);
					StructVector<uint64_t> tmpThresh; tmpThresh.resize(numAllele);
					uintptr_t intNumB = (numAllele + 8 + hotPloidies.size()) * 8;
					uintptr_t dblNumB = (10 + 2*hotPloidies.size()) * 8;
					StructVector<char> tmpOutStore; tmpOutStore.resize(8 * (numAllele + 10 + 2*hotPloidies.size()));
					MersenneTwisterGenerator doRNG; doRNG.seedI(seedValue.value);
					while(curLoadI < totalNumLoadings){
						//load some crap
						uintptr_t numToRun = 0;
						VulkanComputeGPUBufferOutStream seedOut(&seedAcc, 0, maxSimLoad.value * 8);
						VulkanComputeGPUBufferOutStream loadOut(&loadAcc, 0, maxSimLoad.value * numAllele * 8);
						VulkanComputeGPUBufferOutStream rloadIOut(&rloadIAcc, 0, maxSimLoad.value * 8);
						VulkanComputeGPUBufferOutStream rgenoIOut(&rgenoIAcc, 0, maxSimLoad.value * 8);
						std::vector<VulkanComputeGPUBufferOutStream*> rmargIOuts;
						for(uintptr_t i = 0; i<hotPloidies.size(); i++){
							rmargIOuts.push_back(new VulkanComputeGPUBufferOutStream(&(rmargIAccs[i]), 0, maxSimLoad.value * 8));
						}
						try{
							while(curLoadI < totalNumLoadings){
								while(curGenoI < totalNumGenotypes){
									while(curCaseI < drawCaseFan){
										//load it in
											drawFile.forceRead((char*)(tmpLoad[0]), 8*numAllele);
											pcrSequenceCalcDrawProb(numAllele, &(errorMatrix[0]), tmpLoad[0], tmpDProb[0], tmpThresh[0]);
											loadOut.write((char*)(tmpThresh[0]), numAllele * 8);
										//upload seed and right indices
											doRNG.draw(8, tmpDumpIP); seedOut.write(tmpDumpIP, 8);
											tmpDumpI = curLoadI; rloadIOut.write(tmpDumpIP, 8);
											tmpDumpI = baseCollapse.collapseMap[curGenoI]; rgenoIOut.write(tmpDumpIP, 8);
											for(uintptr_t i = 0; i<hotPloidies.size(); i++){
												tmpDumpI = ploidCollapses[i].collapseMap[baseCollapse.collapseMap[curGenoI]];
												rmargIOuts[i]->write(tmpDumpIP, 8);
											}
										//move along
											curCaseI++;
											numToRun++;
											if(numToRun == (uintptr_t)(maxSimLoad.value)){ goto run_a_bitch; }
									}
									curCaseI = 0;
									curGenoI++;
								}
								curGenoI = 0;
								curLoadI++;
							}
							
							run_a_bitch:
							seedOut.close();
							loadOut.close();
							rloadIOut.close();
							rgenoIOut.close();
							for(uintptr_t i = 0; i<hotPloidies.size(); i++){ rmargIOuts[i]->close(); delete(rmargIOuts[i]); }
						}
						catch(std::exception& errE){
							seedOut.close();
							loadOut.close();
							rloadIOut.close();
							rgenoIOut.close();
							for(uintptr_t i = 0; i<hotPloidies.size(); i++){ rmargIOuts[i]->close(); delete(rmargIOuts[i]); }
							throw;
						}
						
						//output status
						if(verbose){
							double curProgress = (drawCaseFan * (totalNumGenotypes * curLoadI + curGenoI) + curCaseI) * 100.0 / (totalNumLoadings * totalNumGenotypes * drawCaseFan);
							char statusDumpBuff[7+3*(8*sizeof(uintmax_t)+8)];
							sprintf(statusDumpBuff, "%d%%", (int)curProgress);
							const char* passSDB = statusDumpBuff;
							useErr->logError(WHODUN_ERROR_LEVEL_DEBUG, WHODUN_ERROR_SDESC_UPDATE, __FILE__, __LINE__, "Progress", 1, &(passSDB));
						}
						
						//run and output
						if(numToRun){
							uintptr_t curRC = 1 << maxReadBI.value;
							while(curRC){
								//set it
									tmpDumpI = curRC;
									readCAcc.write(0, 8, tmpDumpIP);
								//run it
									doSinRun.numX[0] = numToRun / CASE_GROUP_SIZE;
									doSinRun.numX[1] = numToRun / CASE_GROUP_SIZE;
									doSinRun.numX[2] = numToRun / CASE_GROUP_SIZE;
									doSinRun.numX[3] = numToRun / CASE_GROUP_SIZE;
									gotPack->run(&doSinRun);
									gotPack->waitProgram();
								//dump it
									VulkanComputeGPUBufferInStream doReadRI(&resIAcc, 0, numToRun * intNumB);
									VulkanComputeGPUBufferInStream doReadRD(&resDAcc, 0, numToRun * dblNumB);
									try{
										for(uintptr_t i = 0; i<numToRun; i++){
											doReadRI.forceRead(tmpOutStore[0], intNumB);
											dumpOStr.write(tmpOutStore[0], intNumB);
											doReadRD.forceRead(tmpOutStore[0], dblNumB);
											dumpOStr.write(tmpOutStore[0], dblNumB);
										}
										doReadRI.close();
										doReadRD.close();
									}
									catch(std::exception& errE){
										doReadRI.close();
										doReadRD.close();
										throw;
									}
								//do it again
									curRC = curRC >> 1;
							}
						}
					}
				drawFile.close();
			}
			catch(std::exception& errE){
				drawFile.close();
				throw;
			}
		}
		catch(std::exception& errE){
			delete(gotPack);
			throw;
		}
		//clean up
		delete(gotPack);
		dumpOStr.close();
	}
	catch(std::exception& errE){
		dumpOStr.close();
		throw;
	}
}



