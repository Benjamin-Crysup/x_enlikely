#include "whodun_string.h"

#include <math.h>
#include <string.h>
#include <exception>

using namespace whodun;

SizePtrString whodun::toSizePtr(const char* toPackage){
	SizePtrString toRet;
		toRet.len = strlen(toPackage);
		toRet.txt = (char*)toPackage;
	return toRet;
}
SizePtrString whodun::toSizePtr(std::string* toPackage){
	SizePtrString toRet;
		toRet.len = toPackage->size();
		toRet.txt = (char*)(toPackage->c_str());
	return toRet;
}
SizePtrString whodun::toSizePtr(std::vector<char>* toPackage){
	SizePtrString toRet;
		toRet.len = toPackage->size();
		toRet.txt = toRet.len ? &((*toPackage)[0]) : (char*)0;
	return toRet;
}
SizePtrString whodun::toSizePtr(uintptr_t packLen, char* toPackage){
	SizePtrString toRet;
		toRet.len = packLen;
		toRet.txt = toPackage;
	return toRet;
}
intptr_t whodun::parseInt(SizePtrString toParse){
	int needFlip = 0;
	intptr_t toRet = 0;
	SizePtrString curParse = toParse;
	if(curParse.len == 0){ throw std::runtime_error("Empty string passed to parse int."); }
	char leadChar = curParse.txt[0];
	if(leadChar == '+'){ curParse.len--; curParse.txt++; }
	else if(leadChar == '-'){ needFlip = 1; curParse.len--; curParse.txt++; }
	if(curParse.len == 0){ throw std::runtime_error("No digits passed to parse int."); }
	while(curParse.len){
		int curV = curParse.txt[0];
		if((curV < '0') || (curV > '9')){ throw std::runtime_error("Invalid digit passed to parse int."); }
		toRet = (10*toRet) + (curV - '0');
		curParse.txt++;
		curParse.len--;
	}
	if(needFlip){ toRet = -toRet; }
	return toRet;
}
double whodun::parseFloat(SizePtrString toParse){
	int needFlip = 0;
	double manval = 0;
	intptr_t expVal = 0;
	SizePtrString curParse = toParse;
	if(curParse.len == 0){ throw std::runtime_error("Empty string passed to parse float."); }
	//get the sign out of the way
		char leadChar = curParse.txt[0];
		if(leadChar == '+'){ curParse.len--; curParse.txt++; }
		else if(leadChar == '-'){ needFlip = 1; curParse.len--; curParse.txt++; }
		if(curParse.len == 0){ throw std::runtime_error("No digits passed to parse float."); }
	//handle special values
		if(curParse == toSizePtr("nan")){
			return (needFlip ? -0.0 : 0.0) / 0.0;
		}
		if(curParse == toSizePtr("inf")){
			return (needFlip ? -1.0 : 1.0) / 0.0;
		}
	//read to the decimal
		while(curParse.len){
			int curV = curParse.txt[0];
			if((curV == '.') || (curV == 'e') || (curV == 'E')){ break; }
			if((curV < '0') || (curV > '9')){ throw std::runtime_error("Invalid digit passed to parse float."); }
			manval = (10*manval) + curV;
		}
	//read off the fractional part
		if(curParse.len && (curParse.txt[0] == '.')){
			curParse.len--;
			curParse.txt++;
			double curMulV = 0.1;
			while(curParse.len){
				int curV = curParse.txt[0];
				if((curV == 'e') || (curV == 'E')){ break; }
				if((curV < '0') || (curV > '9')){ throw std::runtime_error("Invalid digit passed to parse float."); }
				manval = manval + (curMulV * curV);
				curMulV = 0.1 * curMulV;
			}
		}
	//read the exponent
		if(curParse.len){
			curParse.len--;
			curParse.txt++;
			expVal = parseInt(curParse);
		}
	//return the result
		return (needFlip ? -1.0 : 1.0) * manval * pow(10.0,expVal);
}

bool whodun::operator < (const SizePtrString& strA, const SizePtrString& strB){
	uintptr_t compTo = std::min(strA.len, strB.len);
	int compV = memcmp(strA.txt, strB.txt, compTo);
	if(compV){ return compV < 0; }
	return (strA.len < strB.len);
}
bool whodun::operator > (const SizePtrString& strA, const SizePtrString& strB){
	return strB < strA;
}
bool whodun::operator <= (const SizePtrString& strA, const SizePtrString& strB){
	uintptr_t compTo = std::min(strA.len, strB.len);
	int compV = memcmp(strA.txt, strB.txt, compTo);
	if(compV){ return compV < 0; }
	return (strA.len <= strB.len);
}
bool whodun::operator >= (const SizePtrString& strA, const SizePtrString& strB){
	return strB <= strA;
}
bool whodun::operator == (const SizePtrString& strA, const SizePtrString& strB){
	if(strA.len != strB.len){ return false; }
	return memcmp(strA.txt, strB.txt, strA.len) == 0;
}
bool whodun::operator != (const SizePtrString& strA, const SizePtrString& strB){
	return !(strA == strB);
}
std::ostream& whodun::operator<<(std::ostream& os, SizePtrString const & toOut){
	os.write(toOut.txt, toOut.len);
	return os;
}

