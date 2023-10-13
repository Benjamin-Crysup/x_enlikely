
#include "whodun_oshook.h"

#include <vector>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

using namespace whodun;

const char* whodun::filePathSeparator = "/";

bool whodun::fileExists(const char* fileName){
	struct stat dirFo;
	if((stat(fileName, &dirFo)==0) && (S_ISREG(dirFo.st_mode))){
		return 1;
	}
	return 0;
}

void whodun::fileKill(const char* fileName){
	remove(fileName);
}

intmax_t whodun::fileGetSize(const char* fileName){
	struct stat fdatBuff;
	if(stat(fileName, &fdatBuff)){ return -1; }
	return fdatBuff.st_size;
}

intmax_t whodun::fileTellFutureProof(FILE* stream){
	return ftell(stream);
}

int whodun::fileSeekFutureProof(FILE* stream, intmax_t offset, int whence){
	return fseek(stream, offset, whence);
}

bool whodun::directoryExists(const char* dirName){
	struct stat dirFo;
	if((stat(dirName, &dirFo)==0) && (S_ISDIR(dirFo.st_mode))){
		return 1;
	}
	return 0;
}

int whodun::directoryCreate(const char* dirName){
	return mkdir(dirName, 0777);
}

void whodun::directoryKill(const char* dirName){
	rmdir(dirName);
}

/**Save stuff.*/
class WhodunDirectorySearch{
public:
	/**Set up an empty search.*/
	WhodunDirectorySearch(){}
	/**Clean up.*/
	~WhodunDirectorySearch(){}
	/**The open directory.*/
	DIR* dirFo;
	/**The last read structure.*/
	struct dirent* curDFo;
	/**Get information on a file.*/
	struct stat dirSt;
	/**Save a path.*/
	std::vector<char> savePath;
};

void* whodun::directoryOpen(const char* dirName){
	WhodunDirectorySearch* toRet = new WhodunDirectorySearch();
	toRet->dirFo = opendir(dirName);
	if(toRet->dirFo == 0){
		delete(toRet);
		return 0;
	}
	toRet->savePath.insert(toRet->savePath.end(), dirName, dirName + strlen(dirName));
	return toRet;
}

int whodun::directoryReadNext(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	toRet->curDFo = readdir(toRet->dirFo);
	return (toRet->curDFo != 0);
}

int whodun::directoryCurIsDir(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	uintptr_t origLen = toRet->savePath.size();
	toRet->savePath.push_back('/');
	toRet->savePath.insert(toRet->savePath.end(), toRet->curDFo->d_name, toRet->curDFo->d_name + strlen(toRet->curDFo->d_name));
	toRet->savePath.push_back(0);
	int isDir = (stat(&(toRet->savePath[0]), &(toRet->dirSt))==0) && (S_ISDIR(toRet->dirSt.st_mode));
	toRet->savePath.resize(origLen);
	return isDir;
}

const char* whodun::directoryCurName(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	return toRet->curDFo->d_name;
}

