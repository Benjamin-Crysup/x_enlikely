#include "genlike_data.h"

#include "whodun_sort.h"

namespace whodun{

/**Compare genotypes given index.*/
class GenlikeHaploCollapseCompGenos : public PODComparator{
public:
	~GenlikeHaploCollapseCompGenos();
	uintptr_t itemSize();
	bool compare(const void* itemA, const void* itemB);
	/**The genotypes to look at.*/
	HaploGenotypeSet* allGeno;
};

};

using namespace whodun;

void whodun::loadMatrix(InStream* toRead, DenseMatrix* toFill){
	char workBuff[8];
	ByteUnpacker workUP(workBuff);
	uintptr_t numR;
		toRead->forceRead(workBuff, 8);
		workUP.retarget(workBuff);
		numR = workUP.unpackBE64();
	uintptr_t numC;
		toRead->forceRead(workBuff, 8);
		workUP.retarget(workBuff);
		numC = workUP.unpackBE64();
	toFill->recap(numR, numC);
	double* curFill = toFill->matVs;
	uintptr_t totNR = numR * numC;
	for(uintptr_t i = 0; i<totNR; i++){
		toRead->forceRead(workBuff, 8);
		workUP.retarget(workBuff);
		curFill[i] = workUP.unpackBEDbl();
	}
}

void whodun::loadMatrixTable(TextTableReader* toRead, DenseMatrix* toFill){
	StandardMemorySearcher helpCut;
	uintptr_t numCols = 0;
	std::vector<double> hotVals;
	TextTable curChunk;
	uintptr_t numGot = toRead->read(&curChunk, 1024);
	while(numGot){
		for(uintptr_t i = 0; i<curChunk.saveRows.size(); i++){
			TextTableRow* curRow = curChunk.saveRows[i];
			if(curRow->numCols){
				if(numCols == 0){
					numCols = curRow->numCols;
				}
				else if(curRow->numCols != numCols){
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Number of columns inconsistent.", 0, 0);
				}
				for(uintptr_t j = 0; j<numCols; j++){
					hotVals.push_back(parseFloat(helpCut.trim(curRow->texts[j])));
				}
			}
		}
		numGot = toRead->read(&curChunk, 1024);
	}
	if(numCols == 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Empty matrix.", 0, 0);
	}
	if(hotVals.size() % numCols){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Ragged matrix.", 0, 0);
	}
	toFill->recap(hotVals.size() % numCols, numCols);
	memcpy(toFill->matVs, &hotVals[0], hotVals.size() * sizeof(double));
}

void whodun::saveMatrix(OutStream* toRead, DenseMatrix* toFill){
	char workBuff[8];
	BytePacker workP;
	workP.retarget(workBuff);
		workP.packBE64(toFill->nr);
		toRead->write(workBuff, 8);
	workP.retarget(workBuff);
		workP.packBE64(toFill->nc);
		toRead->write(workBuff, 8);
	double* curFill = toFill->matVs;
	uintptr_t totNR = toFill->nr * toFill->nc;
	for(uintptr_t i = 0; i<totNR; i++){
		workP.retarget(workBuff);
		workP.packBEDbl(curFill[i]);
		toRead->write(workBuff, 8);
	}
}

void whodun::parsePloidies(SizePtrString toParse, std::vector<uintptr_t>* savePloids){
	StandardMemorySearcher doCut;
	SizePtrString curLeft = toParse;
	while(curLeft.len){
		std::pair<SizePtrString,SizePtrString> curSpl = doCut.split(curLeft, toSizePtr("/"));
		savePloids->push_back(parseInt(doCut.trim(curSpl.first)));
		curLeft = curSpl.second;
	}
}

HaploGenotypeSet::HaploGenotypeSet(){}
HaploGenotypeSet::HaploGenotypeSet(uintptr_t numAllele, uintptr_t numHaplo){
	numAl = numAllele;
	numH = numHaplo;
	std::vector<unsigned short> curAdd(numHaplo);
	while(1){
		hotGenos.insert(hotGenos.end(), curAdd.begin(), curAdd.end());
		uintptr_t curAI = curAdd.size();
		
		FixItAgainTony:
		if(curAI == 0){ break; }
		curAI--;
		uintptr_t curAl = curAdd[curAI];
		curAl++;
		if(curAl < numAllele){ curAdd[curAI] = curAl; continue; }
		curAl = 0;
		curAdd[curAI] = curAl;
		goto FixItAgainTony;
	}
}
HaploGenotypeSet::~HaploGenotypeSet(){}

