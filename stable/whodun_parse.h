#ifndef WHODUN_PARSE_H
#define WHODUN_PARSE_H 1

/**
 * @file
 * @brief Parsing operations.
 */

#include "whodun_regex.h"
#include "whodun_string.h"
#include "whodun_thread.h"
#include "whodun_container.h"

namespace whodun {

/**A token.*/
typedef struct{
	/**The text of the token.*/
	SizePtrString text;
	/**The number of types the token could be.*/
	uintptr_t numTypes;
	/**The types it could be.*/
	uintptr_t* types;
} Token;

/**Split strings up into tokens.*/
class Tokenizer{
public:
	/**Generic set up*/
	Tokenizer();
	/**Allow subclasses*/
	virtual ~Tokenizer();
	
	/**
	 * Get the number of types of tokens.
	 * @return The number of types.
	 */
	virtual uintptr_t numTokenTypes() = 0;
	
	/**
	 * Cut a string up into tokens.
	 * @param toCut The string to cut up.
	 * @param fillTokens The place to put the tokens.
	 * @return Any text that did not fit into a token, for one reason or another.
	 */
	virtual SizePtrString tokenize(SizePtrString toCut, StructVector<Token>* fillTokens) = 0;
};

/**Split into tokens based on a character: token type 0 is the delimiter, token type 1 is the text.*/
class CharacterSplitTokenizer : public Tokenizer{
public:
	/**
	 * Set up a tokenizer
	 * @param onChar The character to split on.
	 */
	CharacterSplitTokenizer(char onChar);
	/**Clean up*/
	~CharacterSplitTokenizer();
	uintptr_t numTokenTypes();
	SizePtrString tokenize(SizePtrString toCut, StructVector<Token>* fillTokens);
	/**The character to split on.*/
	char splitOn;
	/**The string method.*/
	StandardMemorySearcher doMem;
	/**The marker for the delimiter.*/
	uintptr_t typeDelim;
	/**The marker for random text.*/
	uintptr_t typeText;
};

/**Split tokens based on character, using multiple threads.*/
class MultithreadedCharacterSplitTokenizer : public CharacterSplitTokenizer{
public:
	/**
	 * Set up a tokenizer
	 * @param onChar The character to split on.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	MultithreadedCharacterSplitTokenizer(char onChar, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~MultithreadedCharacterSplitTokenizer();
	SizePtrString tokenize(SizePtrString toCut, StructVector<Token>* fillTokens);
	/**The thread pool to use.*/
	ThreadPool* usePool;
	/**The actual set of things to run.*/
	void* useForUA;
	/**The actual set of things to run.*/
	void* useForUB;
};

/**Cut into tokens based on a set of regular expressions.*/
class RegexTokenizer : public Tokenizer{
public:
	/**
	 * Make a tokenizer for a set of regexes.
	 * @param useRegex The regex set to use.
	 */
	RegexTokenizer(RegexSet* useRegex);
	/**Clean up*/
	~RegexTokenizer();
	uintptr_t numTokenTypes();
	SizePtrString tokenize(SizePtrString toCut, StructVector<Token>* fillTokens);
	
	/**The regex set to use.*/
	RegexSet* useRgx;
};

};

#endif

