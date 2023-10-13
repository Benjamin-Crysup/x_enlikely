#include "whodunumain_programs.h"

#include <math.h>

#include "whodun_suffix.h"

namespace whodun{

/**Set probabilities of indels in a fastq file.*/
class FastqSeqGraphIndelProbset{
public:
	/**Basic setup.*/
	FastqSeqGraphIndelProbset();
	/**Allow subclasses.*/
	virtual ~FastqSeqGraphIndelProbset();
	
	/**
	 * Figure the indel probabilities.
	 * @param forSeq The sequence to figure for.
	 * @param forPhred The phred scores for that sequence.
	 * @param toFillIn The place to put the probabilities: stride is 10, one extra set of insertion probabilities at the end.
	 */
	virtual void figureIndepPs(SizePtrString forSeq, SizePtrString forPhred, float* toFillIn) = 0;
};

/**Flat indel probability model.*/
class FastqSeqGraphFlatIndelProbset : public FastqSeqGraphIndelProbset{
public:
	/**
	 * Basic setup.
	 * @param pOpenAdd The probability of opening an addition.
	 * @param pExtAdd The probability of extending an addition.
	 * @param pCloseAdd The probability of closing an addition.
	 * @param pOpenSkip The probability of opening a skip.
	 * @param pExtSkip The probability of extending a skip.
	 * @param pCloseSkip The probability of closing a skip.
	 */
	FastqSeqGraphFlatIndelProbset(double pOpenAdd, double pExtAdd, double pCloseAdd, double pOpenSkip, double pExtSkip, double pCloseSkip);
	/**Tear down.*/
	~FastqSeqGraphFlatIndelProbset();
	void figureIndepPs(SizePtrString forSeq, SizePtrString forPhred, float* toFillIn);
	
	/**The stored indel probabilities.*/
	float indelPs[6];
};

/**Mangle the contents of a fastq file.*/
class FastqSeqGraphReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	FastqSeqGraphReadTask();
	/**Clean up.*/
	~FastqSeqGraphReadTask();
	void doTask();
	
	/**The fastq entries this is working over.*/
	FastqSet* toMangle;
	/**The way to get indel probabilities.*/
	FastqSeqGraphIndelProbset* useProbs;
	/**The index to start at.*/
	uintptr_t startI;
	/**The index to end at.*/
	uintptr_t endI;
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - figure out data size totals
	/**The total amount of name data.*/
	uintptr_t totalNameS;
	/**The total amount of sequence data.*/
	uintptr_t totalSeqS;
	
	//phase 2 : pack
	/**The offset in the name data*/
	uintptr_t nameOffset;
	/**The offset in the sequence data*/
	uintptr_t seqOffset;
	/**The offset in the contig probability data.*/
	uintptr_t contPOffset;
	/**The place to put data.*/
	SeqGraphDataSet* toStore;
	/**Build suffix arrays.*/
	SuffixArrayBuilder doSuff;
};

};

#define FASTQ_CPROB_STRIDE 10
#define FASTQ_CPROB_NUB 3

using namespace whodun;

