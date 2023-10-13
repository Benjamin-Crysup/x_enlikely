#include "whodun_container.h"

using namespace whodun;

SizePtrString whodun::toSizePtr(StructVector<char>* toPackage){
	SizePtrString toRet;
		toRet.len = toPackage->size();
		toRet.txt = (*toPackage)[0];
	return toRet;
}

