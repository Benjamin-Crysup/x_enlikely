#include "whodun_ermac.h"

#include <string.h>
#include <algorithm>

namespace whodun {

/**
 * Pack an error description.
 * @param level The error level: should only be warning or worse.
 * @param simpleDescription A simple (one word) description of the error.
 * @param sourceFile The code file the error occured at. Should be a safe pointer.
 * @param sourceLine The line in that file.
 * @param description A full description of the error.
 * @param numExtra The number of extra pieces of information.
 * @param theExtra The extra information.
 * @param curFrame The place to pack it.
 */
void packErrorDescription(int level, const char* simpleDescription, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** theExtra, ErrorDescription* curFrame);

};

using namespace whodun;

#define WHODUN_ERROR_ARRENTRIES 2+WHODUN_ERROR_MAXEXTRA

void whodun::packErrorDescription(int level, const char* simpleDescription, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** theExtra, ErrorDescription* curFrame){
	char* curArr = curFrame->strBuffer;
	int arrLeft = WHODUN_ERROR_ARRSIZE - WHODUN_ERROR_ARRENTRIES;
	int curEntLen;
	#define PACK_ARR_ENTRY(toVar,fromVar) \
		curFrame->toVar = curArr - curFrame->strBuffer;\
		curEntLen = std::min((int)strlen(fromVar), arrLeft);\
		memcpy(curArr, fromVar, curEntLen);\
		curArr[curEntLen] = 0;\
		curArr += (curEntLen+1);\
		arrLeft -= (curEntLen+1);
	curFrame->level = level;
	PACK_ARR_ENTRY(simpleDescOff,simpleDescription)
	curFrame->codeSourceFile = sourceFile;
	curFrame->codeLine = sourceLine;
	PACK_ARR_ENTRY(descriptionOff,description)
	int doExtra = std::min(numExtra,WHODUN_ERROR_MAXEXTRA);
	curFrame->numExtra = doExtra;
	for(int i = 0; i<doExtra; i++){
		PACK_ARR_ENTRY(extraOff[i],theExtra[i])
	}
}

WhodunError::WhodunError(){
	numStack = 0;
}
WhodunError::WhodunError(int level, const char* simpleDescription, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** theExtra){
	numStack = 1;
	packErrorDescription(level, simpleDescription, sourceFile, sourceLine, description, numExtra, theExtra, saveErrors);
}
WhodunError::WhodunError(const ErrorDescription* toCopy){
	numStack = 1;
	saveErrors[0] = *toCopy;
}
WhodunError::WhodunError(const WhodunError& toCopy){
	numStack = toCopy.numStack;
	for(int i = 0; i<numStack; i++){
		saveErrors[i] = toCopy.saveErrors[i];
	}
}
WhodunError::WhodunError(const WhodunError& toCopy, const char* sourceFile, int sourceLine){
	//copy
	numStack = toCopy.numStack;
	for(int i = 0; i<numStack; i++){
		saveErrors[i] = toCopy.saveErrors[i];
	}
	//make space for the new level
	ErrorDescription* curFrame;
	if(numStack < WHODUN_ERROR_MAXSTACK){
		curFrame = saveErrors + numStack;
		numStack++;
	}
	else{
		int midStack = (numStack%2) + (numStack/2);
		if(midStack >= WHODUN_ERROR_MAXSTACK){ return; }
		for(int i = midStack+1; i<numStack; i++){
			saveErrors[i-1] = saveErrors[i];
		}
		curFrame = saveErrors + (numStack-1);
	}
	//and add it
	packErrorDescription(WHODUN_ERROR_LEVEL_SOURCE, "IS", sourceFile, sourceLine, "FROM", 0, 0, curFrame);
}
WhodunError::WhodunError(const std::exception& toCopy, const char* sourceFile, int sourceLine){
	numStack = 1;
	packErrorDescription(WHODUN_ERROR_LEVEL_ERROR, "EXCEPT", sourceFile, sourceLine, toCopy.what(), 0, 0, saveErrors);
}
WhodunError::~WhodunError(){}
WhodunError& WhodunError::operator=(const WhodunError& toCopy){
	numStack = toCopy.numStack;
	for(int i = 0; i<numStack; i++){
		saveErrors[i] = toCopy.saveErrors[i];
	}
	return *this;
}
const char* WhodunError::what() const throw(){
	return saveErrors[0].strBuffer + saveErrors[0].descriptionOff;
}

