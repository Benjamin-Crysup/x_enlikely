//condition variables added in Vista
#define _WIN32_WINNT 0x0600
#include "whodun_oshook.h"

#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

using namespace whodun;

const char* whodun::filePathSeparator = "\\";

bool whodun::fileExists(const char* fileName){
	DWORD dwAttrib = GetFileAttributes(fileName);
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void whodun::fileKill(const char* fileName){
	DeleteFile(fileName);
}

intmax_t whodun::fileGetSize(const char* fileName){
	WIN32_FILE_ATTRIBUTE_DATA storeAtts;
	if(GetFileAttributesEx(fileName, GetFileExInfoStandard, &storeAtts)){
		long long int lowDW = storeAtts.nFileSizeLow;
		long long int higDW = storeAtts.nFileSizeHigh;
		return (higDW << (8*sizeof(DWORD))) + lowDW;
	}
	else{
		return -1;
	}
}

intmax_t whodun::fileTellFutureProof(FILE* stream){
	return _ftelli64(stream);
}

int whodun::fileSeekFutureProof(FILE* stream, intmax_t offset, int whence){
	return _fseeki64(stream, offset, whence);
}

bool whodun::directoryExists(const char* dirName){
	DWORD dwAttrib = GetFileAttributes(dirName);
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int whodun::directoryCreate(const char* dirName){
	return !CreateDirectory(dirName, 0);
}

void whodun::directoryKill(const char* dirName){
	RemoveDirectory(dirName);
}

/**Save stuff.*/
class WhodunDirectorySearch{
public:
	/**Set up an empty search.*/
	WhodunDirectorySearch(){}
	/**Clean up.*/
	~WhodunDirectorySearch(){}
	/**Whether there's still one in the chamber.*/
	int haveLeftover;
	/**The name of the thing being searched through.*/
	std::string searchName;
	/**Storage for find data.*/
	WIN32_FIND_DATA findDat;
	/**The handle to the directory.*/
	HANDLE hFind;
};

void* whodun::directoryOpen(const char* dirName){
	WhodunDirectorySearch* toRet = new WhodunDirectorySearch();
	toRet->searchName.append(dirName);
	toRet->searchName.append("\\*");
	toRet->hFind = FindFirstFile(toRet->searchName.c_str(), &(toRet->findDat));
	if(toRet->hFind == INVALID_HANDLE_VALUE){
		delete(toRet);
		return 0;
	}
	toRet->haveLeftover = 1;
	return toRet;
}

int whodun::directoryReadNext(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	if(toRet->haveLeftover){
		toRet->haveLeftover = 0;
		return 1;
	}
	return FindNextFile(toRet->hFind, &(toRet->findDat));
}

int whodun::directoryCurIsDir(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	return (toRet->findDat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

const char* whodun::directoryCurName(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	return toRet->findDat.cFileName;
}

void whodun::directoryClose(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	FindClose(toRet->hFind);
	delete(toRet);
}

/**Passable info for a thread.*/
typedef struct{
	/**The function.*/
	void(*callFun)(void*);
	/**The argument to the function.*/
	void* callUniform;
} ThreadWorker;

/**
 * Go between for windows threads.
 * @param threadParam The ThreadWorker info.
 * return The thread result.
 */
DWORD WINAPI whodun_mainThreadFunc(LPVOID threadParam){
	ThreadWorker* curInfo = (ThreadWorker*)threadParam;
	void(*callFun)(void*) = curInfo->callFun;
	void* callUni = curInfo->callUniform;
	free(curInfo);
	callFun(callUni);
	return 0;
}

void* whodun::threadStart(void(*callFun)(void*), void* callUniform){
	ThreadWorker* curInfo = (ThreadWorker*)malloc(sizeof(ThreadWorker));
	curInfo->callFun = callFun;
	curInfo->callUniform = callUniform;
	HANDLE* curHand = (HANDLE*)malloc(sizeof(HANDLE));
	*curHand = CreateThread(NULL, 0, whodun_mainThreadFunc, curInfo, 0, 0);
	return curHand;
}

void whodun::threadJoin(void* tHandle){
	HANDLE* tHand = (HANDLE*)tHandle;
	WaitForSingleObject(*tHand, INFINITE);
	CloseHandle(*tHand);
	free(tHand);
}

void* whodun::mutexMake(){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSectionAndSpinCount(curSect, 3);
	return curSect;
}

void whodun::mutexLock(void* toLock){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toLock;
	EnterCriticalSection(curSect);
}

void whodun::mutexUnlock(void* toUnlock){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toUnlock;
	LeaveCriticalSection(curSect);
}

void whodun::mutexKill(void* toKill){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)toKill;
	DeleteCriticalSection(curSect);
	free(curSect);
}

void* whodun::conditionMake(void* forMutex){
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));
	InitializeConditionVariable(curCond);
	return curCond;
}

void whodun::conditionWait(void* forMutex, void* forCondition){
	CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	SleepConditionVariableCS(curCond, curSect, INFINITE);
}

void whodun::conditionSignal(void* forMutex, void* forCondition){
	//CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	WakeConditionVariable(curCond);
}

void whodun::conditionBroadcast(void* forMutex, void* forCondition){
	//CRITICAL_SECTION* curSect = (CRITICAL_SECTION*)forMutex;
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	WakeAllConditionVariable(curCond);
}

void whodun::conditionKill(void* forCondition){
	CONDITION_VARIABLE* curCond = (CONDITION_VARIABLE*)forCondition;
	free(curCond);
}

typedef struct{
	HMODULE dllMod;
} DLLibStruct;

void* whodun::dllLoadIn(const char* loadFrom){
	HMODULE dllMod = LoadLibrary(loadFrom);
	if(dllMod){
		DLLibStruct* toRet = (DLLibStruct*)malloc(sizeof(DLLibStruct));
		toRet->dllMod = dllMod;
		return toRet;
	}
	return 0;
}

void* whodun::dllGetLocation(void* fromDLL, const char* locName){
	DLLibStruct* toRet = (DLLibStruct*)fromDLL;
	return (void*)(GetProcAddress(toRet->dllMod, locName));
}

void whodun::dllUnload(void* toKill){
	DLLibStruct* toRet = (DLLibStruct*)toKill;
	FreeLibrary(toRet->dllMod);
	free(toRet);
}

typedef struct{
	/**The actual process.*/
	PROCESS_INFORMATION runningProc;
	/**The handle for stdin.*/
	HANDLE stdIn;
	/**The handle for stdout.*/
	HANDLE stdOut;
	/**The handle for stderr.*/
	HANDLE stdErr;
	/**Whether stdin need killing.*/
	int inNeedKill;
	/**Whether this thing has a stdout.*/
	int haveOut;
	/**Whether this thing has a stderr.*/
	int haveErr;
} WindowsSubprocess;

/**Lock when creating a subprocess due to handles.*/
OSMutex whodunHandleLock;

#define CSTREND(ofStr) (ofStr + strlen(ofStr))

void* whodun::subprocStart(SubProcessArguments* theArgs){
	void* toRet = 0;
	whodunHandleLock.lock();
	{
		WindowsSubprocess* retProc = (WindowsSubprocess*)malloc(sizeof(WindowsSubprocess));
		memset(retProc, 0, sizeof(WindowsSubprocess));
		//make the full command
		std::vector<char> fullCommand;
		fullCommand.push_back('"');
		fullCommand.insert(fullCommand.end(), theArgs->progName, CSTREND(theArgs->progName));
		fullCommand.push_back('"');
		for(uintptr_t i = 0; i<theArgs->numArgs; i++){
			fullCommand.push_back(' ');
			fullCommand.push_back('"');
			char* curArg = theArgs->progArgs[i];
			while(*curArg){
				if(*curArg == '"'){
					fullCommand.push_back('^');
					fullCommand.push_back('"');
				}
				else if(*curArg == '^'){
					fullCommand.push_back('^');
					fullCommand.push_back('^');
				}
				else{
					fullCommand.push_back(*curArg);
				}
				curArg++;
			}
			fullCommand.push_back('"');
		}
		fullCommand.push_back(0);
		//set up info
		STARTUPINFO procOpts;
		memset(&procOpts, 0, sizeof(STARTUPINFO));
		procOpts.cb = sizeof(STARTUPINFO);
		//set up redirection
		SECURITY_ATTRIBUTES handOpts;
		handOpts.nLength = sizeof(SECURITY_ATTRIBUTES);
		handOpts.bInheritHandle = TRUE;
		handOpts.lpSecurityDescriptor = NULL;
		procOpts.dwFlags = STARTF_USESTDHANDLES;
		HANDLE stdinRead;
		int needKillSIW = 0;
		#define STDIN_ERROR_DO free(retProc); goto endOfLine;
		switch(theArgs->stdinMeans){
			case WHODUN_SUBPROCESS_NULL:{
				stdinRead = CreateFile("NUL", GENERIC_READ, 0, &handOpts, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stdinRead == INVALID_HANDLE_VALUE){STDIN_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_FILE:{
				stdinRead = CreateFile(theArgs->stdinFile, GENERIC_READ, 0, &handOpts, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stdinRead == INVALID_HANDLE_VALUE){STDIN_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_PIPE:{
				needKillSIW = 1;
				int goodP = CreatePipe(&stdinRead, &(retProc->stdIn), &handOpts, 0);
				if(!goodP){STDIN_ERROR_DO}
				goodP = SetHandleInformation(&(retProc->stdIn), HANDLE_FLAG_INHERIT, 0);
				if(!goodP){STDIN_ERROR_DO}
				}break;
			default:
				throw std::runtime_error("Da fuq?");
		};
		HANDLE stdoutWrite;
		int needKillSOW = 0;
		#define STDOUT_ERROR_DO CloseHandle(stdinRead); if(needKillSIW){CloseHandle(retProc->stdIn);} free(retProc); goto endOfLine;
		switch(theArgs->stdoutMeans){
			case WHODUN_SUBPROCESS_NULL:{
				stdoutWrite = CreateFile("NUL", GENERIC_WRITE, 0, &handOpts, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stdoutWrite == INVALID_HANDLE_VALUE){STDOUT_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_FILE:{
				stdoutWrite = CreateFile(theArgs->stdoutFile, GENERIC_WRITE, 0, &handOpts, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stdoutWrite == INVALID_HANDLE_VALUE){STDOUT_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_PIPE:{
				needKillSOW = 1;
				int goodP = CreatePipe(&(retProc->stdOut), &stdoutWrite, &handOpts, 0);
				if(!goodP){STDOUT_ERROR_DO}
				goodP = SetHandleInformation(&(retProc->stdOut), HANDLE_FLAG_INHERIT, 0);
				if(!goodP){STDOUT_ERROR_DO}
				}break;
			default:
				throw std::runtime_error("Da fuq?");
		};
		HANDLE stderrWrite;
		int needKillSEW = 0;
		#define STDERR_ERROR_DO CloseHandle(stdinRead); if(needKillSIW){CloseHandle(retProc->stdIn);} CloseHandle(stdoutWrite); if(needKillSOW){CloseHandle(retProc->stdOut);} free(retProc); goto endOfLine;
		switch(theArgs->stderrMeans){
			case WHODUN_SUBPROCESS_NULL:{
				stderrWrite = CreateFile("NUL", GENERIC_WRITE, 0, &handOpts, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stderrWrite == INVALID_HANDLE_VALUE){STDERR_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_FILE:{
				stderrWrite = CreateFile(theArgs->stderrFile, GENERIC_WRITE, 0, &handOpts, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(stderrWrite == INVALID_HANDLE_VALUE){STDERR_ERROR_DO}
				}break;
			case WHODUN_SUBPROCESS_PIPE:{
				needKillSEW = 1;
				int goodP = CreatePipe(&(retProc->stdErr), &stderrWrite, &handOpts, 0);
				if(!goodP){STDERR_ERROR_DO}
				goodP = SetHandleInformation(&(retProc->stdErr), HANDLE_FLAG_INHERIT, 0);
				if(!goodP){STDERR_ERROR_DO}
				}break;
			default:
				throw std::runtime_error("Da fuq?");
		};
		procOpts.hStdInput = stdinRead;
		procOpts.hStdOutput = stdoutWrite;
		procOpts.hStdError = stderrWrite;
		bool goodC = CreateProcess(NULL, &(fullCommand[0]), NULL, NULL, TRUE, 0, NULL, NULL, &procOpts, &(retProc->runningProc));
		CloseHandle(stdinRead);
		CloseHandle(stdoutWrite);
		CloseHandle(stderrWrite);
		if(!goodC){
			if(needKillSIW){ CloseHandle(retProc->stdIn); }
			if(needKillSOW){ CloseHandle(retProc->stdOut); }
			if(needKillSEW){ CloseHandle(retProc->stdErr); }
			free(retProc);
			goto endOfLine;
		}
		toRet = retProc;
	}
	endOfLine:
	whodunHandleLock.unlock();
	return toRet;
}

void whodun::subprocInFinish(void* theProc){
	WindowsSubprocess* theP = (WindowsSubprocess*)theProc;
	CloseHandle(theP->stdIn);
	theP->inNeedKill = 0;
}

void whodun::subprocInWrite(void* theProc, const char* toW, uintptr_t numW){
	WindowsSubprocess* theP = (WindowsSubprocess*)theProc;
	long unsigned int numWritten = 0;
	WriteFile(theP->stdIn, toW, numW, &numWritten, NULL);
}

uintptr_t whodun::subprocOutRead(void* theProc, char* toR, uintptr_t numR){
	WindowsSubprocess* theP = (WindowsSubprocess*)theProc;
	long unsigned int numRead = 0;
	ReadFile(theP->stdOut, toR, numR, &numRead, NULL);
	return numRead;
}

uintptr_t whodun::subprocErrRead(void* theProc, char* toR, uintptr_t numR){
	WindowsSubprocess* theP = (WindowsSubprocess*)theProc;
	long unsigned int numRead = 0;
	ReadFile(theP->stdErr, toR, numR, &numRead, NULL);
	return numRead;
}

int whodun::subprocWait(void* theProc){
	WindowsSubprocess* theP = (WindowsSubprocess*)theProc;
	if(theP->inNeedKill){ CloseHandle(theP->stdIn); }
	if(theP->haveOut){ CloseHandle(theP->stdOut); }
	if(theP->haveErr){ CloseHandle(theP->stdErr); }
	WaitForSingleObject(theP->runningProc.hProcess, INFINITE);
	DWORD retCode;
	GetExitCodeProcess(theP->runningProc.hProcess, &retCode);
	CloseHandle(theP->runningProc.hProcess);
	CloseHandle(theP->runningProc.hThread);
	free(theP);
	return retCode;
}