WhodunFastqToGraphProgram::WhodunFastqToGraphProgram() : 
	optTabIn(0, "--in", "The sequences to read in."),
	optGraphOut(0, "--out", "The sequence graphs to write out."),
	indelModel("Indel Model"),
	indelFlatModel("--indel-flat", &indelModel),
	indelOptionsD("--indel-optd")
{
	name = "fq2sg";
	summary = "Convert fastq to sequence graph.";
	version = "whodunu fq2sg 0.0\nCopyright (C) 2023 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "fq2sg --in IN.fq --out OUT.zlib.bsgrap";
	
	indelFlatModel.summary = "Use a flat indel model: need P_ins(O,E,C) and P_del(O,E,C) (note: not log).";
	indelOptionsD.summary = "Any options for the indel.";
	
	indelOptionsD.usage = "--indel-optd ##.##";
	
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optGraphOut);
	allOptions.push_back(&indelFlatModel);
	allOptions.push_back(&indelOptionsD);
}
WhodunFastqToGraphProgram::~WhodunFastqToGraphProgram(){}
void WhodunFastqToGraphProgram::baseRun(){
	FastqReader* inStr = 0;
	SeqGraphWriter* outStr = 0;
	FastqSeqGraphIndelProbset* figureIndel = 0;
	//set up the pool
	uintptr_t numThr = optTC.value;
	ThreadPool usePool(optTC.value);
	uintptr_t chunkS = numThr * optChunky.value;
	std::vector<JoinableThreadTask*> passUnis;
	try{
		//open everything
			SeqGraphHeader passHead;
				passHead.name = toSizePtr(optTabIn.value.c_str());
				passHead.version = 0;
				passHead.baseMap = toSizePtr("ACGT");
			inStr = new ExtensionFastqReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
			outStr = new ExtensionSeqGraphWriter(&passHead, optGraphOut.value.c_str(), numThr, &usePool, useOut);
		//prepare the conversion
			if(indelFlatModel.value()){
				if(indelOptionsD.value.size() != 6){
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Flat indel probabilities need six pieces of info.", 0, 0);
				}
				for(uintptr_t i = 0; i<6; i++){
					double curP = indelOptionsD.value[i];
					if((curP < 0) || (curP > 1)){
						throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid probability passed to flat indel model.", 0, 0);
					}
				}
				figureIndel = new FastqSeqGraphFlatIndelProbset(indelOptionsD.value[0], indelOptionsD.value[1], indelOptionsD.value[2], indelOptionsD.value[3], indelOptionsD.value[4], indelOptionsD.value[5]);
			}
			else{
				throw std::runtime_error("Da fuq?");
			}
			FastqSet workTab;
			SeqGraphDataSet dumpGraph;
			for(uintptr_t i = 0; i<numThr; i++){
				FastqSeqGraphReadTask* curT = new FastqSeqGraphReadTask();
				curT->toMangle = &workTab;
				curT->toStore = &dumpGraph;
				curT->useProbs = figureIndel;
				passUnis.push_back(curT);
			}
		//pump and dump
			while(1){
				uintptr_t numGot = inStr->read(&workTab, chunkS);
				if(numGot == 0){ break; }
				uintptr_t realNG = workTab.saveNames.size();
				//get some totals
					uintptr_t numPT = realNG / numThr;
					uintptr_t numET = realNG % numThr;
					uintptr_t curInd = 0;
					for(uintptr_t i = 0; i<numThr; i++){
						FastqSeqGraphReadTask* curT = (FastqSeqGraphReadTask*)(passUnis[i]);
						curT->startI = curInd;
						curInd += (numPT + (i < numET));
						curT->endI = curInd;
						curT->phase = 1;
					}
					usePool.addTasks(numThr, &(passUnis[0]));
					joinTasks(numThr, &(passUnis[0]));
					uintptr_t totalName = 0;
					uintptr_t totalSeq = 0;
					for(uintptr_t i = 0; i<numThr; i++){
						FastqSeqGraphReadTask* curT = (FastqSeqGraphReadTask*)(passUnis[i]);
						totalName += curT->totalNameS;
						totalSeq += curT->totalSeqS;
					}
				//make some space
					dumpGraph.nameLengths.resize(realNG);
					dumpGraph.numContigs.resize(realNG);
					dumpGraph.numLinks.resize(realNG);
					dumpGraph.numLinkInputs.resize(realNG);
					dumpGraph.numLinkOutputs.resize(realNG);
					dumpGraph.numBases.resize(realNG);
					dumpGraph.extraLengths.resize(realNG);
					dumpGraph.nameTexts.resize(totalName);
					dumpGraph.contigSizes.resize(realNG);
					dumpGraph.inputLinkData.resize(2*realNG);
					dumpGraph.outputLinkData.resize(2*realNG);
					dumpGraph.contigProbs.resize(FASTQ_CPROB_STRIDE*totalSeq + FASTQ_CPROB_NUB*realNG);
					dumpGraph.linkProbs.resize(realNG);
					dumpGraph.seqTexts.resize(totalSeq);
					dumpGraph.suffixIndices.resize(totalSeq);
					dumpGraph.extraTexts.resize(0);
				//pack em up
					uintptr_t curNameOff = 0;
					uintptr_t curSeqOff = 0;
					uintptr_t curCproOff = 0;
					for(uintptr_t i = 0; i<numThr; i++){
						FastqSeqGraphReadTask* curT = (FastqSeqGraphReadTask*)(passUnis[i]);
						curT->nameOffset = curNameOff;
						curT->seqOffset = curSeqOff;
						curT->contPOffset = curCproOff;
						curNameOff += curT->totalNameS;
						curSeqOff += curT->totalSeqS;
						curCproOff += (curT->totalSeqS*FASTQ_CPROB_STRIDE + (curT->endI - curT->startI)*FASTQ_CPROB_NUB);
						curT->phase = 2;
					}
					usePool.addTasks(numThr, &(passUnis[0]));
					joinTasks(numThr, &(passUnis[0]));
				//dump
					outStr->write(&dumpGraph);
			}
		//close it
			for(uintptr_t i = 0; i<passUnis.size(); i++){ delete(passUnis[i]); } passUnis.clear();
			delete(figureIndel); figureIndel = 0;
			outStr->close(); delete(outStr); outStr = 0;
			inStr->close(); delete(inStr); inStr = 0;
	}
	catch(std::exception& errE){
		for(uintptr_t i = 0; i<passUnis.size(); i++){ delete(passUnis[i]); } passUnis.clear();
		if(figureIndel){ delete(figureIndel); }
		if(inStr){ inStr->close(); delete(inStr); }
		if(outStr){ outStr->close(); delete(outStr); }
		throw;
	}
}

