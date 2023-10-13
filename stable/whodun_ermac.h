#ifndef WHODUN_ERMAC_H
#define WHODUN_ERMAC_H 1

/**
 * @file
 * @brief Error reporting.
 */

#include <stdint.h>
#include <exception>

#include "whodun_oshook.h"

namespace whodun {

/**Error level for bad assertions (the code is wrong).*/
#define WHODUN_ERROR_LEVEL_FATAL 0
/**Error level for real problems (the type that should probably kill the program: resource limits and the like).*/
#define WHODUN_ERROR_LEVEL_ERROR 10
/**Error level for potentially recoverable problems (io errors, human stupidity).*/
#define WHODUN_ERROR_LEVEL_WARNING 20
/**Error level for weird but valid requests.*/
#define WHODUN_ERROR_LEVEL_INFO 30
/**Error level for debug information.*/
#define WHODUN_ERROR_LEVEL_ACCOUNT 40
/**Error level for debug information.*/
#define WHODUN_ERROR_LEVEL_DEBUG 50
/**An error that notes where the next error comes from.*/
#define WHODUN_ERROR_LEVEL_SOURCE 60

/**The number of characters to use for error descriptions.*/
#define WHODUN_ERROR_ARRSIZE 1024
/**The maximum amount of extra information.*/
#define WHODUN_ERROR_MAXEXTRA 10
/**The maximum number of frames to save.*/
#define WHODUN_ERROR_MAXSTACK 5

/**An unknown error.*/
#define WHODUN_ERROR_SDESC_UNKNOWNEXCEPT "UNCEXC"
/**A bad command line argument.*/
#define WHODUN_ERROR_SDESC_BADCLIARG "BadCLI"
/**A dangling link.*/
#define WHODUN_ERROR_SDESC_DANGNAME "Dangle"
/**The operating system complained.*/
#define WHODUN_ERROR_SDESC_OSCOMP "OS"
/**IO issues.*/
#define WHODUN_ERROR_SDESC_IO "IO"
/**Bad file data.*/
#define WHODUN_ERROR_SDESC_FILEMANG "FileData"
/**Assertion failed.*/
#define WHODUN_ERROR_SDESC_ASSERT "Assert"
/**Funky math.*/
#define WHODUN_ERROR_SDESC_WEIRDMATH "WeirdMath"
/**Current progress.*/
#define WHODUN_ERROR_SDESC_UPDATE "Update"

/**A description of an error.*/
typedef struct{
	/**The level.*/
	int level;
	/**The offset of the simple description.*/
	int simpleDescOff;
	/**The code file.*/
	const char* codeSourceFile;
	/**The code line*/
	int codeLine;
	/**The offset of the description.*/
	int descriptionOff;
	/**The number of extra information.*/
	int numExtra;
	/**The offsets of the extra information.*/
	int extraOff[WHODUN_ERROR_MAXEXTRA];
	/**Character storage.*/
	char strBuffer[WHODUN_ERROR_ARRSIZE];
} ErrorDescription;

/**Specialized exception class*/
class WhodunError : public std::exception {
public:
	/**
	 * Create an uninitialized error.
	 */
	WhodunError();
	/**
	 * Create an error.
	 * @param level The error level: should only be warning or worse.
	 * @param simpleDescription A simple (one word) description of the error.
	 * @param sourceFile The code file the error occured at. Should be a safe pointer.
	 * @param sourceLine The line in that file.
	 * @param description A full description of the error.
	 * @param numExtra The number of extra pieces of information.
	 * @param theExtra The extra information.
	 */
	WhodunError(int level, const char* simpleDescription, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** theExtra);
	/**
	 * Create an error from a description.
	 * @param toCopy The description to copy.
	 */
	WhodunError(const ErrorDescription* toCopy);
	/**
	 * Copy constructor.
	 * @param toCopy The thing to copy.
	 */
	WhodunError(const WhodunError& toCopy);
	/**
	 * Note a new location for an error.
	 * @param toCopy The error to wrap.
	 * @param sourceFile The new code file.
	 * @param sourceLine The new line.
	 */
	WhodunError(const WhodunError& toCopy, const char* sourceFile, int sourceLine);
	/**
	 * Note a new location for an error.
	 * @param toCopy The error to wrap.
	 * @param sourceFile The new code file.
	 * @param sourceLine The new line.
	 */
	WhodunError(const std::exception& toCopy, const char* sourceFile, int sourceLine);
	/**Tear down.*/
	~WhodunError();
	/**
	 * Copy from another error.
	 * @param toCopy THe error to copy.
	 * @return This.
	 */
	WhodunError& operator=(const WhodunError& toCopy);
	/**
	 * Report a simple explanation of this error.
	 * @return The explanation.
	 */
	const char* what() const throw();
	
	/**The size of the stack.*/
	int numStack;
	/**Place to save error information.*/
	ErrorDescription saveErrors[WHODUN_ERROR_MAXSTACK];
};

/**Run some code and ignore any exceptions.*/
#define WHODUN_EXCEPT_IGNORE(onCode) try{ onCode }catch( const std::exception& errE ){}
/**Run some code: if an error occurs, add to the trace.*/
#define WHODUN_EXCEPT_TRACE(onCode) try{ onCode }catch(const whodun::WhodunError& errE){ throw WhodunError(errE, __FILE__, __LINE__); }catch( const std::exception& errE){ throw WhodunError(errE, __FILE__, __LINE__); }

/**A place to log errors.*/
class ErrorLog{
public:
	/**Set up default*/
	ErrorLog();
	/**Allow subclass.*/
	virtual ~ErrorLog();
	/**
	 * Note an error.
	 * @param level The severity of the error.
	 * @param simpleDesc A one-word/abbreviation description of the error.
	 * @param sourceFile The code file the error came from.
	 * @param sourceLine The line in that file.
	 * @param description A full description of the error.
	 * @param numExtra The number of extra items.
	 * @param extras Extra pieces of information.
	 */
	void logError(int level, const char* simpleDesc, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** extras);
	/**
	 * Note an error.
	 * @param toLog The error(s) to log.
	 */
	void logError(const WhodunError& toLog);
	/**
	 * Note an error.
	 * @param toLog The error to log.
	 */
	void logError(const ErrorDescription* toLog);
	/**
	 * Write an error.
	 * @param toLog The error to log.
	 */
	void spitError(const ErrorDescription* toLog);
	/**
	 * Write a piece of error to the final target.
	 * @param level The severity of the error.
	 * @param simpleDesc A one-word/abbreviation description of the error.
	 * @param sourceFile The code file the error came from.
	 * @param sourceLine The line in that file.
	 * @param description A full description of the error.
	 * @param numExtra The number of extra items.
	 * @param extras Extra pieces of information.
	 */
	virtual void spitError(int level, const char* simpleDesc, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** extras) = 0;
	/**Lock*/
	OSMutex useMut;
};

/**Log errors to a stream.*/
class StreamErrorLog : public ErrorLog{
public:
	/**
	 * Wrap the target stream.
	 * @param targetStream The place to write errors.
	 */
	StreamErrorLog(OutStream* targetStream);
	/**Clean up.*/
	~StreamErrorLog();
	void spitError(int level, const char* simpleDesc, const char* sourceFile, int sourceLine, const char* description, int numExtra, const char** extras);
	/**The place to write errors.*/
	OutStream* tgtStr;
};

}

#endif

