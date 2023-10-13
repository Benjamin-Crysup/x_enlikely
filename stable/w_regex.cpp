#include "whodun_regex.h"

#include <map>
#include <string.h>
#include <algorithm>

namespace whodun{

/**
 * Compare traversals.
 * @param itemA The first nfa state.
 * @param itemB The second nfa state.
 * @return Whether state A begins before state B.
 */
bool operator <(const NondeterministicFiniteAutomataTraversal& itemA, const NondeterministicFiniteAutomataTraversal& itemB);

/**A traversal through multiple DFAs.*/
class DFASetTraversal{
public:
	/**Set up.*/
	DFASetTraversal();
	/**Tear down.*/
	~DFASetTraversal();
	
	/**The currently occupied states.*/
	std::vector<uintptr_t> curStates;
};

/**
 * Compare traversals.
 * @param itemA The first nfa state.
 * @param itemB The second nfa state.
 * @return Whether state A begins before state B.
 */
bool operator <(const DFASetTraversal& itemA, const DFASetTraversal& itemB);

}

using namespace whodun;

bool whodun::operator < (const NondeterministicFiniteAutomataTraversal& itemA, const NondeterministicFiniteAutomataTraversal& itemB){
	if(itemA.toW != itemB.toW){ return itemA.toW < itemB.toW; }
	if(itemA.status.size() != itemB.status.size()){ return itemA.status.size() < itemB.status.size(); }
	for(uintptr_t i = 0; i<itemA.status.size(); i++){
		char statA = itemA.status[i];
		char statB = itemB.status[i];
		if(statA && !statB){ return false; }
		if(!statA && statB){ return true; }
	}
	return false;
}

NondeterministicFiniteAutomataState::NondeterministicFiniteAutomataState(){}
NondeterministicFiniteAutomataState::~NondeterministicFiniteAutomataState(){}

NondeterministicFiniteAutomata::NondeterministicFiniteAutomata(uintptr_t numStates){
	states.resize(numStates);
}
NondeterministicFiniteAutomata::~NondeterministicFiniteAutomata(){}

NondeterministicFiniteAutomataTraversal::NondeterministicFiniteAutomataTraversal(){}
NondeterministicFiniteAutomataTraversal::~NondeterministicFiniteAutomataTraversal(){}
void NondeterministicFiniteAutomataTraversal::initialize(NondeterministicFiniteAutomata* toWalk){
	toW = toWalk;
	status.resize(toW->states.size());
	for(uintptr_t i = 0; i<status.size(); i++){
		status[i] = (toW->states[i].stateStatus & WHODUN_NFA_STATE_START) ? 1 : 0;
	}
	expandEpsilon();
}
void NondeterministicFiniteAutomataTraversal::consume(char toC, NondeterministicFiniteAutomataTraversal* fromState){
	toW = fromState->toW;
	status.resize(toW->states.size());
	for(uintptr_t i = 0; i<status.size(); i++){ status[i] = 0; }
	for(uintptr_t i = 0; i<status.size(); i++){
		if(fromState->status[i]){
			status[toW->states[i].charTransitions[0x00FF & toC]] = 1;
		}
	}
	expandEpsilon();
}
void NondeterministicFiniteAutomataTraversal::expandEpsilon(){
	//count the current number of states
	numHot = 0;
	for(uintptr_t i = 0; i<status.size(); i++){ numHot += status[i]; }
	
	//start walking
	tailRecurTgt:
	for(uintptr_t i = 0; i<status.size(); i++){
		if(status[i]==0){ continue; }
		NondeterministicFiniteAutomataState* curS = &(toW->states[i]);
		for(uintptr_t j = 0; j<curS->epsilonTransitions.size(); j++){
			status[curS->epsilonTransitions[j]] = 1;
		}
	}
	//see if it changed
	uintptr_t oldHot = numHot;
	numHot = 0;
	for(uintptr_t i = 0; i<status.size(); i++){ numHot += status[i]; }
	if(oldHot != numHot){ goto tailRecurTgt; }
	
	//update accept/reject counts
	numAccept = 0;
	numReject = 0;
	for(uintptr_t i = 0; i<status.size(); i++){
		if(status[i]==0){ continue; }
		NondeterministicFiniteAutomataState* curS = &(toW->states[i]);
		if(curS->stateStatus & WHODUN_NFA_STATE_ACCEPT){
			numAccept++;
		}
		if(curS->stateStatus & WHODUN_NFA_STATE_REJECT){
			numReject++;
		}
	}
}

