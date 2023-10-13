#include "whodun_oshook.h"

#include <stdio.h>
#include <string.h>

namespace whodun {

/**
 * The actual function for running a thread.
 * @param threadUni The OSThread object to run.
 */
void actualThreadFunc(void* threadUni);

/**Write to stdin of a process.*/
class SubprocessStdinStream : public OutStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStdinStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStdinStream();
	
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};

/**Read from stdout of a process.*/
class SubprocessStdoutStream : public InStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStdoutStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStdoutStream();
	
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};

/**Read from stdout of a process.*/
class SubprocessStderrStream : public InStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStderrStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStderrStream();
	
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};

};

using namespace whodun;

OutStream::OutStream(){
	isClosed = 0;
}
OutStream::~OutStream(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}
void OutStream::write(SizePtrString toW){
	write(toW.txt, toW.len);
}
void OutStream::write(const char* toW){
	write(toW, strlen(toW));
}
void OutStream::flush(){}

#define READ_ALL_BUFFER_SIZE 1024

InStream::InStream(){
	isClosed = 0;
}
InStream::~InStream(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}
void InStream::forceRead(char* toR, uintptr_t numR){
	uintptr_t numRead = read(toR, numR);
	if(numRead != numR){
		throw std::runtime_error("Truncated stream.");
	}
}
void InStream::forceRead(SizePtrString toR){
	forceRead(toR.txt, toR.len);
}
void InStream::readAll(std::vector<char>* toFill){
	char inpBuff[READ_ALL_BUFFER_SIZE];
	uintptr_t numRead = read(inpBuff, READ_ALL_BUFFER_SIZE);
	while(numRead){
		toFill->insert(toFill->end(), inpBuff, inpBuff + numRead);
		numRead = read(inpBuff, READ_ALL_BUFFER_SIZE);
	}
}

ConsoleOutStream::ConsoleOutStream(){}
ConsoleOutStream::~ConsoleOutStream(){}
void ConsoleOutStream::write(int toW){
	fputc(toW, stdout);
}
void ConsoleOutStream::write(const char* toW, uintptr_t numW){
	fwrite(toW, 1, numW, stdout);
}
void ConsoleOutStream::close(){isClosed = 1;};

ConsoleErrStream::ConsoleErrStream(){}
ConsoleErrStream::~ConsoleErrStream(){}
void ConsoleErrStream::write(int toW){
	fputc(toW, stderr);
}
void ConsoleErrStream::write(const char* toW, uintptr_t numW){
	fwrite(toW, 1, numW, stderr);
}
void ConsoleErrStream::close(){isClosed = 1;};

ConsoleInStream::ConsoleInStream(){}
ConsoleInStream::~ConsoleInStream(){}
int ConsoleInStream::read(){
	return fgetc(stdin);
}
uintptr_t ConsoleInStream::read(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, stdin);
}
void ConsoleInStream::close(){isClosed = 1;};