void whodun::directoryClose(void* dirHand){
	WhodunDirectorySearch* toRet = (WhodunDirectorySearch*)dirHand;
	closedir(toRet->dirFo);
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
 * Go between for threads.
 * @param threadParam The ThreadWorker info.
 * return The thread result.
 */
void* whodun_mainThreadFunc(void* threadParam){
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
	pthread_t* curHand = (pthread_t*)malloc(sizeof(pthread_t));
	pthread_create(curHand, 0, whodun_mainThreadFunc, curInfo);
	return curHand;
}

void whodun::threadJoin(void* tHandle){
	pthread_t* tHand = (pthread_t*)tHandle;
	pthread_join(*tHand, 0);
	free(tHand);
}

void* whodun::mutexMake(){
	pthread_mutex_t* curSect = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(curSect, 0);
	return curSect;
}

void whodun::mutexLock(void* toLock){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toLock;
	pthread_mutex_lock(curSect);
}

void whodun::mutexUnlock(void* toUnlock){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toUnlock;
	pthread_mutex_unlock(curSect);
}

void whodun::mutexKill(void* toKill){
	pthread_mutex_t* curSect = (pthread_mutex_t*)toKill;
	pthread_mutex_destroy(curSect);
	free(curSect);
}

void* whodun::conditionMake(void* forMutex){
	pthread_cond_t* curCond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(curCond, 0);
	return curCond;
}

void whodun::conditionWait(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
	pthread_mutex_t* curSect = (pthread_mutex_t*)forMutex;
    pthread_cond_wait(curCond, curSect);
}

void whodun::conditionSignal(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_signal(curCond);
}

void whodun::conditionBroadcast(void* forMutex, void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_broadcast(curCond);
}

void whodun::conditionKill(void* forCondition){
	pthread_cond_t* curCond = (pthread_cond_t*)forCondition;
    pthread_cond_destroy(curCond);
    free(curCond);
}

typedef struct{
	void* dllMod;
} DLLibStruct;

void* whodun::dllLoadIn(const char* loadFrom){
	void* dllMod = dlopen(loadFrom, RTLD_LAZY);
	if(dllMod){
		DLLibStruct* toRet = (DLLibStruct*)malloc(sizeof(DLLibStruct));
		toRet->dllMod = dllMod;
		return toRet;
	}
	return 0;
}

void* whodun::dllGetLocation(void* fromDLL, const char* locName){
	DLLibStruct* toRet = (DLLibStruct*)fromDLL;
	return dlsym(toRet->dllMod, locName);
}

void whodun::dllUnload(void* toKill){
	DLLibStruct* toRet = (DLLibStruct*)toKill;
	dlclose(toRet->dllMod);
	free(toRet);
}

typedef struct{
	/**The actual process.*/
	pid_t runningProc;
	/**The handle for stdin.*/
	int stdIn;
	/**The handle for stdout.*/
	int stdOut;
	/**The handle for stderr.*/
	int stdErr;
	/**Whether stdin need killing.*/
	int inNeedKill;
	/**Whether this thing has a stdout.*/
	int haveOut;
	/**Whether this thing has a stderr.*/
	int haveErr;
} LinuxSubprocess;

#define CSTREND(ofStr) (ofStr + strlen(ofStr))

void* whodun::subprocStart(SubProcessArguments* theArgs){
	//set up the arguments
	std::vector<char> argTextStore;
	std::vector<uintptr_t> argOffStore;
	argOffStore.push_back(argTextStore.size());
	argTextStore.insert(argTextStore.end(), theArgs->progName, CSTREND(theArgs->progName));
	argTextStore.push_back(0);
	for(uintptr_t i = 0; i<theArgs->numArgs; i++){
		char* curArg = theArgs->progArgs[i];
		argOffStore.push_back(argTextStore.size());
		argTextStore.insert(argTextStore.end(), curArg, CSTREND(curArg));
		argTextStore.push_back(0);
	}
	std::vector<char*> argPtrStore;
	for(uintptr_t i = 0; i<argOffStore.size(); i++){
		argPtrStore.push_back(&(argTextStore[0]) + argOffStore[i]);
	}
	argPtrStore.push_back(0);
	//pipes and things to kill
	std::vector<int> closeFail;
	std::vector<int> closeSucc;
	int stdInFDP[2];
	int stdOutFDP[2];
	int stdErrFDP[2];
	switch(theArgs->stdinMeans){
		case WHODUN_SUBPROCESS_NULL:{
			stdInFDP[0] = open("/dev/null", O_RDONLY);
			if(stdInFDP[0] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdInFDP[0]);
			}break;
		case WHODUN_SUBPROCESS_FILE:{
			stdInFDP[0] = open(theArgs->stdinFile, O_RDONLY);
			if(stdInFDP[0] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdInFDP[0]);
			}break;
		case WHODUN_SUBPROCESS_PIPE:{
			if(pipe(stdInFDP)){ goto redirect_fail_handler; }
			closeFail.push_back(stdInFDP[1]);
			closeSucc.push_back(stdInFDP[0]);
			}break;
		default:
			throw std::runtime_error("Da fuq?");
	};
	switch(theArgs->stdoutMeans){
		case WHODUN_SUBPROCESS_NULL:{
			stdOutFDP[1] = open("/dev/null", O_WRONLY);
			if(stdOutFDP[1] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdOutFDP[1]);
			}break;
		case WHODUN_SUBPROCESS_FILE:{
			stdOutFDP[1] = open(theArgs->stdoutFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
			if(stdOutFDP[1] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdOutFDP[1]);
			}break;
		case WHODUN_SUBPROCESS_PIPE:{
			if(pipe(stdOutFDP)){ goto redirect_fail_handler; }
			closeFail.push_back(stdOutFDP[0]);
			closeSucc.push_back(stdOutFDP[1]);
			}break;
		default:
			throw std::runtime_error("Da fuq?");
	};
	switch(theArgs->stderrMeans){
		case WHODUN_SUBPROCESS_NULL:{
			stdErrFDP[1] = open("/dev/null", O_WRONLY);
			if(stdErrFDP[1] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdErrFDP[1]);
			}break;
		case WHODUN_SUBPROCESS_FILE:{
			stdErrFDP[1] = open(theArgs->stderrFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
			if(stdErrFDP[1] < 0){ goto redirect_fail_handler; }
			closeSucc.push_back(stdErrFDP[1]);
			}break;
		case WHODUN_SUBPROCESS_PIPE:{
			if(pipe(stdErrFDP)){ goto redirect_fail_handler; }
			closeFail.push_back(stdErrFDP[0]);
			closeSucc.push_back(stdErrFDP[1]);
			}break;
		default:
			throw std::runtime_error("Da fuq?");
	};
	//handle errors
	goto redirect_success_handler;
	redirect_fail_handler:
	for(uintptr_t i = 0; i<closeFail.size(); i++){
		close(closeFail[i]);
	}
	for(uintptr_t i = 0; i<closeSucc.size(); i++){
		close(closeSucc[i]);
	}
	return 0;
	//fork you
	redirect_success_handler:
	pid_t madeID = fork();
	if(madeID == 0){
		//change horses
		//duplicate the redirects
		dup2(stdInFDP[0], 0);
		dup2(stdOutFDP[1], 1);
		dup2(stdErrFDP[1], 2);
		//close everything
		long maxKill = sysconf(_SC_OPEN_MAX);
		for(long i = 3; i<maxKill; i++){
			close(i);
		}
		//run the program (using the path)
		execvp(argPtrStore[0], &(argPtrStore[0]));
		//...how?
		_exit(EXIT_FAILURE);
	}
	//I'm Spartacus: clean up the parent space
	for(uintptr_t i = 0; i<closeSucc.size(); i++){
		close(closeSucc[i]);
	}
	closeSucc.clear();
	//handle failure
	if(madeID < 0){
		goto redirect_fail_handler;
	}
	//produce the report
	LinuxSubprocess* toRet = (LinuxSubprocess*)malloc(sizeof(LinuxSubprocess));
	toRet->runningProc = madeID;
	toRet->stdIn = stdInFDP[1];
	toRet->stdOut = stdOutFDP[0];
	toRet->stdErr = stdErrFDP[0];
	toRet->inNeedKill = theArgs->stdinMeans == WHODUN_SUBPROCESS_PIPE;
	toRet->haveOut = theArgs->stdoutMeans == WHODUN_SUBPROCESS_PIPE;
	toRet->haveErr = theArgs->stderrMeans == WHODUN_SUBPROCESS_PIPE;
	return toRet;
}

void whodun::subprocInFinish(void* theProc){
	LinuxSubprocess* theP = (LinuxSubprocess*)theProc;
	close(theP->stdIn);
	theP->inNeedKill = 0;
}

void whodun::subprocInWrite(void* theProc, const char* toW, uintptr_t numW){
	LinuxSubprocess* theP = (LinuxSubprocess*)theProc;
	uintptr_t totW = 0;
	while(totW < numW){
		uintptr_t curW = write(theP->stdIn, toW + totW, numW - totW);
		if(curW){
			totW += curW;
		}
		else{
			throw std::runtime_error("Problem writing to process");
		}
	}
}

uintptr_t whodun::subprocOutRead(void* theProc, char* toR, uintptr_t numR){
	LinuxSubprocess* theP = (LinuxSubprocess*)theProc;
	return read(theP->stdOut, toR, numR);
}

uintptr_t whodun::subprocErrRead(void* theProc, char* toR, uintptr_t numR){
	LinuxSubprocess* theP = (LinuxSubprocess*)theProc;
	return read(theP->stdErr, toR, numR);
}

int whodun::subprocWait(void* theProc){
	LinuxSubprocess* theP = (LinuxSubprocess*)theProc;
	if(theP->inNeedKill){ close(theP->stdIn); }
	if(theP->haveOut){ close(theP->stdOut); }
	if(theP->haveErr){ close(theP->stdErr); }
	int status;
	waitpid(theP->runningProc, &status, 0);
	free(theP);
	return status;
}