PloidyLoadingSet::PloidyLoadingSet(uintptr_t numPloid, uintptr_t* thePloids, uintptr_t totalLoad){
	hotPloids.insert(hotPloids.end(), thePloids, thePloids + numPloid);
	std::vector<int> hotAss(numPloid);
	std::map<uintptr_t,uintptr_t> highAss; //(the highest possible assignment for each ploidy)
	for(uintptr_t i = 0; i<numPloid; i++){
		highAss[thePloids[i]] = totalLoad;
	}
	fillOutPloids(0, &(hotAss[0]), totalLoad, &highAss);
}
PloidyLoadingSet::~PloidyLoadingSet(){}
void PloidyLoadingSet::fillOutPloids(uintptr_t nextPI, int* hotAss, uintptr_t loadLeft, std::map<uintptr_t,uintptr_t>* highAss){
	if(nextPI == hotPloids.size()){
		if(loadLeft){ return; }
		hotLoads.insert(hotLoads.end(), hotAss, hotAss + hotPloids.size());
		return;
	}
	uintptr_t curPloid = hotPloids[nextPI];
	std::map<uintptr_t,uintptr_t>::iterator highIt = highAss->find(curPloid);
	uintptr_t oldHigh = highIt->second;
	uintptr_t maxAss = loadLeft / curPloid;
		maxAss = std::min(maxAss, oldHigh);
	uintptr_t ci = maxAss + 1;
	while(ci){
		ci--;
		hotAss[nextPI] = ci;
		highIt->second = ci;
		fillOutPloids(nextPI+1, hotAss, loadLeft - (ci*curPloid), highAss);
	}
	highIt->second = oldHigh;
}

HaploPloidyCollapse::HaploPloidyCollapse(){}
HaploPloidyCollapse::~HaploPloidyCollapse(){}
void HaploPloidyCollapse::figure(HaploGenotypeSet* toCollapse, uintptr_t numPloid, uintptr_t* thePloids){
	//make a copy
		HaploGenotypeSet* copyCol = toCollapse;
	//turn everything in that copy to its canonical form
		PODLessComparator<unsigned short> fixupComp;
		PODSortOptions fixupOpts(&fixupComp);
		PODInMemoryMergesort fixupSort(&fixupOpts);
		unsigned short* curFixUp = &(copyCol->hotGenos[0]);
		uintptr_t totalNumGenotypes = copyCol->hotGenos.size() / copyCol->numH;
		for(uintptr_t i = 0; i<totalNumGenotypes; i++){
			for(uintptr_t j = 0; j<numPloid; j++){
				uintptr_t curPloid = thePloids[j];
				if(curPloid > 1){ fixupSort.sort(curPloid, (char*)curFixUp); }
				curFixUp += curPloid;
			}
			collapseInds.push_back(i);
		}
	//sort them
		GenlikeHaploCollapseCompGenos sortGI;
			sortGI.allGeno = copyCol;
		PODSortOptions sortGIOpts(&sortGI);
		PODInMemoryMergesort sortGISort(&sortGIOpts);
		sortGISort.sort(totalNumGenotypes, (char*)&(collapseInds[0]));
	//figure out how to collapse
		uintptr_t* curLookInd = &(collapseInds[0]);
		uintptr_t* endLookInd = curLookInd + collapseInds.size();
		while(curLookInd != endLookInd){
			PODSortedDataChunk curSDC(&sortGIOpts, (char*)curLookInd, endLookInd - curLookInd);
			uintptr_t upBndInd = curSDC.upperBound((char*)curLookInd);
			collapseCount.push_back(upBndInd);
			curLookInd += upBndInd;
		}
	//set up the map
		collapseMap.resize(collapseInds.size());
		uintptr_t curLookGI = 0;
		for(uintptr_t i = 0; i<collapseCount.size(); i++){
			collapseOffset.push_back(curLookGI);
			uintptr_t curCC = collapseCount[i];
			for(uintptr_t j = 0; j<curCC; j++){
				collapseMap[collapseInds[curLookGI]] = i;
				curLookGI++;
			}
		}
}

