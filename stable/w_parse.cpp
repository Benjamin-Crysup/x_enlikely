#include "whodun_parse.h"

#include <string.h>

namespace whodun {

/**Hunt split points.*/
class MultithreadedCharacterSplitHunter : public JoinableThreadTask{
public:
	/**Basic setup*/
	MultithreadedCharacterSplitHunter();
	/**Tear down*/
	~MultithreadedCharacterSplitHunter();
	void doTask();
	
	/**The character to split on.*/
	char splitOn;
	/**The string to hunt in.*/
	SizePtrString toHunt;
	/**The offset to add to any split indices.*/
	uintptr_t splitOffset;
	/**The identified split indices.*/
	std::vector<uintptr_t> splitIndices;
	/**The string method.*/
	StandardMemorySearcher doMem;
};

/**Turn splits into tokens.*/
class MultithreadedCharacterSplitPatcher : public JoinableThreadTask{
public:
	/**Basic setup*/
	MultithreadedCharacterSplitPatcher();
	/**Tear down*/
	~MultithreadedCharacterSplitPatcher();
	void doTask();
	
	/**The splits to handle.*/
	MultithreadedCharacterSplitHunter* doSplits;
	/**The full text being tokenized.*/
	SizePtrString fullText;
	/**The starting point of this things tokens.*/
	uintptr_t priorBase;
	/**The place to put the tokens.*/
	Token* toFill;
	/**Save the base thing.*/
	MultithreadedCharacterSplitTokenizer* saveBase;
};

};

using namespace whodun;

Tokenizer::Tokenizer(){}
Tokenizer::~Tokenizer(){}

CharacterSplitTokenizer::CharacterSplitTokenizer(char onChar){
	typeDelim = 0;
	typeText = 1;
	splitOn = onChar;
}
CharacterSplitTokenizer::~CharacterSplitTokenizer(){
}
uintptr_t CharacterSplitTokenizer::numTokenTypes(){
	return 2;
}
SizePtrString CharacterSplitTokenizer::tokenize(SizePtrString toCut, StructVector<Token>* fillTokens){
	Token curPush;
	SizePtrString nextCut = toCut;
	char* findIt = (char*)doMem.memchr(nextCut.txt, splitOn, nextCut.len);
	while(findIt){
		curPush.text.txt = nextCut.txt;
			curPush.text.len = findIt - nextCut.txt;
			curPush.numTypes = 1;
			curPush.types = &typeText;
			fillTokens->push_back(&curPush);
		curPush.text.txt = findIt;
			curPush.text.len = 1;
			curPush.numTypes = 1;
			curPush.types = &typeDelim;
			fillTokens->push_back(&curPush);
		nextCut.txt = findIt + 1;
		nextCut.len = (toCut.txt + toCut.len) - nextCut.txt;
		findIt = (char*)doMem.memchr(nextCut.txt, splitOn, nextCut.len);
	}
	return nextCut;
}