DeterministicFiniteAutomata::DeterministicFiniteAutomata(){}
DeterministicFiniteAutomata::DeterministicFiniteAutomata(NondeterministicFiniteAutomata* toCrank){
	NondeterministicFiniteAutomataTraversal nextState;
	NondeterministicFiniteAutomataTraversal storeState;
	//set up the state map
		std::map<NondeterministicFiniteAutomataTraversal,uintptr_t> stateMap;
		std::vector<NondeterministicFiniteAutomataTraversal> openStates;
		DeterministicFiniteAutomataState blankState;
		memset(blankState.charTransitions, 0, 256*sizeof(uintptr_t));
	//set up the first state
		nextState.initialize(toCrank);
		stateMap[nextState] = 0;
		openStates.push_back(nextState);
		if(nextState.numAccept){ blankState.stateStatus = WHODUN_NFA_STATE_ACCEPT; }
		else if(nextState.numHot == nextState.numReject){ blankState.stateStatus = WHODUN_NFA_STATE_REJECT; }
		else{ blankState.stateStatus = 0; }
		states.push_back(blankState);
	//run while there are open states
	while(openStates.size()){
		storeState = openStates[openStates.size() - 1];
		openStates.pop_back();
		uintptr_t updStateIndex = stateMap[storeState];
		for(uintptr_t i = 0; i<256; i++){
			nextState.consume(i, &storeState);
			std::map<NondeterministicFiniteAutomataTraversal,uintptr_t>::iterator smIt;
			smIt = stateMap.find(nextState);
			if(smIt == stateMap.end()){
				stateMap[nextState] = states.size();
				openStates.push_back(nextState);
				if(nextState.numAccept){ blankState.stateStatus = WHODUN_NFA_STATE_ACCEPT; }
				else if(nextState.numHot == nextState.numReject){ blankState.stateStatus = WHODUN_NFA_STATE_REJECT; }
				else{ blankState.stateStatus = 0; }
				states.push_back(blankState);
				smIt = stateMap.find(nextState);
			}
			states[updStateIndex].charTransitions[i] = smIt->second;
		}
	}
}
DeterministicFiniteAutomata::~DeterministicFiniteAutomata(){}


RegexSpecification::RegexSpecification(){}
RegexSpecification::~RegexSpecification(){}

RegexEpsilon::RegexEpsilon(){}
uintptr_t RegexEpsilon::neededStates(){
	return 2;
}
void RegexEpsilon::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	uintptr_t state1 = state0 + 1;
	NondeterministicFiniteAutomataState* sstate = &(toPack->states[state0]);
	NondeterministicFiniteAutomataState* fstate = &(toPack->states[state1]);
	sstate->stateStatus = WHODUN_NFA_STATE_START | WHODUN_NFA_STATE_ACCEPT;
	fstate->stateStatus = WHODUN_NFA_STATE_REJECT;
	for(uintptr_t i = 0; i<256; i++){
		sstate->charTransitions[i] = state1;
		fstate->charTransitions[i] = state1;
	}
}

