#ifndef WHODUN_OSHOOK_H
#define WHODUN_OSHOOK_H 1

/**
 * @file
 * @brief Things that require interaction with the underlying OS.
 */

#include <string>

#include "whodun_string.h"

namespace whodun {

/**iostream is a mess.*/
class OutStream{
public:
	/**Basic setup.*/
	OutStream();
	/**Clean up and close.*/
	virtual ~OutStream();
	/**
	 * Write a byte.
	 * @param toW The byte to write.
	 */
	virtual void write(int toW) = 0;
	/**
	 * Write bytes.
	 * @param toW The bytes to write.
	 * @param numW The number of bytes to write.
	 */
	virtual void write(const char* toW, uintptr_t numW) = 0;
	/**Close the stream.*/
	virtual void close() = 0;
	/**
	 * Write bytes.
	 * @param toW The bytes to write.
	 */
	void write(SizePtrString toW);
	/**
	 * Write null-terminated bytes (without the null terminator).
	 * @param toW The bytes to write.
	 */
	void write(const char* toW);
	/**Flush waiting bytes.*/
	virtual void flush();
	/**Whether this thing has been closed.*/
	int isClosed;
};

/**iostream is a mess.*/
class InStream{
public:
	/**Basic setup.*/
	InStream();
	/**Clean up and close.*/
	virtual ~InStream();
	/**
	 * Read a byte.
	 * @return The read byte. -1 for eof.
	 */
	virtual int read() = 0;
	/**
	 * Read bytes.
	 * @param toR The buffer to store in.
	 * @param numR The maximum number of read.
	 * @return The number actually read. If less than numR, hit end.
	 */
	virtual uintptr_t read(char* toR, uintptr_t numR) = 0;
	/**Close the stream.*/
	virtual void close() = 0;
	/**
	 * Read bytes: throw exception if not enough bytes.
	 * @param toR The buffer to store in.
	 * @param numR The maximum number of read.
	 */
	void forceRead(char* toR, uintptr_t numR);
	/**
	 * Read bytes: throw exception if not enough bytes.
	 * @param toR The buffer to store in.
	 */
	void forceRead(SizePtrString toR);
	/**
	 * Read everything from the stream.
	 * @param toFill The place to put it all.
	 */
	virtual void readAll(std::vector<char>* toFill);
	/**Whether this thing has been closed.*/
	int isClosed;
};

/**A random access input stream.*/
class RandaccInStream : public InStream{
public:
	/**
	 * Get the current position in the stream.
	 * @return The current position in the stream.
	 */
	virtual uintmax_t tell() = 0;
	/**
	 * Seek to the given location.
	 * @param toLoc The location to seek to.
	 */
	virtual void seek(uintmax_t toLoc) = 0;
	/**
	 * Get the size of the thing.
	 * @return The size of the thing.
	 */
	virtual uintmax_t size() = 0;
};

/**Out to console.*/
class ConsoleOutStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleOutStream();
	/**Clean up and close.*/
	~ConsoleOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
};

/**Out to error.*/
class ConsoleErrStream: public OutStream{
public:
	/**Basic setup.*/
	ConsoleErrStream();
	/**Clean up and close.*/
	~ConsoleErrStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
};

/**In from console.*/
class ConsoleInStream: public InStream{
public:
	/**Basic setup.*/
	ConsoleInStream();
	/**Clean up and close.*/
	~ConsoleInStream();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
};

/**Out to file.*/
class FileOutStream: public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	FileOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~FileOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from file.*/
class FileInStream : public RandaccInStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	FileInStream(const char* fileName);
	/**Clean up and close.*/
	~FileInStream();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	/**The base file.*/
	FILE* baseFile;
	/**The name of the file.*/
	std::string myName;
	/**The size of the file.*/
	uintmax_t fileSize;
};

/**A task to a thread.*/
class ThreadTask{
public:
	/**Set up.*/
	ThreadTask();
	/**Clean up.*/
	virtual ~ThreadTask();
	/**Perform the task.*/
	virtual void doIt() = 0;
	/**Whether the task had an error.*/
	int wasErr;
	/**The error message from the task.*/
	std::string errMess;
};