FastqSeqGraphIndelProbset::FastqSeqGraphIndelProbset(){}
FastqSeqGraphIndelProbset::~FastqSeqGraphIndelProbset(){}

FastqSeqGraphFlatIndelProbset::FastqSeqGraphFlatIndelProbset(double pOpenAdd, double pExtAdd, double pCloseAdd, double pOpenSkip, double pExtSkip, double pCloseSkip){
	indelPs[0] = pOpenAdd;
	indelPs[1] = pExtAdd;
	indelPs[2] = pCloseAdd;
	indelPs[3] = pOpenSkip;
	indelPs[4] = pExtSkip;
	indelPs[5] = pCloseSkip;
}
FastqSeqGraphFlatIndelProbset::~FastqSeqGraphFlatIndelProbset(){}
void FastqSeqGraphFlatIndelProbset::figureIndepPs(SizePtrString forSeq, SizePtrString forPhred, float* toFillIn){
	float* curFill = toFillIn;
	for(uintptr_t i = 0; i<forSeq.len; i++){
		memcpy(curFill, indelPs, 6*sizeof(float));
		curFill += FASTQ_CPROB_STRIDE;
	}
	memcpy(curFill, indelPs, 3*sizeof(float));
}

FastqSeqGraphReadTask::FastqSeqGraphReadTask() : doSuff(WHODUN_SUFFIX_ARRAY_PTR){}
FastqSeqGraphReadTask::~FastqSeqGraphReadTask(){}
void FastqSeqGraphReadTask::doTask(){
	if(phase == 1){
		totalNameS = 0;
		totalSeqS = 0;
		for(uintptr_t i = startI; i<endI; i++){
			totalNameS += toMangle->saveNames[i]->len;
			totalSeqS += toMangle->saveStrs[i]->len;
		}
	}
	else{
		uintptr_t* curNameLen = toStore->nameLengths[startI];
		uintptr_t* curNumCont = toStore->numContigs[startI];
		uintptr_t* curNumLink = toStore->numLinks[startI];
		uintptr_t* curNumILin = toStore->numLinkInputs[startI];
		uintptr_t* curNumOLin = toStore->numLinkOutputs[startI];
		uintptr_t* curNumBase = toStore->numBases[startI];
		uintptr_t* curExtraL = toStore->extraLengths[startI];
		char* curNameText = toStore->nameTexts[nameOffset];
		uintptr_t* curConSize = toStore->contigSizes[startI];
		uintptr_t* curILinkD = toStore->inputLinkData[2*startI];
		uintptr_t* curOLinkD = toStore->outputLinkData[2*startI];
		float* curConProb = toStore->contigProbs[contPOffset];
		float* curLinkPro = toStore->linkProbs[startI];
		char* curSeqText = toStore->seqTexts[seqOffset];
		uintptr_t* curSuffInd = toStore->suffixIndices[seqOffset];
		//char* curExtraText = toStore->extraTexts[0];
		for(uintptr_t i = startI; i<endI; i++){
			SizePtrString curFQName = *(toMangle->saveNames[i]);
			SizePtrString curFQSeqd = *(toMangle->saveStrs[i]);
			SizePtrString curFQPred = *(toMangle->savePhreds[i]);
			//start setting the simple stuff
			*curNameLen = curFQName.len; curNameLen++;
			*curNumCont = 1; curNumCont++;
			*curNumLink = 1; curNumLink++;
			*curNumILin = 1; curNumILin++;
			*curNumOLin = 1; curNumOLin++;
			*curNumBase = curFQSeqd.len; curNumBase++;
			*curExtraL = 0; curExtraL++;
			memcpy(curNameText, curFQName.txt, curFQName.len); curNameText += curFQName.len;
			*curConSize = curFQSeqd.len; curConSize++;
			curILinkD[0] = 0; curILinkD[1] = 1; curILinkD+=2;
			curOLinkD[0] = 0; curOLinkD[1] = 0; curOLinkD+=2;
			*curLinkPro = 1.0; curLinkPro++;
			memcpy(curSeqText, curFQSeqd.txt, curFQSeqd.len); curSeqText += curFQSeqd.len;
			//curExtraText is empty
			//fix up the suffix array
			doSuff.build(curFQSeqd, curSuffInd);
			curSuffInd += curFQSeqd.len;
			//fix up the probabilities
				//indels
				useProbs->figureIndepPs(curFQSeqd, curFQPred, curConProb);
				//base probabilities
				for(uintptr_t j = 0; j<curFQSeqd.len; j++){
					curConProb += 6;
					double curEProb = pow(10.0, ((0x00FF & curFQPred.txt[j]) - 33) / -10.0);
					double curRProb = 1.0 - curEProb;
					int numHot; int numCold;
					double wA = 0.0; double wC = 0.0; double wG = 0.0; double wT = 0.0;
					#define FASTQ_CASE(charA,charB,probA,probC,probG,probT) \
						case charA: \
						case charB: \
							numHot = (probA) + (probC) + (probG) + (probT);\
							numCold = (1 - (probA)) + (1 - (probC)) + (1 - (probG)) + (1 - (probT));\
							wA = (probA) ? (curRProb / numHot) : (curEProb / numCold);\
							wC = (probC) ? (curRProb / numHot) : (curEProb / numCold);\
							wG = (probG) ? (curRProb / numHot) : (curEProb / numCold);\
							wT = (probT) ? (curRProb / numHot) : (curEProb / numCold);\
							break;
					switch(curFQSeqd.txt[j]){
						FASTQ_CASE('a','A', 1, 0, 0, 0)
						FASTQ_CASE('c','C', 0, 1, 0, 0)
						FASTQ_CASE('g','G', 0, 0, 1, 0)
						FASTQ_CASE('t','T', 0, 0, 0, 1)
						FASTQ_CASE('u','U', 0, 0, 0, 1)
						FASTQ_CASE('m','M', 1, 1, 0, 0)
						FASTQ_CASE('r','R', 1, 0, 1, 0)
						FASTQ_CASE('w','W', 1, 0, 0, 1)
						FASTQ_CASE('s','S', 0, 1, 1, 0)
						FASTQ_CASE('y','Y', 0, 1, 0, 1)
						FASTQ_CASE('k','K', 0, 0, 1, 1)
						FASTQ_CASE('v','V', 1, 1, 1, 0)
						FASTQ_CASE('h','H', 1, 1, 0, 1)
						FASTQ_CASE('d','D', 1, 0, 1, 1)
						FASTQ_CASE('b','B', 0, 1, 1, 1)
						case 'n':
						case 'N':
						default:
							wA = 0.25;
							wC = 0.25;
							wG = 0.25;
							wT = 0.25;
					};
					curConProb[0] = wA;
					curConProb[1] = wC;
					curConProb[2] = wG;
					curConProb[3] = wT;
					curConProb += 4;
				}
			curConProb += FASTQ_CPROB_NUB;
		}
	}
}