MultithreadedCharacterSplitTokenizer::MultithreadedCharacterSplitTokenizer(char onChar, uintptr_t numThread, ThreadPool* mainPool) : CharacterSplitTokenizer(onChar){
	usePool = mainPool;
	std::vector<MultithreadedCharacterSplitHunter*>* huntTasks = new std::vector<MultithreadedCharacterSplitHunter*>();
	std::vector<MultithreadedCharacterSplitPatcher*>* patchTasks = new std::vector<MultithreadedCharacterSplitPatcher*>();
	for(uintptr_t i = 0; i<numThread; i++){
		MultithreadedCharacterSplitHunter* curHunt = new MultithreadedCharacterSplitHunter();
			curHunt->splitOn = onChar;
			huntTasks->push_back(curHunt);
		MultithreadedCharacterSplitPatcher* curPatch = new MultithreadedCharacterSplitPatcher();
			curPatch->doSplits = curHunt;
			curPatch->saveBase = this;
			patchTasks->push_back(curPatch);
	}
	useForUA = huntTasks;
	useForUB = patchTasks;
}
MultithreadedCharacterSplitTokenizer::~MultithreadedCharacterSplitTokenizer(){
	std::vector<MultithreadedCharacterSplitHunter*>* huntTasks = (std::vector<MultithreadedCharacterSplitHunter*>*)useForUA;
	std::vector<MultithreadedCharacterSplitPatcher*>* patchTasks = (std::vector<MultithreadedCharacterSplitPatcher*>*)useForUB;
	for(uintptr_t i = 0; i<huntTasks->size(); i++){
		delete((*huntTasks)[i]);
		delete((*patchTasks)[i]);
	}
	delete(huntTasks);
	delete(patchTasks);
}
SizePtrString MultithreadedCharacterSplitTokenizer::tokenize(SizePtrString toCut, StructVector<Token>* fillTokens){
	//set up the hunts
		std::vector<MultithreadedCharacterSplitHunter*>* huntTasks = (std::vector<MultithreadedCharacterSplitHunter*>*)useForUA;
		uintptr_t numThread = huntTasks->size();
		uintptr_t numPT = toCut.len / numThread;
		uintptr_t numET = toCut.len % numThread;
		uintptr_t curInd = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			uintptr_t curNum = numPT + (i < numET);
			MultithreadedCharacterSplitHunter* curHunt = ((*huntTasks)[i]);
			curHunt->toHunt.txt = toCut.txt + curInd;
			curHunt->toHunt.len = curNum;
			curHunt->splitOffset = curInd;
			curInd += curNum;
		}
		usePool->addTasks(numThread, (JoinableThreadTask**)(&((*huntTasks)[0])));
		joinTasks(numThread, (JoinableThreadTask**)(&((*huntTasks)[0])));
	//figure out how many splits there are
		uintptr_t numSplit = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			MultithreadedCharacterSplitHunter* curHunt = ((*huntTasks)[i]);
			numSplit += curHunt->splitIndices.size();
		}
		if(numSplit == 0){ return toCut; }
	//patch up the tokens
		std::vector<MultithreadedCharacterSplitPatcher*>* patchTasks = (std::vector<MultithreadedCharacterSplitPatcher*>*)useForUB;
		uintptr_t origSize = fillTokens->size();
		fillTokens->resize(origSize + 2*numSplit);
		Token* curPushTok = (*fillTokens)[origSize];
		uintptr_t curBaseIndex = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			MultithreadedCharacterSplitHunter* curHunt = ((*huntTasks)[i]);
			MultithreadedCharacterSplitPatcher* curPatch = ((*patchTasks)[i]);
			curPatch->fullText = toCut;
			curPatch->priorBase = curBaseIndex;
			curPatch->toFill = curPushTok;
			uintptr_t curNumSplit = curHunt->splitIndices.size();
			if(curNumSplit){
				curPushTok += (2*curNumSplit);
				curBaseIndex = curHunt->splitIndices[curNumSplit-1] + 1;
			}
		}
		usePool->addTasks(numThread, (JoinableThreadTask**)(&((*patchTasks)[0])));
		joinTasks(numThread, (JoinableThreadTask**)(&((*patchTasks)[0])));
	//note what wasn't cut
		SizePtrString remText;
			remText.txt = toCut.txt + curBaseIndex;
			remText.len = toCut.len - curBaseIndex;
		return remText;
}

MultithreadedCharacterSplitHunter::MultithreadedCharacterSplitHunter(){}
MultithreadedCharacterSplitHunter::~MultithreadedCharacterSplitHunter(){}
void MultithreadedCharacterSplitHunter::doTask(){
	splitIndices.clear();
	SizePtrString remHunt = toHunt;
	char* findIt = (char*)doMem.memchr(remHunt.txt, splitOn, remHunt.len);
	while(findIt){
		splitIndices.push_back(splitOffset + (findIt - toHunt.txt));
		remHunt.txt = findIt + 1;
		remHunt.len = (toHunt.txt + toHunt.len) - remHunt.txt;
		findIt = (char*)doMem.memchr(remHunt.txt, splitOn, remHunt.len);
	}
}

MultithreadedCharacterSplitPatcher::MultithreadedCharacterSplitPatcher(){}
MultithreadedCharacterSplitPatcher::~MultithreadedCharacterSplitPatcher(){}
void MultithreadedCharacterSplitPatcher::doTask(){
	uintptr_t prevStart = priorBase;
	Token* curFill = toFill;
	uintptr_t numSplits = doSplits->splitIndices.size();
	for(uintptr_t i = 0; i<numSplits; i++){
		uintptr_t curSplitInd = doSplits->splitIndices[i];
		//make the text token
			curFill->text.len = curSplitInd - prevStart;
			curFill->text.txt = fullText.txt + prevStart;
			curFill->numTypes = 1;
			curFill->types = &(saveBase->typeText);
			curFill++;
		//make the delimiter token
			curFill->text.len = 1;
			curFill->text.txt = fullText.txt + curSplitInd;
			curFill->numTypes = 1;
			curFill->types = &(saveBase->typeDelim);
			curFill++;
		prevStart = curSplitInd + 1;
	}
}

RegexTokenizer::RegexTokenizer(RegexSet* useRegex){
	useRgx = useRegex;
}
RegexTokenizer::~RegexTokenizer(){}
uintptr_t RegexTokenizer::numTokenTypes(){
	return useRgx->theReg.size();
}
SizePtrString RegexTokenizer::tokenize(SizePtrString toCut, StructVector<Token>* fillTokens){
	Token curPush;
	SizePtrString nextCut = toCut;
	while(nextCut.len){
		//TODO fix this to cut on certainty only
		intptr_t longMat = useRgx->longMatchLength(nextCut, &(curPush.numTypes), &(curPush.types));
		if(longMat < 0){ break; }
		curPush.text.txt = nextCut.txt;
		curPush.text.len = longMat;
		fillTokens->push_back(&curPush);
		nextCut.txt += longMat;
		nextCut.len -= longMat;
	}
	return nextCut;
}