void whodun::genotypeLoadingToAlleleLoading(uintptr_t numAl, uint64_t* alLoad, uintptr_t numPloid, uintptr_t* thePloids, int* ploidLoads, unsigned short* curGeno){
	memset(alLoad, 0, numAl * sizeof(uint64_t));
	int* curPL = ploidLoads;
	unsigned short* curHG = curGeno;
	for(uintptr_t i = 0; i<numPloid; i++){
		uintptr_t curPloid = thePloids[i];
		int curL = *curPL;
		for(uintptr_t j = 0; j<curPloid; j++){
			alLoad[*curHG] += curL;
			curHG++;
		}
		curPL++;
	}
}

PCRGPUShaderBuilder::PCRGPUShaderBuilder(){}
PCRGPUShaderBuilder::~PCRGPUShaderBuilder(){}
uintptr_t PCRGPUShaderBuilder::numBuffers(){
	subStepBuffOff.clear();
	uintptr_t curOff = 3;
	for(uintptr_t i = 0; i<subSteps.size(); i++){
		subStepBuffOff.push_back(curOff);
		curOff += subSteps[i]->numSiteBuffers();
	}
	return curOff;
}
void PCRGPUShaderBuilder::addConstants(){
	//RNG as done by Knuth (MMIX)
		randMV = shad->conL(0x5851F42D4C957F2D);
		randAV = shad->conL(0x14057B7EF767814F);
	numSPL = shad->conL(caseFan);
	numAL = shad->conL(numAllele);
	for(uintptr_t i = 0; i<subSteps.size(); i++){
		subSteps[i]->addConstants(this);
	}
	shad->conL(1);
}
void PCRGPUShaderBuilder::addInterface(){
	//register the initial loading, site and seed buffers
		shad->registerInterfaceL();
		shad->registerInterfaceL();
		shad->registerInterfaceL();
	//let the steps add their thing
		for(uintptr_t i = 0; i<subSteps.size(); i++){
			subSteps[i]->addInterface(this);
		}
}
void PCRGPUShaderBuilder::addVariables(){
	//working loading and seed variables
		seedVar = shad->defVarL();
		for(uintptr_t i = 0; i<numAllele; i++){
			countVariables.push_back(shad->defVarL());
		}
	//let the steps do their thing
		for(uintptr_t i = 0; i<subSteps.size(); i++){
			subSteps[i]->addVariables(this);
		}
}
void PCRGPUShaderBuilder::addCode(uintptr_t sideBuffOff){
	//get the relevant indices
		loadInd = convL(shad->invokeGID(0));
		caseInd = convL(shad->invokeGID(1));
	//load the site
		siteIV = shad->interfaceVarL(1).get(loadInd);
	//load the seed into a variable
		VulkanCSValL seedStartV = shad->interfaceVarL(2).get(caseInd + loadInd*numSPL);
		seedVar.set(seedStartV);
	//load the loadings into counts
		VulkanCSValL loadGInd = numAL * loadInd;
		VulkanCSArrL loadArrse = shad->interfaceVarL(0);
		for(uintptr_t i = 0; i<countVariables.size(); i++){
			countVariables[i].set(loadArrse.get(loadGInd));
			loadGInd = loadGInd + shad->conL(1);
		}
	//let the steps do their thing
		for(uintptr_t i = 0; i<subSteps.size(); i++){
			subSteps[i]->addCode(this, subStepBuffOff[i] + sideBuffOff);
		}
}
std::pair<uintptr_t,uintptr_t> PCRGPUShaderBuilder::addBuffer(VulkanComputePackageDescription* toAdd, uintptr_t buffIndex){
	std::pair<uintptr_t,uintptr_t> toRet;
	uintptr_t needSize;
	switch(buffIndex){
		case 0:
			//initial loading
			needSize = maxSimulLoad * numAllele * 8;
			toRet.first = WHODUN_VULKAN_COMPUTE_BUFFER_INPUT;
			toRet.second = toAdd->inputBufferSizes.size();
			toAdd->inputBufferSizes.push_back(needSize);
			return toRet;
		case 1:
			//site index
			needSize = maxSimulLoad * 8;
			toRet.first = WHODUN_VULKAN_COMPUTE_BUFFER_INPUT;
			toRet.second = toAdd->inputBufferSizes.size();
			toAdd->inputBufferSizes.push_back(needSize);
			return toRet;
		case 2:
			//seed value
			needSize = maxSimulLoad * caseFan * 8;
			toRet.first = WHODUN_VULKAN_COMPUTE_BUFFER_INPUT;
			toRet.second = toAdd->inputBufferSizes.size();
			toAdd->inputBufferSizes.push_back(needSize);
			return toRet;
		default:
			//one of the subs
			uintptr_t csi = subSteps.size();
			while(csi){
				csi--;
				if(buffIndex >= subStepBuffOff[csi]){
					needSize = numSite * subSteps[csi]->siteBufferSize(this, buffIndex - subStepBuffOff[csi]);
					toRet.first = WHODUN_VULKAN_COMPUTE_BUFFER_DATA;
					toRet.second = toAdd->dataBufferSizes.size();
					toAdd->dataBufferSizes.push_back(needSize);
					return toRet;
				}
			}
			throw std::runtime_error("Da fuq?");
	};
}