FileOutStream::FileOutStream(int append, const char* fileName){
	if(append){
		baseFile = fopen(fileName, "ab");
	}
	else{
		baseFile = fopen(fileName, "wb");
	}
	if(baseFile == 0){
		isClosed = 1;
		throw std::runtime_error("Could not open file " + myName);
	}
	myName = fileName;
}
FileOutStream::~FileOutStream(){}
void FileOutStream::write(int toW){
	if(fputc(toW, baseFile) < 0){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void FileOutStream::write(const char* toW, uintptr_t numW){
	if(fwrite(toW, 1, numW, baseFile) != numW){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void FileOutStream::close(){
	isClosed = 1;
	if(baseFile){
		int probClose = fclose(baseFile);
		baseFile = 0;
		if(probClose){ throw std::runtime_error("Problem closing file."); }
	}
}

FileInStream::FileInStream(const char* fileName){
	myName = fileName;
	intmax_t testSize = fileGetSize(fileName);
	if(testSize < 0){ isClosed = 1; throw std::runtime_error("Could not open file " + myName); }
	fileSize = testSize;
	baseFile = fopen(fileName, "rb");
	if(baseFile == 0){ isClosed = 1; throw std::runtime_error("Could not open file " + myName); }
}
FileInStream::~FileInStream(){}
uintmax_t FileInStream::tell(){
	intmax_t curPos = fileTellFutureProof(baseFile);
	if(curPos < 0){ throw std::runtime_error("Problem getting position of file " + myName); }
	return curPos;
}
void FileInStream::seek(uintmax_t toLoc){
	int wasP = fileSeekFutureProof(baseFile, toLoc, SEEK_SET);
	if(wasP){ throw std::runtime_error("Problem seeking in file " + myName); }
}
uintmax_t FileInStream::size(){
	return fileSize;
}
int FileInStream::read(){
	int toR = fgetc(baseFile);
	if((toR < 0) && ferror(baseFile)){
		throw std::runtime_error("Problem reading file " + myName);
	}
	return toR;
}
uintptr_t FileInStream::read(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, baseFile);
}
void FileInStream::close(){
	isClosed = 1;
	if(baseFile){
		int probClose = fclose(baseFile);
		baseFile = 0;
		if(probClose){ throw std::runtime_error("Problem closing file."); }
	}
}

ThreadTask::ThreadTask(){
	wasErr = 0;
}
ThreadTask::~ThreadTask(){}

void whodun::actualThreadFunc(void* threadUni){
	OSThread* mainUni = (OSThread*)threadUni;
	ThreadTask* baseUni = mainUni->saveDo;
	try{
		baseUni->doIt();
	}catch(std::exception& err){
		baseUni->wasErr = 1;
		baseUni->errMess = err.what();
	}
}

OSThread::OSThread(ThreadTask* toDo){
	saveDo = toDo;
	threadHandle = threadStart(actualThreadFunc, this);
	if(!threadHandle){
		throw std::runtime_error("Problem starting thread.");
	}
}
OSThread::~OSThread(){
	if(threadHandle){ std::cerr << "You MUST join on a thread before its destruction." << std::endl; std::terminate(); }
}
void OSThread::join(){
	threadJoin(threadHandle);
	threadHandle = 0;
}

OSMutex::OSMutex(){
	myMut = mutexMake();
}
OSMutex::~OSMutex(){
	mutexKill(myMut);
}
void OSMutex::lock(){
	mutexLock(myMut);
}
void OSMutex::unlock(){
	mutexUnlock(myMut);
}

OSCondition::OSCondition(OSMutex* baseMut){
	saveMut = baseMut->myMut;
	myCond = conditionMake(saveMut);
}
OSCondition::~OSCondition(){
	conditionKill(myCond);
}
void OSCondition::wait(){
	conditionWait(saveMut, myCond);
}
void OSCondition::signal(){
	conditionSignal(saveMut, myCond);
}
void OSCondition::broadcast(){
	conditionBroadcast(saveMut, myCond);
}

OSDLLSO::OSDLLSO(const char* dllName){
	myDLL = dllLoadIn(dllName);
	if(myDLL == 0){
		throw std::runtime_error("Could not load dll.");
	}
}
OSDLLSO::~OSDLLSO(){
	dllUnload(myDLL);
}
void* OSDLLSO::get(const char* locName){
	return dllGetLocation(myDLL, locName);
}

SubprocessStdinStream::SubprocessStdinStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStdinStream::~SubprocessStdinStream(){}
void SubprocessStdinStream::write(int toW){
	char realW = toW;
	write(&realW, 1);
}
void SubprocessStdinStream::write(const char* toW, uintptr_t numW){
	subprocInWrite(baseProc->saveProc, toW, numW);
}
void SubprocessStdinStream::close(){
	subprocInFinish(baseProc->saveProc);
	isClosed = 1;
}

SubprocessStdoutStream::SubprocessStdoutStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStdoutStream::~SubprocessStdoutStream(){}
int SubprocessStdoutStream::read(){
	char realR;
	if(read(&realR, 1)){
		return 0x00FF & realR;
	}
	else{
		return -1;
	}
}
uintptr_t SubprocessStdoutStream::read(char* toR, uintptr_t numR){
	return subprocOutRead(baseProc->saveProc, toR, numR);
}
void SubprocessStdoutStream::close(){
	isClosed = 1;
}

SubprocessStderrStream::SubprocessStderrStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStderrStream::~SubprocessStderrStream(){}
int SubprocessStderrStream::read(){
	char realR;
	if(read(&realR, 1)){
		return 0x00FF & realR;
	}
	else{
		return -1;
	}
}
uintptr_t SubprocessStderrStream::read(char* toR, uintptr_t numR){
	return subprocErrRead(baseProc->saveProc, toR, numR);
}
void SubprocessStderrStream::close(){
	isClosed = 1;
}

SubProcess::SubProcess(SubProcessArguments* theArgs){
	saveProc = subprocStart(theArgs);
	if(saveProc == 0){
		throw std::runtime_error("Problem starting process.");
	}
	haveWait = 0;
	this->stdIn = 0;
	this->stdOut = 0;
	this->stdErr = 0;
	if(theArgs->stdinMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdIn = new SubprocessStdinStream(this);
	}
	if(theArgs->stdoutMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdOut = new SubprocessStdoutStream(this);
	}
	if(theArgs->stderrMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdErr = new SubprocessStderrStream(this);
	}
}
SubProcess::~SubProcess(){
	if(saveProc == 0){
		std::cerr << "You MUST wait on a process before terminating.";
		std::terminate();
	}
}
int SubProcess::wait(){
	if(this->stdIn){ delete(this->stdIn); this->stdIn = 0; }
	if(this->stdOut){ delete(this->stdOut); this->stdOut = 0; }
	if(this->stdErr){ delete(this->stdErr); this->stdErr = 0; }
	retCode = subprocWait(saveProc);
	haveWait = 1;
	return retCode;
}