RegexLiteral::RegexLiteral(SizePtrString theSeq){
	toMatch = theSeq;
}
RegexLiteral::RegexLiteral(const char* theSeq){
	toMatch.len = strlen(theSeq);
	toMatch.txt = (char*)theSeq;
}
RegexLiteral::RegexLiteral(char theSeq){
	saveChar = theSeq;
	toMatch.len = 1;
	toMatch.txt = &saveChar;
}
uintptr_t RegexLiteral::neededStates(){
	return toMatch.len + 2;
}
void RegexLiteral::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	uintptr_t stateF = state0 + toMatch.len + 1;
	NondeterministicFiniteAutomataState* cstate = &(toPack->states[state0]);
	cstate->stateStatus = WHODUN_NFA_STATE_START;
	for(uintptr_t i = 0; i<toMatch.len; i++){
		uintptr_t stateT = state0 + i + 1;
		uintptr_t litC = 0x00FF & (toMatch.txt[i]);
		for(uintptr_t j = 0; j<256; j++){
			cstate->charTransitions[j] = (j==litC) ? stateT : stateF;
		}
		cstate = &(toPack->states[stateT]);
		cstate->stateStatus = 0;
	}
	cstate->stateStatus = cstate->stateStatus | WHODUN_NFA_STATE_ACCEPT;
	NondeterministicFiniteAutomataState* fstate = &(toPack->states[stateF]);
	fstate->stateStatus = WHODUN_NFA_STATE_REJECT;
	for(uintptr_t i = 0; i<256; i++){
		cstate->charTransitions[i] = stateF;
		fstate->charTransitions[i] = stateF;
	}
}

RegexCharset::RegexCharset(SizePtrString theSeq){
	toMatch = theSeq;
}
RegexCharset::RegexCharset(const char* theSeq){
	toMatch.len = strlen(theSeq);
	toMatch.txt = (char*)theSeq;
}
uintptr_t RegexCharset::neededStates(){
	return 3;
}
void RegexCharset::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	uintptr_t stateA = state0 + 1;
	uintptr_t stateF = state0 + 2;
	//get the jumps into a useful form
	char hotJumps[256];
	memset(hotJumps, 0, 256);
	for(uintptr_t i = 0; i<toMatch.len; i++){
		hotJumps[0x00FF & toMatch.txt[i]] = 1;
	}
	//make the jumps
	NondeterministicFiniteAutomataState* sstate = &(toPack->states[state0]);
	sstate->stateStatus = WHODUN_NFA_STATE_START;
	NondeterministicFiniteAutomataState* fstate = &(toPack->states[stateF]);
	fstate->stateStatus = WHODUN_NFA_STATE_REJECT;
	NondeterministicFiniteAutomataState* astate = &(toPack->states[stateA]);
	astate->stateStatus = WHODUN_NFA_STATE_ACCEPT;
	for(uintptr_t i = 0; i<256; i++){
		sstate->charTransitions[i] = hotJumps[i] ? stateA : stateF;
		astate->charTransitions[i] = stateF;
		fstate->charTransitions[i] = stateF;
	}
}
	
RegexConcatenate::RegexConcatenate(){}
uintptr_t RegexConcatenate::neededStates(){
	if(toConcat.size() == 0){ return 2; }
	uintptr_t totS = 0;
	for(uintptr_t i = 0; i<toConcat.size(); i++){
		totS += toConcat[i]->neededStates();
	}
	return totS;
}
void RegexConcatenate::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	if(toConcat.size() == 0){
		//same as epsilon
		RegexEpsilon subBuild;
		subBuild.packStates(toPack, state0);
		return;
	}
	//let the pieces build
	uintptr_t cstate0 = state0;
	for(uintptr_t i = 0; i<toConcat.size(); i++){
		RegexSpecification* subC = toConcat[i];
		subC->packStates(toPack, cstate0);
		cstate0 += subC->neededStates();
	}
	//link up accepts to the next start: clear accepts and starts
	cstate0 = state0;
	for(uintptr_t i = 1; i<toConcat.size(); i++){
		RegexSpecification* prevC = toConcat[i-1];
		uintptr_t nstate0 = cstate0 + prevC->neededStates();
		RegexSpecification* curC = toConcat[i];
		uintptr_t fstate0 = nstate0 + curC->neededStates();
		
		//link
		for(uintptr_t stateS = nstate0; stateS < fstate0; stateS++){
			NondeterministicFiniteAutomataState* tgtState = &(toPack->states[stateS]);
			if((tgtState->stateStatus & WHODUN_NFA_STATE_START) == 0){continue;}
			tgtState->stateStatus = tgtState->stateStatus & ~WHODUN_NFA_STATE_START;
			for(uintptr_t stateA = cstate0; stateA < nstate0; stateA++){
				NondeterministicFiniteAutomataState* srcState = &(toPack->states[stateA]);
				if((srcState->stateStatus & WHODUN_NFA_STATE_ACCEPT) == 0){continue;}
				srcState->stateStatus = srcState->stateStatus & ~WHODUN_NFA_STATE_ACCEPT;
				srcState->epsilonTransitions.push_back(stateS);
			}
		}
		cstate0 = nstate0;
	}
}

