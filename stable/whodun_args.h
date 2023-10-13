#ifndef WHODUN_ARGS_H
#define WHODUN_ARGS_H 1

/**
 * @file
 * @brief Command line argument parsing.
 */

#include <map>
#include <string>

#include "whodun_ermac.h"
#include "whodun_oshook.h"

namespace whodun {

class StandardProgram;

/**A command line argument.*/
class ArgumentOption{
public:
	/**Default set up*/
	ArgumentOption();
	/**Allow subclasses*/
	virtual ~ArgumentOption();
	
	/**Whether this option should be advertised.*/
	int isCommon;
	/**The name of this option.*/
	std::string name;
	/**A summary of this option.*/
	std::string summary;
	/**A full description of this option. Empty if the summary will suffice.*/
	std::string description;
	/**How to use this option.*/
	std::string usage;
	/**A type code to describe the basic type of this argument.*/
	const char* typeCode;
	/**An extended type code.*/
	const char* extTypeCode;
	
	/**
	 * Test whether this can parse the lead argument.
	 * @param argc The number of arguments remaining.
	 * @param argv The arguments.
	 * @return Whether it can parse.
	 */
	virtual int canParse(uintptr_t argc, char** argv) = 0;
	
	/**
	 * Parse the lead argument.
	 * @param argc The number of arguments remaining.
	 * @param argv The arguments.
	 * @param forProg The program to parse for.
	 * @return The number of ate arguments.
	 */
	virtual uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg) = 0;
	
	/**
	 * Any additional checks on this argument.
	 */
	virtual void idiotCheck();
	
	/**
	 * Output gui info in a standard format.
	 * @param toStr The place to write it.
	 */
	virtual void dumpInfo(OutStream* toStr);
	
	/**
	 * Dump common gui info.
	 * @param toStr The place to write it.
	 * @param numExtra The number of extra bytes.
	 */
	void dumpCommonInfo(OutStream* toStr, uintptr_t numExtra);
};

/**********************************************************************/
/*Helpful arguments*/

/**Help*/
class ArgumentOptionHelp : public ArgumentOption{
public:
	/**Set up*/
	ArgumentOptionHelp();
	/**Tear down.*/
	~ArgumentOptionHelp();
	int canParse(uintptr_t argc, char** argv);
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
};

/**Version*/
class ArgumentOptionVersion : public ArgumentOption{
public:
	/**Set up*/
	ArgumentOptionVersion();
	/**Tear down.*/
	~ArgumentOptionVersion();
	int canParse(uintptr_t argc, char** argv);
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
};

/**Dump for gui*/
class ArgumentOptionHelpArgdump : public ArgumentOption{
public:
	/**Set up*/
	ArgumentOptionHelpArgdump();
	/**Tear down.*/
	~ArgumentOptionHelpArgdump();
	int canParse(uintptr_t argc, char** argv);
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
};

/**Idiot check only*/
class ArgumentOptionHelpArgcheck : public ArgumentOption{
public:
	/**Set up*/
	ArgumentOptionHelpArgcheck();
	/**Tear down.*/
	~ArgumentOptionHelpArgcheck();
	int canParse(uintptr_t argc, char** argv);
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
};

/**********************************************************************/
/*Common arguments*/

/**A simple named argument.*/
class ArgumentOptionNamed : public ArgumentOption{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionNamed(const char* theName);
	/**Tear down.*/
	~ArgumentOptionNamed();
	int canParse(uintptr_t argc, char** argv);
};

/**A flag argument.*/
class ArgumentOptionFlag : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFlag(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFlag();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	/**The value of the flag.*/
	bool value;
};

/**Storage for the value jacked with by the enums.*/
class ArgumentOptionEnumValue{
public:
	/**
	 * Set up the storage.
	 * @param className The name of this enum flavor.
	 */
	ArgumentOptionEnumValue(const char* className);
	/**Clean up.*/
	~ArgumentOptionEnumValue();
	
	/**The name of this class.*/
	std::string classFlavor;
	/**The value of the enum.*/
	uintptr_t value;
	/**The number of options registered for this enum.*/
	uintptr_t numRegistered;
};

