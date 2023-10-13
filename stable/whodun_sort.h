#ifndef WHODUN_SORT_H
#define WHODUN_SORT_H 1

/**
 * @file
 * @brief Sorting
 */

#include <deque>
#include <vector>
#include <stdint.h>

#include "whodun_ermac.h"
#include "whodun_thread.h"
#include "whodun_string.h"
#include "whodun_compress.h"

namespace whodun {

/**Compare plain old data.*/
class PODComparator{
public:
	/**Allow subclasses to implement.*/
	virtual ~PODComparator();
	/**
	 * Get the size of the items this works on.
	 * @return The size of the items this works on.
	 */
	virtual uintptr_t itemSize() = 0;
	/**
	 * Compare with raw memory address.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whether itemA should come before itemB.
	 */
	virtual bool compare(const void* itemA, const void* itemB) = 0;
};

/**Compare using the default less than.*/
template<typename T>
class PODLessComparator : public PODComparator{
public:
	uintptr_t itemSize(){
		return sizeof(T);
	}
	bool compare(const void* itemA, const void* itemB){
		return *((const T*)itemA) < *((const T*)itemB);
	}
};

/**Compare pointer targets using the default less than.*/
template<typename T>
class PODLessIndirectComparator : public PODComparator{
public:
	uintptr_t itemSize(){
		return sizeof(T*);
	}
	bool compare(const void* itemA, const void* itemB){
		return **((const T**)itemA) < **((const T**)itemB);
	}
};

/**Options for sorting data.*/
class PODSortOptions{
public:
	/** Set up some empty sort options. */
	PODSortOptions();
	/**
	 * Set up some simple sort options.
	 * @param itemS The size of each item.
	 * @param useU The uniform to use.
	 * @param compM The comparison method.
	 */
	PODSortOptions(uintptr_t itemS, void* useU, bool(*compM)(void*,void*,void*));
	/**
	 * Set up to use a comparator.
	 * @param useComp The comparator to use.
	 */
	PODSortOptions(PODComparator* useComp);
	/**
	 * The method to use for comparison (i.e. less than).
	 * @param unif A uniform for the comparison.
	 * @param itemA The first item.
	 * @param itemB The second item.
	 * @return Whther itemA should come before itemB (false if equal).
	 */
	bool (*compMeth)(void* unif, void* itemA,void* itemB);
	/**The uniform to use.*/
	void* useUni;
	/**The size of each item.*/
	uintptr_t itemSize;
};

/**A view of a sorted chunk of data.*/
class PODSortedDataChunk{
public:
	/**
	 * Set up a chunk of data.
	 * @param theOpts Information about the data.
	 * @param dataStart Where the data are in memory.
	 * @param numData The number of things.
	 */
	PODSortedDataChunk(PODSortOptions* theOpts, char* dataStart, uintptr_t numData);
	/**Clean up.*/
	~PODSortedDataChunk();
	
	/**
	 * Find the lower bound for an item in this chunk.
	 * @param lookFor The thing to hunt for.
	 * @return The index of the insertion point.
	 */
	uintptr_t lowerBound(char* lookFor);
	/**
	 * Find the upper bound for an item in this chunk.
	 * @param lookFor The thing to hunt for.
	 * @return The index of the insertion point.
	 */
	uintptr_t upperBound(char* lookFor);
	
	/**The number of things in the thing.*/
	uintptr_t size;
	/**The things in the thing.*/
	char* data;
	/**Options for sorting.*/
	PODSortOptions opts;
};

/**Merge sorted data.*/
class PODSortedDataMerger{
public:
	/**
	 * Set up data merging.
	 * @param theOpts Information about the data.
	 */
	PODSortedDataMerger(PODSortOptions* theOpts);
	/**
	 * Set up data merging.
	 * @param theOpts Information about the data.
	 * @param numThr The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	PODSortedDataMerger(PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up*/
	~PODSortedDataMerger();
	
	/**
	 * Perform the merge.
	 * @param numEA The number of elements in A.
	 * @param dataA The data in A.
	 * @param numEB The number of elements in B.
	 * @param dataB The data in B.
	 * @param dataE The place to put the merged data.
	 */
	void merge(uintptr_t numEA, char* dataA, uintptr_t numEB, char* dataB, char* dataE);
	
