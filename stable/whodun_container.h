#ifndef WHODUN_CONTAINER_H
#define WHODUN_CONTAINER_H 1

/**
 * @file
 * @brief Some useful container things.
 */

#include <map>
#include <set>
#include <vector>
#include <string.h>

#include "whodun_string.h"
#include "whodun_oshook.h"

namespace whodun {

/**
 * Call delete on everything in a vector.
 * @param toKill The vector to kill in.
 */
template <typename OfT>
void deleteAll(std::vector<OfT*>* toKill){
	for(uintptr_t i = 0; i<toKill->size(); i++){
		OfT* curKill = (*toKill)[i];
		if(curKill){ delete(curKill); }
	}
}

/**A simpler vector for structures/pod classes.*/
template <typename OfT>
class StructVector{
public:
	/**Set up an empty vector.*/
	StructVector(){
		curCap = 16;
		curSize = 0;
		datums = (OfT*)malloc(curCap*sizeof(OfT));
	}
	/**
	 * Set up a vector with the specified size.
	 * @param newSize The size to make it with.
	 */
	StructVector(uintptr_t newSize){
		curCap = newSize;
		curSize = newSize;
		datums = (OfT*)malloc(curCap*sizeof(OfT));
	}
	/**
	 * Copy constructor.
	 * @param toCopy The thing to copy.
	 */
	StructVector(const StructVector<OfT>& toCopy){
		uintptr_t newSize = toCopy.curSize;
		if(newSize < 16){ newSize = 16; }
		datums = (OfT*)malloc(newSize * sizeof(OfT));
		curCap = newSize;
		curSize = toCopy.curSize;
		memcpy(datums, toCopy.datums, curSize * sizeof(OfT));
	}
	/**Tear down.*/
	~StructVector(){
		free(datums);
	}
	/**
	 * Assignment operator
	 * @param newVal The value to set to.
	 */
	StructVector<OfT>& operator=(const StructVector<OfT>& newVal){
		//quit early on assignment to self
		if(newVal.datums == datums){ return *this; }
		//resize and copy
		clear();
		resize(newVal.curSize);
		memcpy(datums, newVal.datums, curSize * sizeof(OfT));
		//and return
		return *this;
	}
	/**
	 * Get the size of the vector.
	 * @return The number of things in the vector.
	 */
	size_t size(){
		return curSize;
	}
	/**
	 * Get an element of the vector.
	 * @param itemI The index of interest.
	 * @return The element.
	 */
	OfT* at(uintptr_t itemI){
		return datums + itemI;
	}
	/**
	 * Get an element of the vector.
	 * @param itemI The index of interest.
	 * @return The element.
	 */
	OfT* operator[](uintptr_t itemI){
		return datums + itemI;
	}
	
	/**Empty the vector.*/
	void clear(){
		curSize = 0;
	}
	/**
	 * Resize the vector. Any new allocations will be uninitialized.
	 * @param newSize The new size.
	 */
	void resize(uintptr_t newSize){
		if(curCap < newSize){
			curCap = std::max(2*curCap,newSize);
			datums = (OfT*)realloc(datums, curCap*sizeof(OfT));
		}
		curSize = newSize;
	}
	
	/**
	 * Insert data in this vector.
	 * @param atInd The index to insert at.
	 * @param fromD The first thing to add.
	 * @param toD The place to stop adding at.
	 */
	void insert(uintptr_t atInd, OfT* fromD, OfT* toD){
		//reallocate if necessary
			uintptr_t numAdd = toD - fromD;
			uintptr_t origSize = curSize;
			resize(origSize + numAdd);
		//shift everything after down
			OfT* curSrcP = datums + origSize;
			OfT* curTgtP = datums + curSize;
			while(origSize > atInd){
				curSrcP--;
				curTgtP--;
				*curTgtP = *curSrcP;
				origSize--;
			}
		//and add the rest
			memcpy(datums + atInd, fromD, numAdd * sizeof(OfT));
	}
	
	/**
	 * Push a thing on the back of this thing.
	 * @param toPush The thing to push.
	 */
	void push_back(OfT* toPush){
		uintptr_t nei = curSize;
		resize(nei+1);
		*(at(nei)) = *toPush;
	}
	
	/**
	 * Pop a thing off the back of this thing.
	 */
	void pop_back(){
		curSize--;
	}
	
