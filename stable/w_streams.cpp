#include "whodun_streams.h"

#include <string.h>
#include <iostream>
#include <exception>
#include <stdexcept>

using namespace whodun;

NullInStream::NullInStream(){}
NullInStream::~NullInStream(){}
uintptr_t NullInStream::read(char* toR, uintptr_t numR){
	return 0;
}
int NullInStream::read(){
	return -1;
}
void NullInStream::close(){
	isClosed = 1;
}
uintmax_t NullInStream::tell(){
	return 0;
}
void NullInStream::seek(uintmax_t toLoc){
	if(toLoc){ throw std::runtime_error("Empty stream has no data, cannot seek."); }
}
uintmax_t NullInStream::size(){
	return 0;
}

NullOutStream::NullOutStream(){}
NullOutStream::~NullOutStream(){}
void NullOutStream::write(int toW){}
void NullOutStream::write(const char* toW, uintptr_t numW){}
void NullOutStream::close(){
	isClosed = 1;
}

MemoryInStream::MemoryInStream(uintptr_t numBytes, const char* theBytes){
	numBts = numBytes;
	theBts = theBytes;
	nextBt = 0;
}
MemoryInStream::~MemoryInStream(){}
uintmax_t MemoryInStream::tell(){
	return nextBt;
}
void MemoryInStream::seek(uintmax_t toLoc){
	nextBt = toLoc;
}
uintmax_t MemoryInStream::size(){
	return numBts;
}
int MemoryInStream::read(){
	if(nextBt >= numBts){
		return -1;
	}
	int toRet = 0x00FF & theBts[nextBt];
	nextBt++;
	return toRet;
}
uintptr_t MemoryInStream::read(char* toR, uintptr_t numR){
	uintptr_t numLeft = numBts - nextBt;
	uintptr_t numGet = std::min(numLeft, numR);
	memcpy(toR, theBts + nextBt, numGet);
	nextBt += numGet;
	return numGet;
}
void MemoryInStream::close(){isClosed = 1;}

MemoryOutStream::MemoryOutStream(){}
MemoryOutStream::~MemoryOutStream(){}
void MemoryOutStream::write(int toW){
	saveArea.push_back(toW);
}
void MemoryOutStream::write(const char* toW, uintptr_t numW){
	saveArea.insert(saveArea.end(), toW, toW + numW);
}
void MemoryOutStream::close(){isClosed = 1;}