	/**
	 * Start a merge, without blocking for finish.
	 * @param numEA The number of elements in A.
	 * @param dataA The data in A.
	 * @param numEB The number of elements in B.
	 * @param dataB The data in B.
	 * @param dataE The place to put the merged data.
	 */
	void startMerge(uintptr_t numEA, char* dataA, uintptr_t numEB, char* dataB, char* dataE);
	
	/**Join a previously started merge.*/
	void joinMerge();
	
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**Storage for things to do.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**Options for sorting.*/
	PODSortOptions opts;
};

/**Do merge sorts in memory.*/
class PODInMemoryMergesort{
public:
	/**
	 * Set up in memory mergesorting.
	 * @param theOpts The options for sorting.
	 */
	PODInMemoryMergesort(PODSortOptions* theOpts);
	/**
	 * Set up in memory mergesorting.
	 * @param theOpts The options for sorting.
	 * @param numThr The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	PODInMemoryMergesort(PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~PODInMemoryMergesort();
	
	/**
	 * Actually sort some stuff.
	 * @param numEntries The number of entries.
	 * @param entryStore The entries.
	 */
	void sort(uintptr_t numEntries, char* entryStore);
	
	/**Whether this wants the original sort to put things in temporary storage.*/
	int wantInTemp;
	/**The amount of space allocated for temporaries.*/
	uintptr_t allocSize;
	/**The temporary space.*/
	char* allocTmp;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**Sort the pieces independently.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The ways to merge things.*/
	std::vector<PODSortedDataMerger*> mergeMeths;
	/**The pieces waiting to merge.*/
	std::vector< uintptr_t > threadPieceSizes;
	/**Options for sorting.*/
	PODSortOptions opts;
};

/**Run sorts in external memory.*/
class PODExternalMergeSort{
public:
	/**
	 * Set up an external sort.
	 * @param workDirName The working directory for temporary files.
	 * @param theOpts The options for sorting.
	 */
	PODExternalMergeSort(const char* workDirName, PODSortOptions* theOpts);
	/**
	 * Set up an external sort.
	 * @param workDirName The working directory for temporary files.
	 * @param theOpts The options for sorting.
	 * @param numThr The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	PODExternalMergeSort(const char* workDirName, PODSortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool);
	/**Clean up.*/
	~PODExternalMergeSort();
	/**
	 * Add some (sorted) data.
	 * @param numEntries The number of entries.
	 * @param entryStore The entries. Memory can be reused after this call.
	 */
	void addData(uintptr_t numEntries, char* entryStore);
	/**
	 * Merge all the data.
	 * @param toDump The place to write it.
	 */
	void mergeData(OutStream* toDump);
	/**A place to dump status, if any.*/
	ErrorLog* statusDump;
	/**The maximum number of bytes to load.*/
	uintptr_t maxLoad;
	/**The maximum number of files to have open at one time while merging.*/
	uintptr_t maxMergeFiles;
	/**The minimum number of entities to have loaded per file while merging.*/
	uintptr_t minMergeEnts;
	/**The number of created temporary files.*/
	uintptr_t numTemps;
	/**The name of the temporary directory.*/
	std::string tempName;
	/**Whether this made the temporary directory.*/
	int madeTemp;
	/**Options for sorting.*/
	PODSortOptions opts;
	/**The number of threads to make use of.*/
	uintptr_t numThread;
	/**The thread pool to use, if any.*/
	ThreadPool* usePool;
	/**Storage for information on temporary files.*/
	std::deque<std::string> allTempBase;
	/**Storage for information on temporary files.*/
	std::deque<std::string> allTempBlock;
	/**
	 * Do a single merge.
	 * @param numFileOpen The number of files to merge in one go.
	 * @param loadEnts The number of entries to load.
	 * @param toDump The place to write.
	 */
	void mergeSingle(uintptr_t numFileOpen, uintptr_t loadEnts, OutStream* toDump);
};

//TODO serialize external

};

#endif

