#include "whodunumain_programs.h"

#include <math.h>

#include "whodun_suffix.h"

namespace whodun{

/**Mangle the contents of a sequence file.*/
class SequenceToSeqGraphConvertTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	SequenceToSeqGraphConvertTask();
	/**Clean up.*/
	~SequenceToSeqGraphConvertTask();
	void doTask();
	
	/**The fastq entries this is working over.*/
	SequenceSet* toMangle;
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
	/**The number of characters in play.*/
	uintptr_t numHotChars;
	/**Map from character to probability.*/
	float* charProbMap;
	/**The probability of opening a break.*/
	float openP;
	/**The probability of closing a break.*/
	float closeP;
	/**The probability of extending a break.*/
	float extendP;
	/**The place to put data.*/
	SeqGraphDataSet* toStore;
	/**Build suffix arrays.*/
	SuffixArrayBuilder doSuff;
};

//TODO

};

using namespace whodun;

WhodunSequenceToGraphProgram::WhodunSequenceToGraphProgram() : 
	optTabIn(0, "--in", "The sequences to read in."),
	optGraphOut(0, "--out", "The sequence graphs to write out."),
	expectChars("--chars"),
	translateMap("--eq"),
	pError("--pErr"),
	pOpen("--pOpen"),
	pExtend("--pExtend"),
	pClose("--pClose")
{
	name = "s2sg";
	summary = "Convert basic sequences to sequence graphs.";
	version = "whodunu s2sg 0.0\nCopyright (C) 2023 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "s2sg --in IN.fasta --out OUT.zlib.bsgrap";
	
	expectChars.summary = "The characters in play in this sequence.";
	translateMap.summary = "What any given character may be equivalent to.";
	pError.summary = "The flat probability of base error/variance.";
	pOpen.summary = "The flat probability of opening an indel.";
	pExtend.summary = "The flat probability of extending an indel.";
	pClose.summary = "The flat probability of closing an indel.";
	
	expectChars.usage = "--chars ACGT";
	translateMap.usage = "--eq RAG";
	pError.usage = "--pErr 0.01";
	pOpen.usage = "--pOpen 0.00005";
	pExtend.usage = "--pExtend 0.1";
	pClose.usage = "--pClose 0.00005";
	
	pError.value = 0.01;
	pOpen.value = 0.00005;
	pExtend.value = 0.1;
	pClose.value = 0.00005;
	
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optGraphOut);
	allOptions.push_back(&expectChars);
	allOptions.push_back(&translateMap);
	allOptions.push_back(&pError);
	allOptions.push_back(&pOpen);
	allOptions.push_back(&pExtend);
	allOptions.push_back(&pClose);
}
WhodunSequenceToGraphProgram::~WhodunSequenceToGraphProgram(){}
void WhodunSequenceToGraphProgram::idiotCheck(){
	if((pError.value < 0) || (pError.value > 1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid probability.", 0, 0); }
	if((pOpen.value < 0) || (pOpen.value > 1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid probability.", 0, 0); }
	if((pExtend.value < 0) || (pExtend.value > 1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid probability.", 0, 0); }
	if((pClose.value < 0) || (pClose.value > 1)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid probability.", 0, 0); }
	if(expectChars.value.size() < 2){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Need at least two characters in play for a sequence graph to be meaniningful.", 0, 0); }
	if(expectChars.value.size() > 256){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Either your bytes are 9bit, or you've got duplicate characters.", 0, 0); }
	//idiot check equivalences
	//TODO
}
void WhodunSequenceToGraphProgram::baseRun(){
	//build an error matrix
	uintptr_t numCIP = expectChars.value.size();
	std::vector<float> charErrMat(256*numCIP);
	{
		//figure out the error stuff
			double errVal = pError.value;
			double goodVal = 1 - errVal;
		//figure out which characters are in play (and fill in basic error values)
			short charIPMap[256];
			for(uintptr_t i = 0; i<256; i++){ charIPMap[i] = -1; }
			double comBadVal = errVal / (numCIP - 1);
			for(uintptr_t i = 0; i<expectChars.value.size(); i++){
				int curCV = 0x00FF & expectChars.value[i];
				charIPMap[curCV] = i;
				float* curFill = &(charErrMat[curCV * numCIP]);
				for(uintptr_t j = 0; j<expectChars.value.size(); j++){
					curFill[j] = ((i==j) ? goodVal : comBadVal);
				}
			}
		//walk through anything equivalent
			for(uintptr_t i = 0; i<translateMap.value.size(); i++){
				const char* curTran = translateMap.value[i].c_str();
				if(*curTran == 0){ continue; }
				int startChar = 0x00FF & *curTran;
				curTran++;
				float* curFill = &(charErrMat[startChar * numCIP]);
				uintptr_t numCurG = strlen(curTran);
				uintptr_t numCurB = numCIP - numCurG;
				double badVal; double hotVal;
				if(numCurB == 0){
					badVal = 0;
					hotVal = 1.0 / numCIP;
				}
				else{
					badVal = errVal / numCurB;
					hotVal = goodVal / numCurG;
				}
				for(uintptr_t j = 0; j<numCIP; j++){ curFill[j] = badVal; }
				for(uintptr_t j = 0; j<numCurG; j++){ curFill[charIPMap[0x00FF & curTran[j]]] = hotVal; }
			}
	}
	//open em up
	SequenceReader* inStr = 0;
	SeqGraphWriter* outStr = 0;
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
				passHead.baseMap = toSizePtr(&(expectChars.value));
			inStr = new ExtensionSequenceReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
			outStr = new ExtensionSeqGraphWriter(&passHead, optGraphOut.value.c_str(), numThr, &usePool, useOut);
		//prepare the threading
			SequenceSet workTab;
			SeqGraphDataSet dumpGraph;
			for(uintptr_t i = 0; i<numThr; i++){
				SequenceToSeqGraphConvertTask* curT = new SequenceToSeqGraphConvertTask();
				curT->toMangle = &workTab;
				curT->toStore = &dumpGraph;
				curT->numHotChars = numCIP;
				curT->charProbMap = &(charErrMat[0]);
				curT->openP = pOpen.value;
				curT->closeP = pClose.value;
				curT->extendP = pExtend.value;
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
						SequenceToSeqGraphConvertTask* curT = (SequenceToSeqGraphConvertTask*)(passUnis[i]);
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
						SequenceToSeqGraphConvertTask* curT = (SequenceToSeqGraphConvertTask*)(passUnis[i]);
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
					dumpGraph.contigProbs.resize((6+numCIP)*totalSeq + 3*realNG);
					dumpGraph.linkProbs.resize(realNG);
					dumpGraph.seqTexts.resize(totalSeq);
					dumpGraph.suffixIndices.resize(totalSeq);
					dumpGraph.extraTexts.resize(0);
				//pack em up
					uintptr_t curNameOff = 0;
					uintptr_t curSeqOff = 0;
					uintptr_t curCproOff = 0;
					for(uintptr_t i = 0; i<numThr; i++){
						SequenceToSeqGraphConvertTask* curT = (SequenceToSeqGraphConvertTask*)(passUnis[i]);
						curT->nameOffset = curNameOff;
						curT->seqOffset = curSeqOff;
						curT->contPOffset = curCproOff;
						curNameOff += curT->totalNameS;
						curSeqOff += curT->totalSeqS;
						curCproOff += (curT->totalSeqS*(6+numCIP) + (curT->endI - curT->startI)*3);
						curT->phase = 2;
					}
					usePool.addTasks(numThr, &(passUnis[0]));
					joinTasks(numThr, &(passUnis[0]));
				//dump
					outStr->write(&dumpGraph);
			}
		//close it
			for(uintptr_t i = 0; i<passUnis.size(); i++){ delete(passUnis[i]); } passUnis.clear();
			outStr->close(); delete(outStr); outStr = 0;
			inStr->close(); delete(inStr); inStr = 0;
	}
	catch(std::exception& errE){
		for(uintptr_t i = 0; i<passUnis.size(); i++){ delete(passUnis[i]); } passUnis.clear();
		if(inStr){ inStr->close(); delete(inStr); }
		if(outStr){ outStr->close(); delete(outStr); }
		throw;
	}
}

SequenceToSeqGraphConvertTask::SequenceToSeqGraphConvertTask() : doSuff(WHODUN_SUFFIX_ARRAY_PTR){}
SequenceToSeqGraphConvertTask::~SequenceToSeqGraphConvertTask(){}
void SequenceToSeqGraphConvertTask::doTask(){
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
			for(uintptr_t j = 0; j<curFQSeqd.len; j++){
				curConProb[0] = openP;
				curConProb[1] = extendP;
				curConProb[2] = closeP;
				curConProb[3] = openP;
				curConProb[4] = extendP;
				curConProb[5] = closeP;
				curConProb += 6;
				memcpy(curConProb, charProbMap + numHotChars * (0x00FF & curFQSeqd.txt[j]), numHotChars * sizeof(float));
				curConProb += numHotChars;
			}
			curConProb[0] = openP;
			curConProb[1] = extendP;
			curConProb[2] = closeP;
			curConProb += 3;
		}
	}
}