RegexAlternate::RegexAlternate(){}
uintptr_t RegexAlternate::neededStates(){
	if(toConcat.size() == 0){ return 2; }
	uintptr_t totS = 2;
	for(uintptr_t i = 0; i<toConcat.size(); i++){
		totS += toConcat[i]->neededStates();
	}
	return totS;
}
void RegexAlternate::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	if(toConcat.size() == 0){
		//same as epsilon
		RegexEpsilon subBuild;
		subBuild.packStates(toPack, state0);
		return;
	}
	//let the pieces build
	uintptr_t cstate0 = state0 + 2;
	for(uintptr_t i = 0; i<toConcat.size(); i++){
		RegexSpecification* subC = toConcat[i];
		subC->packStates(toPack, cstate0);
		cstate0 += subC->neededStates();
	}
	//set up the real start state
	uintptr_t stateF = state0 + 1;
	NondeterministicFiniteAutomataState* sstate = &(toPack->states[state0]);
	NondeterministicFiniteAutomataState* fstate = &(toPack->states[stateF]);
	sstate->stateStatus = WHODUN_NFA_STATE_START;
	fstate->stateStatus = WHODUN_NFA_STATE_REJECT;
	for(uintptr_t i = 0; i<256; i++){
		sstate->charTransitions[i] = stateF;
		fstate->charTransitions[i] = stateF;
	}
	//link any start states to the real start state
	cstate0 = state0 + 2;
	for(uintptr_t i = 0; i<toConcat.size(); i++){
		RegexSpecification* subC = toConcat[i];
		uintptr_t nstate0 = cstate0 + subC->neededStates();
		for(uintptr_t stateS = cstate0; stateS < nstate0; stateS++){
			NondeterministicFiniteAutomataState* tgtState = &(toPack->states[stateS]);
			if((tgtState->stateStatus & WHODUN_NFA_STATE_START) == 0){continue;}
			tgtState->stateStatus = tgtState->stateStatus & ~WHODUN_NFA_STATE_START;
			sstate->epsilonTransitions.push_back(stateS);
		}
		cstate0 = nstate0;
	}
}

RegexStar::RegexStar(RegexSpecification* toRepeat){
	toRep = toRepeat;
}
uintptr_t RegexStar::neededStates(){
	return 2 + toRep->neededStates();
}
void RegexStar::packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0){
	//set up the real start state
	uintptr_t stateF = state0 + 1;
	NondeterministicFiniteAutomataState* sstate = &(toPack->states[state0]);
	NondeterministicFiniteAutomataState* fstate = &(toPack->states[stateF]);
	sstate->stateStatus = WHODUN_NFA_STATE_START | WHODUN_NFA_STATE_ACCEPT;
	fstate->stateStatus = WHODUN_NFA_STATE_REJECT;
	for(uintptr_t i = 0; i<256; i++){
		sstate->charTransitions[i] = stateF;
		fstate->charTransitions[i] = stateF;
	}
	//set up the jumps into
	uintptr_t cstate0 = state0 + 2;
	uintptr_t nstate0 = cstate0 + toRep->neededStates();
	toRep->packStates(toPack, cstate0);
	for(uintptr_t stateS = cstate0; stateS < nstate0; stateS++){
		NondeterministicFiniteAutomataState* tgtState = &(toPack->states[stateS]);
		if((tgtState->stateStatus & WHODUN_NFA_STATE_START) == 0){continue;}
		tgtState->stateStatus = tgtState->stateStatus & ~WHODUN_NFA_STATE_START;
		sstate->epsilonTransitions.push_back(stateS);
	}
	//set up the repeating jumps
	for(uintptr_t stateS = cstate0; stateS < nstate0; stateS++){
		NondeterministicFiniteAutomataState* tgtState = &(toPack->states[stateS]);
		if((tgtState->stateStatus & WHODUN_NFA_STATE_ACCEPT) == 0){continue;}
		tgtState->stateStatus = tgtState->stateStatus & ~WHODUN_NFA_STATE_ACCEPT;
		tgtState->epsilonTransitions.push_back(state0);
	}
}