PCRSimulationStepSpec::PCRSimulationStepSpec(){}
PCRSimulationStepSpec::~PCRSimulationStepSpec(){}

PCRAmplificationStepSpec::PCRAmplificationStepSpec(){
	maxNumMake = 0;
}
PCRAmplificationStepSpec::~PCRAmplificationStepSpec(){}
uintptr_t PCRAmplificationStepSpec::numSiteBuffers(){
	return 2;
}
uintptr_t PCRAmplificationStepSpec::siteBufferSize(PCRGPUShaderBuilder* toBuild, uintptr_t siteInd){
	if(siteInd == 0){
		//efficiency
		return 8 * toBuild->numAllele;
	}
	else{
		//mutation
		return 8 * toBuild->numAllele * toBuild->numAllele;
	}
}
void PCRAmplificationStepSpec::addConstants(PCRGPUShaderBuilder* toBuild){
	toBuild->shad->conL(0);
	toBuild->shad->conL(1);
	toBuild->shad->conD(0.0);
	numCycCV = toBuild->shad->conL(numCycles);
	if(maxNumMake >= 0){ initReactV = toBuild->shad->conL(maxNumMake); }
	//TODO
}
void PCRAmplificationStepSpec::addInterface(PCRGPUShaderBuilder* toBuild){
	toBuild->shad->registerInterfaceL();
	toBuild->shad->registerInterfaceL();
}
void PCRAmplificationStepSpec::addVariables(PCRGPUShaderBuilder* toBuild){
	cycleIndV = toBuild->shad->defVarL();
	cycleJV = toBuild->shad->defVarL();
	for(uintptr_t i = 0; i<toBuild->numAllele; i++){
		copyCounts.push_back(toBuild->shad->defVarL());
		madeCounts.push_back(toBuild->shad->defVarL());
	}
	if(maxNumMake >= 0){ reactRemainV = toBuild->shad->defVarL(); }
}
void PCRAmplificationStepSpec::addCode(PCRGPUShaderBuilder* toBuild, uintptr_t sideBuffOff){
	VulkanCSValL setZero = toBuild->shad->conL(0);
	VulkanCSValL addOne = toBuild->shad->conL(1);
	cycleIndV.set(setZero);
	
	uintptr_t numAllele = toBuild->numAllele;
	
	//load the relevant efficiency and mutation
	std::vector<VulkanCSValL> hotEffs;
	std::vector<VulkanCSValL> hotMuts;
	VulkanCSValL effBaseI = toBuild->siteIV * toBuild->numAL;
	VulkanCSArrL effArray = toBuild->shad->interfaceVarL(sideBuffOff + 0);
	for(uintptr_t i = 0; i<numAllele; i++){
		hotEffs.push_back(effArray.get(effBaseI));
		effBaseI = effBaseI + addOne;
	}
	VulkanCSValL mutBaseI = toBuild->siteIV * toBuild->numAL * toBuild->numAL;
	VulkanCSArrL mutArray = toBuild->shad->interfaceVarL(sideBuffOff + 1);
	for(uintptr_t i = 0; i<numAllele; i++){
		for(uintptr_t j = 0; j<numAllele; j++){
			hotMuts.push_back(mutArray.get(mutBaseI));
			mutBaseI = mutBaseI + addOne;
		}
	}
	
	//set up the reactant watch
	if(maxNumMake >= 0){
		reactRemainV.set(initReactV);
	}
	
	//run through the cycles
	toBuild->shad->doStart();
		VulkanCSValB cycleLoopVar = cycleIndV.get() < numCycCV;
	toBuild->shad->doWhile(cycleLoopVar);
	{
		//figure out how many new things get made
		for(uintptr_t i = 0; i<numAllele; i++){
			VulkanCSValL initNumC = toBuild->countVariables[i].get();
			VulkanCSValL curEff = hotEffs[i];
			if(maxNumMake >= 0){
				if(maxNumMake == 0){
					curEff = setZero;
				}
				else{
					curEff = convL(convD(curEff) * (convD(reactRemainV.get()) / convD(initReactV)));
				}
			}
			VulkanCSVarL curCopyVar = copyCounts[i];
			curCopyVar.set(setZero);
			cycleJV.set(setZero);
			toBuild->shad->doStart();
				VulkanCSValB copyLoopVar = cycleJV.get() < initNumC;
			toBuild->shad->doWhile(copyLoopVar);
				VulkanCSValL curSeedV = toBuild->seedVar.get();
				VulkanCSValB curItAdd = uclt(ursh(curSeedV, addOne),curEff);
				curCopyVar.set(curCopyVar.get() + select(curItAdd, addOne, setZero));
				toBuild->seedVar.set(toBuild->randMV*curSeedV + toBuild->randAV);
			toBuild->shad->doNext();
				cycleJV.set(cycleJV.get() + addOne);
			toBuild->shad->doEnd();
		}
		//figure out what they turn into
		for(uintptr_t i = 0; i<numAllele; i++){
			madeCounts[i].set(setZero);
		}
		for(uintptr_t i = 0; i<numAllele; i++){
			VulkanCSValL copyNumC = copyCounts[i].get();
			VulkanCSValL* curMuts = &(hotMuts[numAllele*i]);
			cycleJV.set(setZero);
			toBuild->shad->doStart();
				VulkanCSValB makeLoopVar = cycleJV.get() < copyNumC;
			toBuild->shad->doWhile(makeLoopVar);
				VulkanCSValL curSeedV = toBuild->seedVar.get();
				VulkanCSValL curTestVal = ursh(curSeedV, addOne);
				VulkanCSValB prevWasLess = toBuild->shad->conFalse;
				for(uintptr_t j = 0; j<numAllele; j++){
					VulkanCSValB curIsLess = uclt(curTestVal, curMuts[j]);
					VulkanCSValB curIsAdd = curIsLess & !prevWasLess;
					madeCounts[j].set(madeCounts[j].get() + select(curIsAdd, addOne, setZero));
					prevWasLess = curIsLess;
				}
				toBuild->seedVar.set(toBuild->randMV*curSeedV + toBuild->randAV);
			toBuild->shad->doNext();
				cycleJV.set(cycleJV.get() + addOne);
			toBuild->shad->doEnd();
		}
		//update the variables
		for(uintptr_t i = 0; i<numAllele; i++){
			toBuild->countVariables[i].set(toBuild->countVariables[i].get() + madeCounts[i].get());
		}
		if(maxNumMake > 0){
			VulkanCSValL totalCopyM = copyCounts[0].get();
			for(uintptr_t i = 1; i<numAllele; i++){
				totalCopyM = totalCopyM + copyCounts[i].get();
			}
			VulkanCSValL endNumReact = reactRemainV.get();
			endNumReact = select(endNumReact > totalCopyM, endNumReact - totalCopyM, setZero);
			reactRemainV.set(endNumReact);
		}
	}
	toBuild->shad->doNext();
		cycleIndV.set(cycleIndV.get() + addOne);
	toBuild->shad->doEnd();
}


