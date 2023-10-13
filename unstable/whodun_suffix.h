#ifndef WHODUN_SUFFIX_H
#define WHODUN_SUFFIX_H 1

/**
 * @file
 * @brief Suffix array utilities.
 */

#include <vector>
#include <stdint.h>

#include "whodun_sort.h"
#include "whodun_string.h"
#include "whodun_thread.h"
#include "whodun_container.h"

/**Suffix array entries are pointer sized.*/
#define WHODUN_SUFFIX_ARRAY_PTR 0
/**Suffix array entries are 32-bits.*/
#define WHODUN_SUFFIX_ARRAY_I32 32
/**Suffix array entries are 64-bits.*/
#define WHODUN_SUFFIX_ARRAY_I64 64

namespace whodun{

/**Build a suffix array in memory.*/
class SuffixArrayBuilder{
public:
	/**
	 * Set up a single threaded builder.
	 * @param itemS The size of integer to use for the array entries.
	 */
	SuffixArrayBuilder(uintptr_t itemS);
	/**
	 * Set up a builder that uses multiple threads.
	 * @param itemS The size of integer to use for the array entries.
	 * @param numThr The number of threads to use.
	 * @param mainPool The pool to use.
	 */
	SuffixArrayBuilder(uintptr_t itemS, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~SuffixArrayBuilder();
	
	/**
	 * Build the suffix array.
	 * @param buildOn The string to build on.
	 * @param saveArr The place to put the result.
	 */
	void build(SizePtrString buildOn, void* saveArr);
	
	/**The number of threads to use.*/
	uintptr_t numThread;
	/**The pool to use.*/
	ThreadPool* usePool;
	
	/**Do sorting (on rank).*/
	PODInMemoryMergesort* rankSort;
	/**Set up the initial ranks.*/
	ParallelForLoop* initRank;
	/**Rerank (and fix between threads).*/
	std::vector<JoinableThreadTask*> reRanks;
	/**Build an inverse map.*/
	ParallelForLoop* invMapB;
	/**Figure out the next ranks.*/
	ParallelForLoop* nextRank;
	/**Dump out the final results.*/
	ParallelForLoop* dumpEnd;
	
	/**The size of each piece of an array entry.*/
	uintptr_t itemSize;
	/**Store a set of entries while building.*/
	StructVector<char> entryStore;
	/**Store an inverse map.*/
	StructVector<char> invMapS;
	/**Save whether each chunk needs incrementing.*/
	std::vector<bool> chunkInc;
};

/**A search status in a suffix array.*/
typedef struct{
	/**The string the suffix array is for.*/
	SizePtrString forStr;
	/**The suffix array.*/
	void* inArray;
	/**The size of each piece of an array entry.*/
	uintptr_t arrSize;
	/**The number of characters matched so far.*/
	uintptr_t charInd;
	/**The suffix array index to start from.*/
	uintptr_t fromInd;
	/**The suffix array index to end at.*/
	uintptr_t toInd;
} SuffixArraySearchState;

/**Perform searches in a suffix array.*/
class SuffixArraySearch{
public:
	/**Default setup.*/
	SuffixArraySearch();
	/**Tear down.*/
	~SuffixArraySearch();
	
	/**
	 * Start a search.
	 * @param origStr The string to search through.
	 * @param arrSize The size of indices in the array.
	 * @param suffArr The suffix array.
	 * @param saveS The place to put the state.
	 */
	void startSearch(SizePtrString origStr, uintptr_t arrSize, void* suffArr, SuffixArraySearchState* saveS);
	
	/**
	 * Limit a search to matching characters.
	 * @param searchS The search to update.
	 * @param toChar The character to match.
	 */
	void limitMatch(SuffixArraySearchState* searchS, char toChar);
};

};

#endif