/**Create and manage threads.*/
class OSThread{
public:
	/**
	 * Start a thread for the given task.
	 * @param toDo The task to do.
	 */
	OSThread(ThreadTask* toDo);
	/**Clean up.*/
	~OSThread();
	/**Join on the thread.*/
	void join();
	/**The handle to the thread, if not already joined.*/
	void* threadHandle;
	/**Save the task.*/
	ThreadTask* saveDo;
};

/**A managed mutex.*/
class OSMutex{
public:
	/**Set up the mutex.*/
	OSMutex();
	/**Destroy the muted.*/
	~OSMutex();
	/**Lock the mutex.*/
	void lock();
	/**Unlock the mutex.*/
	void unlock();
	/**The mutex.*/
	void* myMut;
};

/**A managed condition.*/
class OSCondition{
public:
	/**
	 * Make a condition around a mutex.
	 * @param baseMut The mutex this condition is for.
	 */
	OSCondition(OSMutex* baseMut);
	/**Destroy.*/
	~OSCondition();
	/**Wait on the condition.*/
	void wait();
	/**Signal one from the condition.*/
	void signal();
	/**Signal all from the condition.*/
	void broadcast();
	/**The mutex.*/
	void* saveMut;
	/**The condition.*/
	void* myCond;
};

/**A managed dll.*/
class OSDLLSO{
public:
	/**
	 * Load the dll.
	 * @param dllName The name of the dll to load.
	 */
	OSDLLSO(const char* dllName);
	/**Unload.*/
	~OSDLLSO();
	/**
	 * Get a location from this dll.
	 * @param locName The name of the thing to get.
	 * @return The location, or null if not found.
	 */
	void* get(const char* locName);
	/**The loaded dll.*/
	void* myDLL;
};

/**Nothin from nothin leave nothin*/
#define WHODUN_SUBPROCESS_NULL 0
/**Get/send data to a file.*/
#define WHODUN_SUBPROCESS_FILE 1
/**Will pipe between two processes.*/
#define WHODUN_SUBPROCESS_PIPE 2

/**The options to pass to a sub-process*/
typedef struct{
	/**The name of the program to run.*/
	char* progName;
	/**The number of command line arguments.*/
	uintptr_t numArgs;
	/**The command line arguments to pass.*/
	char** progArgs;
	/**How to handle stdin.*/
	int stdinMeans;
	/**How to handle stdout.*/
	int stdoutMeans;
	/**How to handle stderr.*/
	int stderrMeans;
	/**The file to use for stdin, if relevant.*/
	char* stdinFile;
	/**The file to use for stdout, if relevant.*/
	char* stdoutFile;
	/**The file to use for stderr, if relevant.*/
	char* stderrFile;
} SubProcessArguments;

/**Manage a subprocess.*/
class SubProcess{
public:
	/**
	 * Spin up a subprocess.
	 * @param theArgs The arguments to the subprocess.
	 */
	SubProcess(SubProcessArguments* theArgs);
	/**Clean up.*/
	~SubProcess();
	/**
	 * Wait on the process.
	 * @return The return code.
	 */
	int wait();
	
	/**Standard in for the process, if available.*/
	OutStream* stdIn;
	/**Standard out for the process, if available.*/
	InStream* stdOut;
	/**Standard error for the process, if available.*/
	InStream* stdErr;
	
	/**Save the process.*/
	void* saveProc;
	/**Whether it has waited.*/
	int haveWait;
	/**The return code from the process.*/
	int retCode;
};

/**The seperator for path elements.*/
extern const char* filePathSeparator;

/**
 * Gets whether a file exists.
 * @param fileName THe name of the file to test.
 * @return Whether it exists.
 */
bool fileExists(const char* fileName);

/**
 * Deletes a file, if it exists.
 * @param fileName THe name of the file to test.
 */
void fileKill(const char* fileName);

/**
 * Gets the size of a file.
 * @param fileName THe name of the file to test.
 * @return The size of said file: -1 on error.
 */
intmax_t fileGetSize(const char* fileName);

/**
 * Like ftell, but future/idiot-proofed (goddamn Windows).
 * @param stream The file in question.
 * @return The position.
 */
intmax_t fileTellFutureProof(FILE* stream);

/**
 * Like fseek, but future/idiot-proofed (goddamn Windows).
 * @param stream The file to seek.
 * @param offset The number of bytes in the file.
 * @param whence The relative location.
 * @return Whether there was a problem.
 */
int fileSeekFutureProof(FILE* stream, intmax_t offset, int whence);