void whodun::pcrEfficiencyModelFlat(uintptr_t numAl, double baseEff, uint64_t* toFill){
	uint64_t curEffPack = (uint64_t)(0x8000000000000000 * baseEff);
	for(uintptr_t i = 0; i<numAl; i++){
		toFill[i] = curEffPack;
	}
}

void whodun::pcrMutationModelFlat(uintptr_t numAl, double mutRate, uint64_t* toFill){
	double offProb = mutRate / (numAl - 1);
	double mainProb = 1 - mutRate;
	double totProb = mainProb; for(uintptr_t i = 1; i<numAl; i++){ totProb += offProb; }
	uint64_t* curFill = toFill;
	for(uintptr_t i = 0; i<numAl; i++){
		double curCDF = 0.0;
		for(uintptr_t j = 0; j<numAl; j++){
			curCDF += ((i==j) ? mainProb : offProb);
			curFill[j] = (uint64_t)(0x8000000000000000 * (curCDF/totProb));
		}
		curFill += numAl;
	}
}

void whodun::pcrMutationModelStutter(uintptr_t numAl, double negRate, double posRate, uint64_t* toFill){
	uint64_t* curFill = toFill;
	//first row special
	{
		double mainProb = 1.0 - posRate;
		double totProb = mainProb + posRate;
		curFill[0] = (uint64_t)(0x8000000000000000 * (mainProb/totProb));
		for(uintptr_t i = 1; i<numAl; i++){
			curFill[i] = 0x8000000000000000;
		}
		curFill += numAl;
	}
	//middle rows
	for(uintptr_t i = 1; (i+1)<numAl; i++){
		double mainProb = 1.0 - (posRate + negRate);
		double totProb = negRate + mainProb + posRate;
		for(uintptr_t j = 0; (j+1)<i; j++){
			curFill[j] = 0;
		}
		curFill[i-1] = (uint64_t)(0x8000000000000000 * (negRate/totProb));
		curFill[i] = (uint64_t)(0x8000000000000000 * ((negRate+mainProb)/totProb));
		for(uintptr_t j = i+1; j<numAl; j++){
			curFill[j] = 0x8000000000000000;
		}
		curFill += numAl;
	}
	//last row special
	{
		double mainProb = 1.0 - negRate;
		double totProb = mainProb + negRate;
		for(uintptr_t i = 2; i<numAl; i++){
			curFill[i-2] = 0;
		}
		curFill[numAl-2] = (uint64_t)(0x8000000000000000 * (negRate/totProb));
		curFill[numAl-1] = 0x8000000000000000;
	}
}

