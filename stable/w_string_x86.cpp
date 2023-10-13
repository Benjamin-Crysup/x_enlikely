#include "whodun_string.h"

using namespace whodun;

size_t whodun::memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				return curCS;
			}
		}
	}
	return numB1;
}

size_t whodun::memspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				goto wasGut;
			}
		}
		return curCS;
		wasGut:
	}
	return numB1;
}

char* whodun::memmem(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB2 > numB1){
		return 0;
	}
	size_t maxCheck = (numB1 - numB2) + 1;
	for(size_t curCS = 0; curCS < maxCheck; curCS++){
		if(memcmp(str1+curCS, str2, numB2) == 0){
			return str1+curCS;
		}
	}
	return 0;
}

void whodun::memswap(char* arrA, char* arrB, size_t numBts){
	for(int i = 0; i<numBts; i++){
		char tmp = arrA[i];
		arrA[i] = arrB[i];
		arrB[i] = tmp;
	}
}

void BytePacker::packBE64(uint64_t toPack){
	target[7] = toPack & 0x00FF;
	target[6] = (toPack>>8) & 0x00FF;
	target[5] = (toPack>>16) & 0x00FF;
	target[4] = (toPack>>24) & 0x00FF;
	target[3] = (toPack>>32) & 0x00FF;
	target[2] = (toPack>>40) & 0x00FF;
	target[1] = (toPack>>48) & 0x00FF;
	target[0] = (toPack>>56) & 0x00FF;
	target += 8;
}
void BytePacker::packBE32(uint32_t toPack){
	target[3] = toPack & 0x00FF;
	target[2] = (toPack>>8) & 0x00FF;
	target[1] = (toPack>>16) & 0x00FF;
	target[0] = (toPack>>24) & 0x00FF;
	target += 4;
}
void BytePacker::packBE16(uint16_t toPack){
	target[1] = toPack & 0x00FF;
	target[0] = (toPack>>8) & 0x00FF;
	target += 2;
}
void BytePacker::packBEDbl(double toPack){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	packBE64(tmpSave.saveI);
}
void BytePacker::packBEFlt(float toPack){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	packBE32(tmpSave.saveI);
}
void BytePacker::packLE64(uint64_t toPack){
	target[0] = toPack & 0x00FF;
	target[1] = (toPack>>8) & 0x00FF;
	target[2] = (toPack>>16) & 0x00FF;
	target[3] = (toPack>>24) & 0x00FF;
	target[4] = (toPack>>32) & 0x00FF;
	target[5] = (toPack>>40) & 0x00FF;
	target[6] = (toPack>>48) & 0x00FF;
	target[7] = (toPack>>56) & 0x00FF;
	target += 8;
}
void BytePacker::packLE32(uint32_t toPack){
	target[0] = toPack & 0x00FF;
	target[1] = (toPack>>8) & 0x00FF;
	target[2] = (toPack>>16) & 0x00FF;
	target[3] = (toPack>>24) & 0x00FF;
	target += 4;
}
void BytePacker::packLE16(uint16_t toPack){
	target[0] = toPack & 0x00FF;
	target[1] = (toPack>>8) & 0x00FF;
	target += 2;
}
void BytePacker::packLEDbl(double toPack){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	packLE64(tmpSave.saveI);
}
void BytePacker::packLEFlt(float toPack){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	packLE32(tmpSave.saveI);
}

uint64_t ByteUnpacker::unpackBE64(){
	uint64_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[0]);
	toRet = (toRet << 8) + (0x00FF & target[1]);
	toRet = (toRet << 8) + (0x00FF & target[2]);
	toRet = (toRet << 8) + (0x00FF & target[3]);
	toRet = (toRet << 8) + (0x00FF & target[4]);
	toRet = (toRet << 8) + (0x00FF & target[5]);
	toRet = (toRet << 8) + (0x00FF & target[6]);
	toRet = (toRet << 8) + (0x00FF & target[7]);
	target += 8;
	return toRet;
}
uint32_t ByteUnpacker::unpackBE32(){
	uint32_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[0]);
	toRet = (toRet << 8) + (0x00FF & target[1]);
	toRet = (toRet << 8) + (0x00FF & target[2]);
	toRet = (toRet << 8) + (0x00FF & target[3]);
	target += 4;
	return toRet;
}
uint16_t ByteUnpacker::unpackBE16(){
	uint16_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[0]);
	toRet = (toRet << 8) + (0x00FF & target[1]);
	target += 2;
	return toRet;
}
double ByteUnpacker::unpackBEDbl(){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveI = unpackBE64();
	return tmpSave.saveF;
}
float ByteUnpacker::unpackBEFlt(){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveI = unpackBE32();
	return tmpSave.saveF;
}
uint64_t ByteUnpacker::unpackLE64(){
	uint64_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[7]);
	toRet = (toRet << 8) + (0x00FF & target[6]);
	toRet = (toRet << 8) + (0x00FF & target[5]);
	toRet = (toRet << 8) + (0x00FF & target[4]);
	toRet = (toRet << 8) + (0x00FF & target[3]);
	toRet = (toRet << 8) + (0x00FF & target[2]);
	toRet = (toRet << 8) + (0x00FF & target[1]);
	toRet = (toRet << 8) + (0x00FF & target[0]);
	target += 8;
	return toRet;
}
uint32_t ByteUnpacker::unpackLE32(){
	uint32_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[3]);
	toRet = (toRet << 8) + (0x00FF & target[2]);
	toRet = (toRet << 8) + (0x00FF & target[1]);
	toRet = (toRet << 8) + (0x00FF & target[0]);
	target += 4;
	return toRet;
}
uint16_t ByteUnpacker::unpackLE16(){
	uint16_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & target[1]);
	toRet = (toRet << 8) + (0x00FF & target[0]);
	target += 2;
	return toRet;
}
double ByteUnpacker::unpackLEDbl(){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveI = unpackLE64();
	return tmpSave.saveF;
}
float ByteUnpacker::unpackLEFlt(){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveI = unpackLE32();
	return tmpSave.saveF;
}