/**An enum argument.*/
class ArgumentOptionEnum : public ArgumentOptionNamed{
public:
	/**
	 * Set up.
	 * @param theName The name of this argument.
	 * @param forClass The flavor of enum this is for.
	 */
	ArgumentOptionEnum(const char* theName, ArgumentOptionEnumValue* forClass);
	/**Clean up.*/
	~ArgumentOptionEnum();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	/**
	 * Get whether this is the selected enum.
	 * @return WHether this is the selected option.
	 */
	bool value();
	void dumpInfo(OutStream* toStr);
	/**The enum this is for.*/
	ArgumentOptionEnumValue* myClass;
	/**The value this sets to on call.*/
	uintptr_t setValue;
};

/**An integer argument.*/
class ArgumentOptionInteger : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionInteger(const char* theName);
	/**Tear down.*/
	~ArgumentOptionInteger();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	void dumpInfo(OutStream* toStr);
	/**The value of the flag.*/
	intptr_t value;
};

/**An integer argument.*/
class ArgumentOptionIntegerVector : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionIntegerVector(const char* theName);
	/**Tear down.*/
	~ArgumentOptionIntegerVector();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	/**The value of the flag.*/
	std::vector<intptr_t> value;
};

/**A float argument.*/
class ArgumentOptionFloat : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFloat(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFloat();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	void dumpInfo(OutStream* toStr);
	/**The value of the flag.*/
	double value;
};

/**A float argument.*/
class ArgumentOptionFloatVector : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFloatVector(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFloatVector();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	/**The value of the flag.*/
	std::vector<double> value;
};

/**A string argument.*/
class ArgumentOptionString : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionString(const char* theName);
	/**Tear down.*/
	~ArgumentOptionString();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	void dumpInfo(OutStream* toStr);
	/**The value of the flag.*/
	std::string value;
};

/**A string argument.*/
class ArgumentOptionStringVector : public ArgumentOptionNamed{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionStringVector(const char* theName);
	/**Tear down.*/
	~ArgumentOptionStringVector();
	uintptr_t parse(uintptr_t argc, char** argv, StandardProgram* forProg);
	/**The value of the flag.*/
	std::vector<std::string> value;
};

/**********************************************************************/
/*Specialized arguments*/

/**How many threads to use.*/
class ArgumentOptionThreadcount : public ArgumentOptionInteger{
public:
	/** Set up */
	ArgumentOptionThreadcount();
	/**Tear down.*/
	~ArgumentOptionThreadcount();
	void idiotCheck();
};

/**How many things to do per thread.*/
class ArgumentOptionThreadgrain : public ArgumentOptionInteger{
public:
	/** Set up */
	ArgumentOptionThreadgrain();
	/**Tear down.*/
	~ArgumentOptionThreadgrain();
	void idiotCheck();
};

/**A file to open for reading.*/
class ArgumentOptionFileRead : public ArgumentOptionString{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFileRead(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFileRead();
	void idiotCheck();
	void dumpInfo(OutStream* toStr);
	/**The extensions this is looking for.*/
	std::vector<const char*> validExts;
	/**Whether this argument is required.*/
	int required;
};

/**A file to open for writing.*/
class ArgumentOptionFileWrite : public ArgumentOptionString{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFileWrite(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFileWrite();
	void idiotCheck();
	void dumpInfo(OutStream* toStr);
	/**The extensions this is looking for.*/
	std::vector<const char*> validExts;
	/**Whether this argument is required.*/
	int required;
};

/**Files to open for reading.*/
class ArgumentOptionFileReadVector : public ArgumentOptionStringVector{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFileReadVector(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFileReadVector();
	void dumpInfo(OutStream* toStr);
	/**The extensions this is looking for.*/
	std::vector<const char*> validExts;
};

/**Files to open for writing.*/
class ArgumentOptionFileWriteVector : public ArgumentOptionStringVector{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFileWriteVector(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFileWriteVector();
	void dumpInfo(OutStream* toStr);
	/**The extensions this is looking for.*/
	std::vector<const char*> validExts;
};

/**A folder to open for reading.*/
class ArgumentOptionFolderRead : public ArgumentOptionString{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFolderRead(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFolderRead();
	void idiotCheck();
	/**Whether this argument is required.*/
	int required;
};

/**A folder to open for writing.*/
class ArgumentOptionFolderWrite : public ArgumentOptionString{
public:
	/**
	 * Set up
	 * @param theName The name of the argument.
	 */
	ArgumentOptionFolderWrite(const char* theName);
	/**Tear down.*/
	~ArgumentOptionFolderWrite();
	void idiotCheck();
	/**Whether this argument is required.*/
	int required;
};

/**********************************************************************/
/*Programs and parsing*/

/**A standard program.*/
class StandardProgram{
public:
	/**Default set up*/
	StandardProgram();
	/**Allow subclasses*/
	virtual ~StandardProgram();
	