Regex::Regex(RegexSpecification* toFind){
	NondeterministicFiniteAutomata compNFA(toFind->neededStates());
	toFind->packStates(&compNFA, 0);
	compRegex = new DeterministicFiniteAutomata(&compNFA);
}
Regex::~Regex(){ delete(compRegex); }
intptr_t Regex::longMatchLength(SizePtrString toLook){
	intptr_t winM = -1;
	uintptr_t curState = 0;
	uintptr_t curSStat = compRegex->states[curState].stateStatus;
	if(curSStat & WHODUN_NFA_STATE_ACCEPT){ winM = 0; }
	if(curSStat & WHODUN_NFA_STATE_REJECT){ return winM; }
	
	for(uintptr_t i = 0; i<toLook.len; i++){
		curState = compRegex->states[curState].charTransitions[0x00FF & toLook.txt[i]];
		curSStat = compRegex->states[curState].stateStatus;
		if(curSStat & WHODUN_NFA_STATE_ACCEPT){ winM = i+1; }
		if(curSStat & WHODUN_NFA_STATE_REJECT){ return winM; }
	}
	
	return winM;
}
intptr_t Regex::firstMatchLength(SizePtrString toLook){
	uintptr_t curState = 0;
	uintptr_t curSStat = compRegex->states[curState].stateStatus;
	if(curSStat & WHODUN_NFA_STATE_ACCEPT){ return 0; }
	if(curSStat & WHODUN_NFA_STATE_REJECT){ return -1; }
	
	for(uintptr_t i = 0; i<toLook.len; i++){
		curState = compRegex->states[curState].charTransitions[0x00FF & toLook.txt[i]];
		curSStat = compRegex->states[curState].stateStatus;
		if(curSStat & WHODUN_NFA_STATE_ACCEPT){ return i+1; }
		if(curSStat & WHODUN_NFA_STATE_REJECT){ return -1; }
	}
	
	return -1;
}
int Regex::findLong(SizePtrString toLook, uintptr_t* repS, uintptr_t* repE){
	intptr_t longFind = -1;
	for(uintptr_t i = 0; i<=toLook.len; i++){
		SizePtrString curLook = {toLook.len - i, toLook.txt + i};
		intptr_t curFind = longMatchLength(curLook);
		if(curFind > longFind){
			longFind = curFind;
			*repS = i;
			*repE = (i+curFind);
		}
	}
	return longFind >= 0;
}
int Regex::findFirst(SizePtrString toLook, uintptr_t* repS, uintptr_t* repE){
	for(uintptr_t i = 0; i<=toLook.len; i++){
		SizePtrString curLook = {toLook.len - i, toLook.txt + i};
		intptr_t curFind = firstMatchLength(curLook);
		if(curFind > -1){
			*repS = i;
			*repE = (i+curFind);
			return 1;
		}
	}
	return 0;
}
int Regex::matches(SizePtrString toLook){
	intptr_t matL = longMatchLength(toLook);
	return (matL == (intptr_t)(toLook.len));
}

DFASetTraversal::DFASetTraversal(){}
DFASetTraversal::~DFASetTraversal(){}
bool whodun::operator <(const DFASetTraversal& itemA, const DFASetTraversal& itemB){
	uintptr_t itemASize = itemA.curStates.size();
	uintptr_t itemBSize = itemB.curStates.size();
	if(itemASize != itemBSize){
		return itemASize < itemBSize;
	}
	for(uintptr_t i = 0; i<itemASize; i++){
		uintptr_t curSA = itemA.curStates[i];
		uintptr_t curSB = itemB.curStates[i];
		if(curSA != curSB){
			return curSA < curSB;
		}
	}
	return false;
}