/**
 * Get whether a directory exists.
 * @param dirName The name of the directory.
 * @return Whether it exists.
 */
bool directoryExists(const char* dirName);

/**
 * Make a directory.
 * @param dirName The name of the directory.
 * @return Whether there was a problem.
 */
int directoryCreate(const char* dirName);

/**
 * Delete a directory, if it exists.
 * @param dirName The name of the directory to delete.
 */
void directoryKill(const char* dirName);

/**
 * Open a directory.
 * @param dirName The name of the directory.
 * @return A handle to the directory.
 */
void* directoryOpen(const char* dirName);

/**
 * Read the next entry from a directory.
 * @param dirHand The handle to the directory.
 * @return Whether there is such an entry.
 */
int directoryReadNext(void* dirHand);

/**
 * Get whether the current item is a directory.
 * @param dirHand The handle to the directory.
 * @return Whether it is a directory.
 */
int directoryCurIsDir(void* dirHand);

/**
 * Get the name of the current item.
 * @param dirHand The handle to the directory.
 * @return The name.
 */
const char* directoryCurName(void* dirHand);

/**
 * Close a directory.
 * @param dirHand The directory to the handle.
 */
void directoryClose(void* dirHand);

/**
 * This will start a thread.
 * @param callFun The thread function.
 * @param callUniform The thing to pass to said function.
 * @return A handle to the thread.
 */
void* threadStart(void(*callFun)(void*), void* callUniform);

/**
 * Joins on a thread and drops it.
 * @param tHandle THe thread to join on.
 */
void threadJoin(void* tHandle);

/**
 * Make a mutex for future use.
 * @return The created mutex.
 */
void* mutexMake();

/**
 * Get a lock.
 * @param toLock The lock to get.
 */
void mutexLock(void* toLock);

/**
 * Release a lock.
 * @param toUnlock The lock to get.
 */
void mutexUnlock(void* toUnlock);

/**
 * Delete a mutex.
 * @param toKill The lock to delete.
 */
void mutexKill(void* toKill);

/**
 * Make a condition variable.
 * @param forMutex The lock to make it for.
 * @return The cretated condition variable.
 */
void* conditionMake(void* forMutex);

/**
 * Wait on a condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionWait(void* forMutex, void* forCondition);

/**
 * Start one waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionSignal(void* forMutex, void* forCondition);

/**
 * Start all waiting on condition.
 * @param forMutex The relevant lock.
 * @param forCondition The condition in question.
 */
void conditionBroadcast(void* forMutex, void* forCondition);

/**
 * Delete a condition variable.
 * @param forCondition The condition to kill.
 */
void conditionKill(void* forCondition);

/**
 * Load a dll.
 * @param loadFrom The name of the dll.
 * @return A handle to it.
 */
void* dllLoadIn(const char* loadFrom);

/**
 * Get the location of a loaded item in a dll.
 * @param fromDLL The dll to get from.
 * @param locName The name.
 * @return The handle to the dll.
 */
void* dllGetLocation(void* fromDLL, const char* locName);

/**
 * Unload a dll.
 * @param toKill The dll to load.
 */
void dllUnload(void* toKill);

/**
 * Spin up a subprocess.
 * @param theArgs The information for the subprocess.
 * @return The subprocess handle.
 */
void* subprocStart(SubProcessArguments* theArgs);

/**
 * Note that nothing else is coming over stdin.
 * @param theProc The process in question.
 */
void subprocInFinish(void* theProc);

/**
 * Write to the standard input of a process.
 * @param theProc The process.
 * @param toW The bytes to write.
 * @param numW The number of bytes to write.
 */
void subprocInWrite(void* theProc, const char* toW, uintptr_t numW);

/**
 * Read from the standard output of a process.
 * @param theProc The process.
 * @param toR The place to store the bytes.
 * @param numR The number of bytes to read.
 * @return The number of bytes actually read.
 */
uintptr_t subprocOutRead(void* theProc, char* toR, uintptr_t numR);

/**
 * Read from the standard output of a process.
 * @param theProc The process.
 * @param toR The place to store the bytes.
 * @param numR The number of bytes to read.
 * @return The number of bytes actually read.
 */
uintptr_t subprocErrRead(void* theProc, char* toR, uintptr_t numR);

/**
 * Wait for the process to end.
 * @param theProc The process to wait on.
 * @return The return value of the process.
 */
int subprocWait(void* theProc);


};

#endif