	/**The storage space for the data.*/
	OfT* datums;
	/**The current size of the thing.*/
	uintptr_t curSize;
	/**The number of things this thing can thing.*/
	uintptr_t curCap;
};

/**
 * This will package characters in a vector.
 * @param toPackage The string to pack.
 * @return As a SizePtrString.
 */
SizePtrString toSizePtr(StructVector<char>* toPackage);

/**A faster deque for structures/pod classes.*/
template <typename OfT>
class StructDeque{
public:
	/**
	 * Set up an empty circular array.
	 */
	StructDeque(){
		offset0 = 0;
		curSize = 0;
		arenaSize = 1;
		saveArena = (OfT*)malloc(arenaSize * sizeof(OfT));;
	}
	/**Clean up.*/
	~StructDeque(){
		if(saveArena){ free(saveArena); }
	}
	/**
	 * Get the number of things in the thing.
	 * @return The thing.
	 */
	size_t size(){
		return curSize;
	}
	/**
	 * Get an item.
	 * @param itemI The index to get.
	 * @return The relevant item.
	 */
	OfT* getItem(uintptr_t itemI){
		return saveArena + ((itemI + offset0) % arenaSize);
	}
	/**
	 * Get the number of things that can be pushed back in one go and still be contiguous.
	 * @return The number of things that can be pushed back and have all of them contiguous. Note that this may be zero if there is currently no space.
	 */
	uintptr_t pushBackSpan(){
		uintptr_t sizeMax = arenaSize - curSize;
		uintptr_t loopMax = arenaSize - ((offset0 + curSize)%arenaSize);
		return std::min(sizeMax, loopMax);
	}
	/**
	 * Mark some number of spots as occupied and get the address of the first thing.
	 * @param numPush THe number of spots to occupy.
	 * @return The address of the first item. Note that the items may be discontinuous.
	 */
	OfT* pushBack(uintptr_t numPush){
		//reallocate if necessary
		if((curSize + numPush) > arenaSize){
			uintptr_t numAlloc = std::max((arenaSize << 1) + 1, (curSize + numPush));
			uintptr_t origSize = curSize;
			OfT* newAlloc = (OfT*)malloc(numAlloc * sizeof(OfT));
			OfT* cpyAlloc = newAlloc;
			while(curSize){
				uintptr_t maxGet = popFrontSpan();
				OfT* curTgt = popFront(maxGet);
				memcpy(cpyAlloc, curTgt, maxGet * sizeof(OfT));
				cpyAlloc += maxGet;
			}
			if(saveArena){ free(saveArena); }
			saveArena = newAlloc;
			offset0 = 0;
			curSize = origSize;
			arenaSize = numAlloc;
		}
		//start pushing
		OfT* toRet = getItem(curSize);
		curSize += numPush;
		return toRet;
	}
	/**
	 * Note how many contiguous items can be popped from the front.
	 * @return The number of contiguous items at the front. Note that an empty queue will return 0.
	 */
	uintptr_t popFrontSpan(){
		uintptr_t sizeMax = curSize;
		uintptr_t loopMax = arenaSize - offset0;
		return std::min(sizeMax, loopMax);
	}
	/**
	 * Pop some items from the front and get their memory address (it's still good until YOU do something).
	 * @param numPop The number to pop.
	 * @return The address of the first popped thing. Note that the items may be discontinuous.
	 */
	OfT* popFront(uintptr_t numPop){
		OfT* toRet = getItem(0);
		offset0 = (offset0 + numPop) % arenaSize;
		curSize -= numPop;
		return toRet;
	}
	/**The offset to the first item.*/
	uintptr_t offset0;
	/**The number of items currently present.*/
	uintptr_t curSize;
	/**The amount of space in the arena.*/
	uintptr_t arenaSize;
	/**The memory for the stuff.*/
	OfT* saveArena;
};

/**Like std::pair.*/
template <typename TA, typename TB, typename TC>
class triple{
public:
	/**The first thing.*/
	TA first;
	/**The second thing.*/
	TB second;
	/**The third thing.*/
	TC third;
	
	/**Set up an uniniailized tuple.*/
	triple(){}
	
	/**
	 * Set up a tuple with the given values.
	 * @param myF The first value.
	 * @param myS The second value.
	 * @param myT The third value.
	 */
	triple(TA myF, TB myS, TC myT){
		first = myF;
		second = myS;
		third = myT;
	}
	
	/**Compare element by element. @param compTo The value to compare against. @return Whether this is less than compTo.*/
	bool operator < (const triple<TA,TB,TC>& compTo) const{
		if(first < compTo.first){ return true; }
		if(compTo.first < first){ return false; }
		if(second < compTo.second){ return true; }
		if(compTo.second < second){ return false; }
		if(third < compTo.third){ return true; }
		return false;
	}
};

};

#endif