ErrorLog::ErrorLog(){}
ErrorLog::~ErrorLog(){}
void ErrorLog::logError(const WhodunError& toLog){
	useMut.lock();
	try{
		int i = toLog.numStack;
		while(i){
			i--;
			spitError(&(toLog.saveErrors[i]));
		}
	}
	catch(std::exception& errE){
		useMut.unlock();
		throw;
	}
	useMut.unlock();
}
void ErrorLog::logError(const ErrorDescription* toLog){
	useMut.lock();
	try{
		spitError(toLog);
	}
	catch(std::exception& errE){
		useMut.unlock();
		throw;
	}
	useMut.unlock();
}
void ErrorLog::logError(int level, const char* simpleDesc, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** extras){
	useMut.lock();
	try{
		spitError(level, simpleDesc, sourceFile, sourceLine, description, numExtra, extras);
	}
	catch(std::exception& errE){
		useMut.unlock();
		throw;
	}
	useMut.unlock();
}
void ErrorLog::spitError(const ErrorDescription* toLog){
	#define E_GET_STR(offName) (toLog->strBuffer + toLog->offName)
	const char* passExt[WHODUN_ERROR_MAXEXTRA];
	for(int i = 0; i<toLog->numExtra; i++){
		passExt[i] = E_GET_STR(extraOff[i]);
	}
	spitError(toLog->level, E_GET_STR(simpleDescOff), toLog->codeSourceFile, toLog->codeLine, E_GET_STR(descriptionOff), toLog->numExtra, passExt);
}

StreamErrorLog::StreamErrorLog(OutStream* targetStream){
	tgtStr = targetStream;
}
StreamErrorLog::~StreamErrorLog(){}
void StreamErrorLog::spitError(int level, const char* simpleDesc, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** extras){
	const char* levelStr = "DAFUQ";
	switch(level){
		case WHODUN_ERROR_LEVEL_FATAL:
			levelStr = "FATAL"; break;
		case WHODUN_ERROR_LEVEL_ERROR:
			levelStr = "ERROR"; break;
		case WHODUN_ERROR_LEVEL_WARNING:
			levelStr = "WARNING"; break;
		case WHODUN_ERROR_LEVEL_INFO:
			levelStr = "INFO"; break;
		case WHODUN_ERROR_LEVEL_ACCOUNT:
			levelStr = "ACCOUNT"; break;
		case WHODUN_ERROR_LEVEL_DEBUG:
			levelStr = "DEBUG"; break;
		case WHODUN_ERROR_LEVEL_SOURCE:
			levelStr = "SOURCE"; break;
		default:
			levelStr = "DAFUQ";
	};
	char codeLineBuff[8*sizeof(uintmax_t)+8]; sprintf(codeLineBuff, "%ju", (uintmax_t)sourceLine);
	tgtStr->write(levelStr);
	tgtStr->write('\t');
	tgtStr->write(simpleDesc);
	tgtStr->write('\t');
	tgtStr->write(sourceFile);
	tgtStr->write('\t');
	tgtStr->write(codeLineBuff);
	tgtStr->write('\t');
	const char* curDump;
	#define DUMP_SOMETHING \
	while(*curDump){\
		int numOut = strcspn(curDump, "\t\r\n");\
		if(numOut){\
			tgtStr->write(curDump, numOut);\
		}\
		else{\
			tgtStr->write('\\');\
			if(*curDump == '\t'){tgtStr->write('t');}\
			if(*curDump == '\r'){tgtStr->write('r');}\
			if(*curDump == '\n'){tgtStr->write('n');}\
			numOut = 1;\
		}\
		curDump += numOut;\
	}
	curDump = description; DUMP_SOMETHING
	for(int i = 0; i<numExtra; i++){
		tgtStr->write('\t');
		curDump = extras[i];
		DUMP_SOMETHING
	}
	tgtStr->write('\n');
}


