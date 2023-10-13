#include "whodun_stat_randoms.h"

#include <math.h>
#include <stdlib.h>

using namespace whodun;

#define MERSENNE_TWIST_N 624
#define MERSENNE_TWIST_M 397
#define MERSENNE_TWIST_MATRIXA 0x9908B0DFUL
#define MERSENNE_TWIST_UPPERMASK 0x80000000UL
#define MERSENNE_TWIST_LOWERMASK 0x7FFFFFFFUL

RandomGenerator::~RandomGenerator(){}

void RandomGenerator::draw(uintptr_t numDraw, char* toFill){
	for(uintptr_t i = 0; i<numDraw; i++){
		toFill[i] = next();
	}
}

void RandomGenerator::draw(uintptr_t numDraw, double* toFill){
	int64_t* fillAsI = (int64_t*)toFill;
	//fill all with random
		draw(numDraw*sizeof(double), (char*)toFill);
	//convert all to within range [1,2).
		for(uintptr_t i = 0; i<numDraw; i++){
			fillAsI[i] = 0x3FF0000000000000LL | (0x000FFFFFFFFFFFFFLL & fillAsI[i]);
		}
	//subtract 1.0 from each
		for(uintptr_t i = 0; i<numDraw; i++){
			toFill[i] = (toFill[i] - 1.0);
		}
}

void RandomGenerator::draw(uintptr_t numDraw, float* toFill){
	int32_t* fillAsI = (int32_t*)toFill;
	//fill all with random
		draw(numDraw*sizeof(float), (char*)toFill);
	//convert all to within range [1,2).
		for(uintptr_t i = 0; i<numDraw; i++){
			fillAsI[i] = 0x3F800000LL | (0x007FFFFFLL & fillAsI[i]);
		}
	//subtract 1.0 from each
		for(uintptr_t i = 0; i<numDraw; i++){
			toFill[i] = (toFill[i] - 1.0);
		}
}

void RandomGenerator::seedI(uintptr_t seedV){
	uintptr_t seedS = seedSize();
	char* toFill = (char*)malloc(seedS);
	uintptr_t curSV = seedV;
	for(uintptr_t i = 0; i<seedS; i++){
		toFill[i] = 0x00FF & curSV;
		curSV = curSV >> 8;
	}
	seed(toFill);
	free(toFill);
}

MersenneTwisterGenerator::MersenneTwisterGenerator(){
	numPG = 0;
	nextEnt = MERSENNE_TWIST_N;
	haveSeed = false;
}

MersenneTwisterGenerator::~MersenneTwisterGenerator(){
}

uintptr_t MersenneTwisterGenerator::seedSize(){
	return sizeof(uint32_t)*MERSENNE_TWIST_N;
}

void MersenneTwisterGenerator::seed(char* seedV){
	haveSeed = true;
	//initialize the array
		mtarr[0] = 19650218UL;
		for(int i = 1; i<MERSENNE_TWIST_N; i++){
			mtarr[i] = (1812433253UL * (mtarr[i-1] ^ (mtarr[i-1] >> 30)) + i);
		}
	//mangle with the seed
		int i = 1;
		int j = 0;
		char* focSeed = seedV;
		for(int k = MERSENNE_TWIST_N; k; k--){
			uint32_t curKey = 0;
			for(unsigned j = 0; j<sizeof(uint32_t); j++){
				curKey = (curKey << 8) + (0x00FF & *focSeed);
				focSeed++;
			}
			mtarr[i] = (mtarr[i] ^ ((mtarr[i-1] ^ (mtarr[i-1] >> 30)) * 1664525UL)) + curKey + j;
			i++; j++;
			if(i >= MERSENNE_TWIST_N){
				mtarr[0] = mtarr[MERSENNE_TWIST_N-1];
				i = 1;
			}
		}
	//another post-mangle
		for(int k = MERSENNE_TWIST_N-1; k; k--){
			mtarr[i] = (mtarr[i] ^ ((mtarr[i-1] ^ (mtarr[i-1] >> 30)) * 1566083941UL)) - i;
			i++;
			if(i >= MERSENNE_TWIST_N){
				mtarr[0] = mtarr[MERSENNE_TWIST_N-1];
				i = 1;
			}
		}
	//clamp the first entry
		mtarr[0] = 0x80000000UL;
}

char MersenneTwisterGenerator::next(){
	if(numPG){
		char toRet = prevGen & 0x00FF;
		prevGen = prevGen >> 8;
		numPG--;
		return toRet;
	}
	if(nextEnt < MERSENNE_TWIST_N){
		numPG = sizeof(uint32_t);
		prevGen = mtarr[nextEnt];
		prevGen ^= (prevGen >> 11);
		prevGen ^= (prevGen << 7) & 0x9D2C5680UL;
		prevGen ^= (prevGen << 15) & 0xEFC60000UL;
		prevGen ^= (prevGen >> 18);
		nextEnt++;
		return next();
	}
	//generate a new batch
	int kk = 0;
	for(; kk<(MERSENNE_TWIST_N - MERSENNE_TWIST_M); kk++){
		uint32_t y = (mtarr[kk]&MERSENNE_TWIST_UPPERMASK) | (mtarr[kk+1]&MERSENNE_TWIST_LOWERMASK);
		mtarr[kk] = mtarr[kk+MERSENNE_TWIST_M] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	}
	for(; kk<(MERSENNE_TWIST_N-1); kk++){
		uint32_t y = (mtarr[kk]&MERSENNE_TWIST_UPPERMASK) | (mtarr[kk+1]&MERSENNE_TWIST_LOWERMASK);
		mtarr[kk] = mtarr[kk+MERSENNE_TWIST_M-MERSENNE_TWIST_N] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	}
	uint32_t y = (mtarr[MERSENNE_TWIST_N-1]&MERSENNE_TWIST_UPPERMASK) | (mtarr[0]&MERSENNE_TWIST_LOWERMASK);
	mtarr[MERSENNE_TWIST_N-1] = mtarr[MERSENNE_TWIST_M-1] ^ (y >> 1) ^ ((y&0x01) ? MERSENNE_TWIST_MATRIXA : 0);
	nextEnt = 0;
	return next();
}



