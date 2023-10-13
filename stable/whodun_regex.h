#ifndef WHODUN_REGEX_H
#define WHODUN_REGEX_H 1

/**
 * @file
 * @brief (Basic) regular expressions.
 */

#include <vector>
#include <stdint.h>

#include "whodun_string.h"

namespace whodun {

/**Mark a start state.*/
#define WHODUN_NFA_STATE_START 1
/**Mark a state that accepts. A state may both accept and reject: what that means depends on use.*/
#define WHODUN_NFA_STATE_ACCEPT 2
/**Mark a state that rejects.*/
#define WHODUN_NFA_STATE_REJECT 4

/**A state in a nondeterministic finite automaton.*/
class NondeterministicFiniteAutomataState{
public:
	/**Set up.*/
	NondeterministicFiniteAutomataState();
	/**Tear down.*/
	~NondeterministicFiniteAutomataState();
	
	/**Extra information about the state.*/
	uintptr_t stateStatus;
	/**Transitions on each character.*/
	uintptr_t charTransitions[256];
	/**Any transitions on epsilon.*/
	std::vector<uintptr_t> epsilonTransitions;
};

/**A nondeterministic finite automata.*/
class NondeterministicFiniteAutomata{
public:
	/**
	 * Set up.
	 * @param numStates The number of states needed.
	 */
	NondeterministicFiniteAutomata(uintptr_t numStates);
	/**Tear down.*/
	~NondeterministicFiniteAutomata();
	
	/**The states in the automata.*/
	std::vector<NondeterministicFiniteAutomataState> states;
};

/**The state of a traversal of an NFA.*/
class NondeterministicFiniteAutomataTraversal{
public:
	/**Set up an empty state.*/
	NondeterministicFiniteAutomataTraversal();
	/**Clean up.*/
	~NondeterministicFiniteAutomataTraversal();
	
	/**
	 * Initialize the states.
	 * @param toWalk The NFA to walk over.
	 */
	void initialize(NondeterministicFiniteAutomata* toWalk);
	
	/**
	 * Eat a byte.
	 * @param toC The byte to eat.
	 * @param fromState The starting state.
	 */
	void consume(char toC, NondeterministicFiniteAutomataTraversal* fromState);
	
	/**The NFA being walked.*/
	NondeterministicFiniteAutomata* toW;
	/**The status of each state.*/
	std::vector<bool> status;
	
	/**The number of accepting states.*/
	uintptr_t numAccept;
	/**The number of rejecting states.*/
	uintptr_t numReject;
	/**The number of hot states.*/
	uintptr_t numHot;
	
	/**Utility: follow epsilon jumps, update counts.*/
	void expandEpsilon();
};

/**A state in a deterministic finite automata.*/
class DeterministicFiniteAutomataState{
public:
	/**Extra information about the state.*/
	uintptr_t stateStatus;
	/**Transitions on each character.*/
	uintptr_t charTransitions[256];
};

/**The deterministic version of the above.*/
class DeterministicFiniteAutomata{
public:
	/**Make a blank set of states.*/
	DeterministicFiniteAutomata();
	/**
	 * Walk through the possible states of an nfa.
	 * @param toCrank The nfa to convert.
	 */
	DeterministicFiniteAutomata(NondeterministicFiniteAutomata* toCrank);
	/**Clean up.*/
	~DeterministicFiniteAutomata();
	
	/**The states in this thing: state 0 is the start.*/
	std::vector<DeterministicFiniteAutomataState> states;
};

/**A basic regular expression.*/
class RegexSpecification{
public:
	/**Set up.*/
	RegexSpecification();
	/**Allow subclasses.*/
	virtual ~RegexSpecification();
	
	/**
	 * Get the number of nfa states this needs.
	 * @return The number of needed nfa states.
	 */
	virtual uintptr_t neededStates() = 0;
	
	/**
	 * Actually pack the states.
	 * @param toPack The place to put them.
	 * @param state0 The offset of the first state.
	 */
	virtual void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0) = 0;
};

/**The empty string.*/
class RegexEpsilon : public RegexSpecification{
public:
	/**Set up.*/
	RegexEpsilon();
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

/**A sequence of characters.*/
class RegexLiteral : public RegexSpecification{
public:
	/**
	 * Set the sequence to match.
	 * @param theSeq The sequence to match.
	 */
	RegexLiteral(SizePtrString theSeq);
	/**
	 * Set the sequence to match.
	 * @param theSeq The sequence to match.
	 */
	RegexLiteral(const char* theSeq);
	/**
	 * Set the sequence to match.
	 * @param theSeq The character to match.
	 */
	RegexLiteral(char theSeq);
	