	/**The name of this program.*/
	std::string name;
	/**A summary of this option.*/
	std::string summary;
	/**A full description of this option. Empty if the summary will suffice.*/
	std::string description;
	/**How to use this option.*/
	std::string usage;
	
	/**Version info for this program.*/
	std::string version;
	
	/**All the possible options.*/
	std::vector<ArgumentOption*> allOptions;
	/**The input to use.*/
	InStream* useIn;
	/**The output to use.*/
	OutStream* useOut;
	/**The place to log errors*/
	ErrorLog* useErr;
	/**Whether this still needs to run.*/
	int needRun;
	/**Whether idiot checks need to run.*/
	int needIdiot;
	/**Whether in/out/err need killing (1/2/4).*/
	int needKill;
	/**Whether this thing hit an error.*/
	int wasError;
	
	/**
	 * Parse a set of command line arguments.
	 * @param argc The number of arguments.
	 * @param argv The arguments.
	 */
	void parseArguments(int argc, char** argv);
	
	/**
	 * Any additional checks on the arguments.
	 * @return
	 */
	virtual void idiotCheck();
	
	/**Let the arguments do an initial check of themselves.*/
	void idiotCheckArguments();
	
	/**
	 * Run the program.
	 */
	void run();
	
	/**
	 * The part that actually does what the program is supposed to do.
	 * @return
	 */
	virtual void baseRun() = 0;
	
	/**
	 * Write out some help info.
	 * @param dumpTgt The place to write the help data to.
	 */
	void printHelp(OutStream* dumpTgt);
	
	/**
	 * Write out some version info.
	 * @param dumpTgt The place to write the version data to.
	 */
	void printVersion(OutStream* dumpTgt);
	
	/**
	 * Dump argument data.
	 * @param dumpTgt The place to write the argument data to.
	 */
	void dumpArguments(OutStream* dumpTgt);
	
	/**The help option.*/
	ArgumentOptionHelp optHelp;
	/**The version option.*/
	ArgumentOptionVersion optVersion;
	/**The argdump option.*/
	ArgumentOptionHelpArgdump optHelpArgdump;
	/**The idiot check option.*/
	ArgumentOptionHelpArgcheck optHelpIdiot;
};

/**A set of programs.*/
class StandardProgramSet{
public:
	/**Default set up*/
	StandardProgramSet();
	/**Allow subclasses*/
	virtual ~StandardProgramSet();
	
	/**The name of this set.*/
	std::string name;
	/**A summary of this set.*/
	std::string summary;
	
	/**Version info for this program.*/
	std::string version;
	
	/**
	 * Parse command line arguments to figure out what to run.
	 * @param argc The number of arguments.
	 * @param argv The arguments.
	 * @param useIn The place to get input from.
	 * @param useOut The place to send output.
	 * @param useErr The place to send errors.
	 * @return The program, ready to run with parsed arguments, or null if none should run.
	 */
	StandardProgram* parseArguments(int argc, char** argv, InStream* useIn, OutStream* useOut, ErrorLog* useErr);
	
	/**
	 * Write out some help info.
	 * @param dumpTgt The place to write the help data to.
	 */
	void printHelp(OutStream* dumpTgt);
	
	/**
	 * Write out some version info.
	 * @param dumpTgt The place to write the version data to.
	 */
	void printVersion(OutStream* dumpTgt);
	
	/**
	 * Dump the programs this can run.
	 * @param dumpTgt The place to write the argument data to.
	 */
	void dumpPrograms(OutStream* dumpTgt);
	
	/**The programs this can run.*/
	std::map<std::string,StandardProgram*(*)()> hotPrograms;
};

/**
 * Helpful factory function.
 * @return The produced program.
 */
template<typename PT>
StandardProgram* makeNewProgram(){
	return new PT();
}

};

#endif