RegexSet::RegexSet(){}
RegexSet::~RegexSet(){}
void RegexSet::compile(){
	uintptr_t numRegex = theReg.size();
	std::map<DFASetTraversal,uintptr_t> seenStates;
	std::vector<DFASetTraversal> openStates;
	//add the starting state to the open set
		DFASetTraversal curState;
			curState.curStates.resize(numRegex);
			seenStates[curState] = 0;
			compiledRegs.states.resize(1);
	//walk through jumps until nothing open
		DFASetTraversal nextState;
			nextState.curStates.resize(numRegex);
		while(openStates.size()){
			//get the current element
				curState = openStates[openStates.size()-1];
				openStates.pop_back();
				uintptr_t dfaIndex = seenStates[curState];
			//figure out its status, including hot states
				uintptr_t winStatIndex = compiledStatus.size();
				compiledRegs.states[dfaIndex].stateStatus = winStatIndex;
				compiledStatus.push_back(0);
				compiledStatus.push_back(0);
				uintptr_t numAccept = 0;
				uintptr_t numReject = 0;
				uintptr_t numPass = 0;
				for(uintptr_t i = 0; i<numRegex; i++){
					uintptr_t curSStatus = theReg[i]->compRegex->states[curState.curStates[i]].stateStatus;
					if(curSStatus & WHODUN_NFA_STATE_ACCEPT){ numAccept++; compiledStatus.push_back(i); }
					if(curSStatus & WHODUN_NFA_STATE_REJECT){ numReject++; }
					if((curSStatus & (WHODUN_NFA_STATE_ACCEPT | WHODUN_NFA_STATE_REJECT)) == 0){ numPass++; }
				}
				if(numAccept){
					compiledStatus[winStatIndex] = WHODUN_NFA_STATE_ACCEPT;
					compiledStatus[winStatIndex + 1] = numAccept;
				}
				else if(numReject && (numPass == 0)){
					compiledStatus[winStatIndex] = WHODUN_NFA_STATE_REJECT;
				}
			//figure out where it goes
				for(uintptr_t c = 0; c<256; c++){
					for(uintptr_t i = 0; i<numRegex; i++){
						nextState.curStates[i] = theReg[i]->compRegex->states[curState.curStates[i]].charTransitions[c];
					}
					std::map<DFASetTraversal,uintptr_t>::iterator hasSeen = seenStates.find(nextState);
					if(hasSeen == seenStates.end()){
						uintptr_t newStateInd = compiledRegs.states.size();
						seenStates[nextState] = newStateInd;
						openStates.push_back(nextState);
						compiledRegs.states.resize(newStateInd+1);
						compiledRegs.states[dfaIndex].charTransitions[c] = newStateInd;
					}
					else{
						compiledRegs.states[dfaIndex].charTransitions[c] = hasSeen->second;
					}
				}
		}
}
intptr_t RegexSet::longMatchLength(SizePtrString toLook, uintptr_t* storeTC, uintptr_t** storeTypes){
	intptr_t winM = -1;
	uintptr_t curState = 0;
	uintptr_t curStatusIndex = compiledRegs.states[curState].stateStatus;
	uintptr_t curStatus = compiledStatus[curStatusIndex];
	if(curStatus & WHODUN_NFA_STATE_ACCEPT){
		*storeTC = compiledStatus[curStatusIndex+1];
		*storeTypes = &(compiledStatus[0]) + curStatusIndex+2;
		winM = 0;
	}
	if(curStatus & WHODUN_NFA_STATE_REJECT){
		*storeTC = 0;
		return winM;
	}
	
	for(uintptr_t i = 0; i<toLook.len; i++){
		curState = compiledRegs.states[curState].charTransitions[0x00FF & toLook.txt[i]];
		curStatusIndex = compiledRegs.states[curState].stateStatus;
		curStatus = compiledStatus[curStatusIndex];
		if(curStatus & WHODUN_NFA_STATE_ACCEPT){
			*storeTC = compiledStatus[curStatusIndex+1];
			*storeTypes = &(compiledStatus[0]) + curStatusIndex+2;
			winM = i+1;
		}
		if(curStatus & WHODUN_NFA_STATE_REJECT){
			return winM;
		}
	}
	
	return winM;
}