MemoryShuttler::MemoryShuttler(){}
MemoryShuttler::~MemoryShuttler(){}

StandardMemoryShuttler::StandardMemoryShuttler(){}
StandardMemoryShuttler::~StandardMemoryShuttler(){}
void StandardMemoryShuttler::memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum){
	::memcpy(cpyTo, cpyFrom, copyNum);
}
void StandardMemoryShuttler::memset(void* setP, int value, size_t numBts){
	::memset(setP, value, numBts);
}
void StandardMemoryShuttler::memswap(char* arrA, char* arrB, size_t numBts){
	::memswap(arrA, arrB, numBts);
}

MemorySearcher::MemorySearcher(){}
MemorySearcher::~MemorySearcher(){}
int MemorySearcher::memendswith(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB1 < numB2){ return 0; }
	return memcmp(str1 + (numB1 - numB2), str2, numB2) == 0;
}
int MemorySearcher::memstartswith(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB1 < numB2){ return 0; }
	return memcmp(str1, str2, numB2) == 0;
}
int MemorySearcher::memendswith(SizePtrString str1, SizePtrString str2){
	return memendswith(str1.txt, str1.len, str2.txt, str2.len);
}
int MemorySearcher::memstartswith(SizePtrString str1, SizePtrString str2){
	return memstartswith(str1.txt, str1.len, str2.txt, str2.len);
}
SizePtrString MemorySearcher::trim(SizePtrString strMain, SizePtrString toRem){
	SizePtrString toRet = strMain;
	while(memstartswith(toRet, toRem)){
		toRet.txt += toRem.len;
		toRet.len -= toRem.len;
	}
	while(memendswith(toRet, toRem)){
		toRet.len -= toRem.len;
	}
	return toRet;
}
SizePtrString MemorySearcher::trim(SizePtrString strMain, uintptr_t numRem, SizePtrString* toRem){
	SizePtrString toRet = strMain;
	
	needGoAgain:
	for(uintptr_t i = 0; i<numRem; i++){
		SizePtrString curT = trim(toRet, toRem[i]);
		if(curT.len != toRet.len){
			toRet = curT;
			goto needGoAgain;
		}
	}
	
	return toRet;
}
SizePtrString MemorySearcher::trim(SizePtrString strMain){
	char killCs[] = {' ','\t','\r','\n'};
	SizePtrString comRem[4];
	for(uintptr_t i = 0; i<4; i++){
		comRem[i].txt = killCs + i;
		comRem[i].len = 1;
	}
	return trim(strMain, 4, comRem);
}
std::pair<SizePtrString,SizePtrString> MemorySearcher::split(SizePtrString toSplit, SizePtrString toSplitOn){
	std::pair<SizePtrString,SizePtrString> toRet;
	char* foundIt = memmem(toSplit.txt, toSplit.len, toSplitOn.txt, toSplitOn.len);
	if(foundIt){
		toRet.first.txt = toSplit.txt;
		toRet.first.len = foundIt - toSplit.txt;
		toRet.second.txt = foundIt + toSplitOn.len;
		toRet.second.len = (toSplit.txt + toSplit.len) - toRet.second.txt;
	}
	else{
		toRet.first = toSplit;
		toRet.second.txt = 0;
		toRet.second.len = 0;
	}
	return toRet;
}

StandardMemorySearcher::StandardMemorySearcher(){}
StandardMemorySearcher::~StandardMemorySearcher(){}
int StandardMemorySearcher::memcmp(const char* str1, const char* str2, size_t num){
	return ::memcmp(str1, str2, num);
}
void* StandardMemorySearcher::memchr(const void* str1, int value, size_t num){
	return (void*)::memchr(str1, value, num);
}
size_t StandardMemorySearcher::memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	return ::memcspn(str1, numB1, str2, numB2);
}
size_t StandardMemorySearcher::memspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	return ::memspn(str1, numB1, str2, numB2);
}
char* StandardMemorySearcher::memmem(const char* str1, size_t numB1, const char* str2, size_t numB2){
	return (char*)::memmem(str1, numB1, str2, numB2);
}

BytePacker::BytePacker(){}
BytePacker::BytePacker(char* toFill){
	target = toFill;
}
BytePacker::~BytePacker(){}
void BytePacker::retarget(char* toFill){
	target = toFill;
}
void BytePacker::skip(uintptr_t numSkip){
	target += numSkip;
}

ByteUnpacker::ByteUnpacker(){}
ByteUnpacker::ByteUnpacker(char* toFill){
	target = toFill;
}
ByteUnpacker::~ByteUnpacker(){}
void ByteUnpacker::retarget(char* toFill){
	target = toFill;
}
void ByteUnpacker::skip(uintptr_t numSkip){
	target += numSkip;
}