void whodun::pcrSequencerModelFlat(uintptr_t numAl, double mutRate, double* toFill){
	double offProb = mutRate / (numAl - 1);
	double mainProb = 1 - mutRate;
	double* curFill = toFill;
	for(uintptr_t i = 0; i<numAl; i++){
		for(uintptr_t j = 0; j<numAl; j++){
			curFill[j] = ((i==j) ? mainProb : offProb);
		}
		curFill += numAl;
	}
}

void whodun::pcrSequenceCalcDrawProb(uintptr_t numAl, double* errorMat, uint64_t* tempCount, double* tmpScratch, uint64_t* drawProb){
	//get the total number of templates
		uint64_t totalTemps = 0;
		for(uintptr_t i = 0; i<numAl; i++){
			totalTemps += tempCount[i];
		}
	//calculate errorMat * tempProb
		double* curEMRow = errorMat;
		for(uintptr_t i = 0; i<numAl; i++){
			double curDot = 0.0;
			for(uintptr_t j = 0; j<numAl; j++){
				double curEnt = totalTemps ? (tempCount[j] * curEMRow[j] / totalTemps) : (curEMRow[j] / numAl);
				curDot += curEnt;
			}
			tmpScratch[i] = curDot;
			curEMRow += numAl;
		}
	//calculate total probability
		double totalPro = 0.0;
		for(uintptr_t i = 0; i<numAl; i++){
			totalPro += tmpScratch[i];
		}
	//turn into cdf
		double totalCDF = 0.0;
		for(uintptr_t i = 0; i<numAl; i++){
			totalCDF += tmpScratch[i];
			drawProb[i] = (uint64_t)(0x8000000000000000 * (totalCDF/totalPro));
		}
}

GenlikeHaploCollapseCompGenos::~GenlikeHaploCollapseCompGenos(){}
uintptr_t GenlikeHaploCollapseCompGenos::itemSize(){
	return sizeof(uintptr_t);
}
bool GenlikeHaploCollapseCompGenos::compare(const void* itemA, const void* itemB){
	uintptr_t indA = *((const uintptr_t*)itemA);
	uintptr_t indB = *((const uintptr_t*)itemB);
	uintptr_t numH = allGeno->numH;
	unsigned short* genoA = &(allGeno->hotGenos[indA * numH]);
	unsigned short* genoB = &(allGeno->hotGenos[indB * numH]);
	for(uintptr_t i = 0; i<numH; i++){
		unsigned short curA = genoA[i];
		unsigned short curB = genoB[i];
		if(curA != curB){ return curA < curB; }
	}
	return 0;
}