	/**The thing to match.*/
	SizePtrString toMatch;
	/**Place to save a single character.*/
	char saveChar;
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

/**Match one of a set of characters.*/
class RegexCharset : public RegexSpecification{
public:
	/**
	 * Set the characters to match.
	 * @param theSeq The characters to match.
	 */
	RegexCharset(SizePtrString theSeq);
	/**
	 * Set the sequence to match.
	 * @param theSeq The sequence to match.
	 */
	RegexCharset(const char* theSeq);
	
	/**The thing to match.*/
	SizePtrString toMatch;
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

/**A concatenation of zero or more items.*/
class RegexConcatenate : public RegexSpecification{
public:
	/**Create an empty regex.*/
	RegexConcatenate();
	
	/**The things to go through.*/
	std::vector<RegexSpecification*> toConcat;
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

/**One of several items.*/
class RegexAlternate : public RegexSpecification{
public:
	/**Create an empty regex.*/
	RegexAlternate();
	
	/**The things to go through.*/
	std::vector<RegexSpecification*> toConcat;
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

/**An item repeated.*/
class RegexStar : public RegexSpecification{
public:
	/**
	 * Repeat an item.
	 * @param toRepeat The item to repeat.
	 */
	RegexStar(RegexSpecification* toRepeat);
	
	/**The things to go through.*/
	RegexSpecification* toRep;
	
	uintptr_t neededStates();
	void packStates(NondeterministicFiniteAutomata* toPack, uintptr_t state0);
};

//+ ? {N}

/**Compile and use a regular expression.*/
class Regex{
public:
	/**
	 * Compile a regex.
	 * @param toFind The regular expression to hunt for.
	 */
	Regex(RegexSpecification* toFind);
	/**Clena up*/
	~Regex();
	
	/**
	 * Get the length of the match starting at the first character.
	 * @param toLook The string to look at.
	 * @return The number of characters in the match: -1 for no match.
	 */
	intptr_t longMatchLength(SizePtrString toLook);
	/**
	 * Get the length of the match starting at the first character.
	 * @param toLook The string to look at.
	 * @return The number of characters in the match: -1 for no match.
	 */
	intptr_t firstMatchLength(SizePtrString toLook);
	/**
	 * Look for the regular expression anywhere in the string.
	 * @param toLook The string to look at.
	 * @param repS The place to put the location of the start of the match.
	 * @param repE The place to put the location of the end of the match.
	 * @return Whether there was a match.
	 */
	int findFirst(SizePtrString toLook, uintptr_t* repS, uintptr_t* repE);
	/**
	 * Look for the regular expression anywhere in the string.
	 * @param toLook The string to look at.
	 * @param repS The place to put the location of the start of the match.
	 * @param repE The place to put the location of the end of the match.
	 * @return Whether there was a match.
	 */
	int findLong(SizePtrString toLook, uintptr_t* repS, uintptr_t* repE);
	/**
	 * See if the given string matches this pattern.
	 * @param toLook The string to look at.
	 * @return Whether it matches.
	 */
	int matches(SizePtrString toLook);
	
	/**The compiled expression.*/
	DeterministicFiniteAutomata* compRegex;
};

/**A collection of regular expressions.*/
class RegexSet{
public:
	/**Create an empty set of regexes.*/
	RegexSet();
	/**Clean up*/
	~RegexSet();
	
	/**Once all the regexes have been added, compile any acceleration structures.*/
	void compile();
	
	/**
	 * Get the length of the match starting at the first character.
	 * @param toLook The string to look at.
	 * @param storeTC The place to store the number of token types.
	 * @param storeTypes The place to store the token types.
	 * @return The number of characters in the match: -1 for no match.
	 */
	intptr_t longMatchLength(SizePtrString toLook, uintptr_t* storeTC, uintptr_t** storeTypes);
	
	/**The regular expressions.*/
	std::vector<Regex*> theReg;
	/**Information on them, compiled into a handy structure.*/
	DeterministicFiniteAutomata compiledRegs;
	/**Collected information on the structures: status, numStates, states[]*/
	std::vector<uintptr_t> compiledStatus;
};

};

#endif

