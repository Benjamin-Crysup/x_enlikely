#include "whodun_args.h"

#include <string.h>

using namespace whodun;

ArgumentOption::ArgumentOption(){
	isCommon = 1;
}
ArgumentOption::~ArgumentOption(){}
void ArgumentOption::idiotCheck(){}
void ArgumentOption::dumpInfo(OutStream* toStr){
	dumpCommonInfo(toStr, 0);
}
void ArgumentOption::dumpCommonInfo(OutStream* toStr, uintptr_t numExtra){
	char intDump[8];
	BytePacker packI(intDump);
	//name
		packI.retarget(intDump); packI.packBE64(name.size()); toStr->write(intDump, 8);
		toStr->write(name.c_str(), name.size());
	//summary
		packI.retarget(intDump); packI.packBE64(summary.size()); toStr->write(intDump, 8);
		toStr->write(summary.c_str(), summary.size());
	//description
		packI.retarget(intDump); packI.packBE64(description.size()); toStr->write(intDump, 8);
		toStr->write(description.c_str(), description.size());
	//usage
		packI.retarget(intDump); packI.packBE64(usage.size()); toStr->write(intDump, 8);
		toStr->write(usage.c_str(), usage.size());
	//whether it is common
		toStr->write(isCommon ? 1 : 0);
	//type codes
		SizePtrString typeCodeS = toSizePtr(typeCode);
			packI.retarget(intDump); packI.packBE64(typeCodeS.len); toStr->write(intDump, 8);
			toStr->write(typeCodeS.txt, typeCodeS.len);
		SizePtrString extTypeCodeS = toSizePtr(extTypeCode);
			packI.retarget(intDump); packI.packBE64(extTypeCodeS.len); toStr->write(intDump, 8);
			toStr->write(extTypeCodeS.txt, extTypeCodeS.len);
	//amount of extra crap
		packI.retarget(intDump); packI.packBE64(numExtra); toStr->write(intDump, 8);
}

StandardProgram::StandardProgram(){
	needRun = 1;
	needIdiot = 1;
	useIn = 0;
	useOut = 0;
	useErr = 0;
	needKill = 0;
	wasError = 0;
	allOptions.push_back(&optHelp);
	allOptions.push_back(&optVersion);
	allOptions.push_back(&optHelpArgdump);
	allOptions.push_back(&optHelpIdiot);
}
StandardProgram::~StandardProgram(){
	if(needKill & 1){ useIn->close(); delete(useIn); }
	if(needKill & 2){ useOut->close(); delete(useOut); }
	if(needKill & 4){
		OutStream* tmpErr = ((StreamErrorLog*)useErr)->tgtStr;
		delete(useErr);
		tmpErr->close();
		delete(tmpErr);
	}
}
void StandardProgram::parseArguments(int argc, char** argv){
	if(useIn == 0){
		useIn = new ConsoleInStream();
		needKill = needKill | 1;
	}
	if(useOut == 0){
		useOut = new ConsoleOutStream();
		needKill = needKill | 2;
	}
	if(useErr == 0){
		OutStream* tmpErr = new ConsoleErrStream();
		useErr = new StreamErrorLog(tmpErr);
		needKill = needKill | 4;
	}
	try{
		int numArg = argc;
		char** theArgs = argv;
		while(numArg){
			int handled = 0;
			for(uintptr_t i = 0; i<allOptions.size(); i++){
				ArgumentOption* curOpt = allOptions[i];
				if(curOpt->canParse(numArg, theArgs)){
					uintptr_t numEat = curOpt->parse(numArg, theArgs, this);
					numArg -= numEat;
					theArgs += numEat;
					handled = 1;
					break;
				}
			}
			if(!handled){
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown command line argument.", 1, (const char**)theArgs);
			}
		}
		if(needIdiot){
			idiotCheckArguments();
			idiotCheck();
		}
	}
	catch(WhodunError& errW){
		wasError = 1;
		needRun = 0;
		useErr->logError(errW);
	}
	catch(std::exception& errE){
		wasError = 1;
		needRun = 0;
		useErr->logError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_UNKNOWNEXCEPT, __FILE__, __LINE__, errE.what(), 0, 0);
	}
}
void StandardProgram::idiotCheck(){
	//the default implementation does nothing
}
void StandardProgram::idiotCheckArguments(){
	for(uintptr_t i = 0; i<allOptions.size(); i++){
		allOptions[i]->idiotCheck();
	}
}
void StandardProgram::run(){
	try{
		if(needRun){
			baseRun();
		}
	}
	catch(WhodunError& errW){
		wasError = 1;
		useErr->logError(errW);
	}
	catch(std::exception& errE){
		wasError = 1;
		useErr->logError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_UNKNOWNEXCEPT, __FILE__, __LINE__, errE.what(), 0, 0);
	}
}
void StandardProgram::printHelp(OutStream* dumpTgt){
	dumpTgt->write(name.c_str()); dumpTgt->write('\n');
	dumpTgt->write(summary.c_str()); dumpTgt->write('\n');
	for(uintptr_t i = 0; i<allOptions.size(); i++){
		ArgumentOption* curOpt = allOptions[i];
		if(!(curOpt->isCommon)){ continue; }
		dumpTgt->write("  ");
		dumpTgt->write(curOpt->name.c_str());
		dumpTgt->write(" : ");
		dumpTgt->write(curOpt->summary.c_str());
		dumpTgt->write("\n");
		if(curOpt->usage.size()){
			dumpTgt->write("    ");
			dumpTgt->write(curOpt->usage.c_str());
			dumpTgt->write("\n");
		}
	}
}
void StandardProgram::printVersion(OutStream* dumpTgt){
	dumpTgt->write(version.c_str());
}
void StandardProgram::dumpArguments(OutStream* dumpTgt){
	char intDump[8];
	BytePacker packI(intDump);
	//program data
	//name
		packI.retarget(intDump); packI.packBE64(name.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(name.c_str(), name.size());
	//summary
		packI.retarget(intDump); packI.packBE64(summary.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(summary.c_str(), summary.size());
	//description
		packI.retarget(intDump); packI.packBE64(description.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(description.c_str(), description.size());
	//usage
		packI.retarget(intDump); packI.packBE64(usage.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(usage.c_str(), usage.size());
	//the arguments
	packI.retarget(intDump); packI.packBE64(allOptions.size()); dumpTgt->write(intDump, 8);
	for(uintptr_t i = 0; i<allOptions.size(); i++){
		allOptions[i]->dumpInfo(dumpTgt);
	}
}

ArgumentOptionHelp::ArgumentOptionHelp(){
	isCommon = 0;
	name = "--help";
	summary = "Print out help information.";
	typeCode = "meta";
	extTypeCode = "";
}
ArgumentOptionHelp::~ArgumentOptionHelp(){}
int ArgumentOptionHelp::canParse(uintptr_t argc, char** argv){
	int isHelpA = strcmp(argv[0], "--help") == 0;
	int isHelpB = strcmp(argv[0], "-h") == 0;
	int isHelpC = strcmp(argv[0], "/?") == 0;
	return isHelpA || isHelpB || isHelpC;
}
uintptr_t ArgumentOptionHelp::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	forProg->needRun = 0;
	forProg->needIdiot = 0;
	forProg->printHelp(forProg->useOut);
	return argc;
}

ArgumentOptionVersion::ArgumentOptionVersion(){
	isCommon = 0;
	name = "--version";
	summary = "Print out version information.";
	typeCode = "meta";
	extTypeCode = "";
}
ArgumentOptionVersion::~ArgumentOptionVersion(){}
int ArgumentOptionVersion::canParse(uintptr_t argc, char** argv){
	int isHelpA = strcmp(argv[0], "--version") == 0;
	return isHelpA;
}
uintptr_t ArgumentOptionVersion::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	forProg->needRun = 0;
	forProg->needIdiot = 0;
	forProg->printVersion(forProg->useOut);
	return argc;
}

ArgumentOptionHelpArgdump::ArgumentOptionHelpArgdump(){
	isCommon = 0;
	name = "--help_argdump";
	summary = "Dump out argument information.";
	typeCode = "meta";
	extTypeCode = "";
}
ArgumentOptionHelpArgdump::~ArgumentOptionHelpArgdump(){}
int ArgumentOptionHelpArgdump::canParse(uintptr_t argc, char** argv){
	int isHelpA = strcmp(argv[0], "--help_argdump") == 0;
	return isHelpA;
}
uintptr_t ArgumentOptionHelpArgdump::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	forProg->needRun = 0;
	forProg->needIdiot = 0;
	forProg->dumpArguments(forProg->useOut);
	return argc;
}

ArgumentOptionHelpArgcheck::ArgumentOptionHelpArgcheck(){
	isCommon = 0;
	name = "--help_id10t";
	summary = "Do not actually run, just check arguments.";
	typeCode = "meta";
	extTypeCode = "";
}
ArgumentOptionHelpArgcheck::~ArgumentOptionHelpArgcheck(){}
int ArgumentOptionHelpArgcheck::canParse(uintptr_t argc, char** argv){
	int isHelpA = strcmp(argv[0], "--help_id10t") == 0;
	return isHelpA;
}
uintptr_t ArgumentOptionHelpArgcheck::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	forProg->needRun = 0;
	return 1;
}

ArgumentOptionNamed::ArgumentOptionNamed(const char* theName){
	name = theName;
}
ArgumentOptionNamed::~ArgumentOptionNamed(){}
int ArgumentOptionNamed::canParse(uintptr_t argc, char** argv){
	return strcmp(argv[0], name.c_str()) == 0;
}

ArgumentOptionFlag::ArgumentOptionFlag(const char* theName) : ArgumentOptionNamed(theName){
	value = 0;
	typeCode = "flag";
	extTypeCode = "";
}
ArgumentOptionFlag::~ArgumentOptionFlag(){}
uintptr_t ArgumentOptionFlag::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	value = 1;
	return 1;
}

ArgumentOptionEnumValue::ArgumentOptionEnumValue(const char* className){
	classFlavor = className;
	value = 0;
	numRegistered = 0;
}
ArgumentOptionEnumValue::~ArgumentOptionEnumValue(){}

ArgumentOptionEnum::ArgumentOptionEnum(const char* theName, ArgumentOptionEnumValue* forClass) : ArgumentOptionNamed(theName){
	myClass = forClass;
	setValue = forClass->numRegistered;
	forClass->numRegistered++;
	typeCode = "enum";
	extTypeCode = "";
}
ArgumentOptionEnum::~ArgumentOptionEnum(){}
uintptr_t ArgumentOptionEnum::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	myClass->value = setValue;
	return 1;
}
bool ArgumentOptionEnum::value(){
	return (myClass->value == setValue);
}
void ArgumentOptionEnum::dumpInfo(OutStream* toStr){
	dumpCommonInfo(toStr, 8 + myClass->classFlavor.size() + 1);
	char intDump[8];
	BytePacker packI(intDump);
	packI.retarget(intDump); packI.packBE64(myClass->classFlavor.size()); toStr->write(intDump, 8);
	toStr->write(myClass->classFlavor.c_str(), myClass->classFlavor.size());
	toStr->write(setValue ? 0 : 1);
}

ArgumentOptionInteger::ArgumentOptionInteger(const char* theName) : ArgumentOptionNamed(theName){
	value = 0;
	typeCode = "int";
	extTypeCode = "";
}
ArgumentOptionInteger::~ArgumentOptionInteger(){}
uintptr_t ArgumentOptionInteger::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("Integer option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	const char* lookArg = argv[1];
	if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
	if(strspn(argv[1], "0123456789") != strlen(argv[1])){
		std::string errMess("Malformed integer value (");
			errMess.append(argv[1]);
			errMess.append(") for ");
			errMess.append(name);
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	value = atol(argv[1]);
	return 2;
}
void ArgumentOptionInteger::dumpInfo(OutStream* toStr){
	dumpCommonInfo(toStr, 8);
	char intDump[8];
	BytePacker packI(intDump);
	packI.packBE64(value); toStr->write(intDump, 8);
}

ArgumentOptionIntegerVector::ArgumentOptionIntegerVector(const char* theName) : ArgumentOptionNamed(theName){
	typeCode = "intvec";
	extTypeCode = "";
}
ArgumentOptionIntegerVector::~ArgumentOptionIntegerVector(){}
uintptr_t ArgumentOptionIntegerVector::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("Integer option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	const char* lookArg = argv[1];
	if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
	if(strspn(argv[1], "0123456789") != strlen(argv[1])){
		std::string errMess("Malformed integer value (");
			errMess.append(argv[1]);
			errMess.append(") for ");
			errMess.append(name);
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	value.push_back(atol(argv[1]));
	return 2;
}

ArgumentOptionFloat::ArgumentOptionFloat(const char* theName) : ArgumentOptionNamed(theName){
	value = 0;
	typeCode = "float";
	extTypeCode = "";
}
ArgumentOptionFloat::~ArgumentOptionFloat(){}
uintptr_t ArgumentOptionFloat::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("Float option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	const char* lookArg = argv[1];
	if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
	lookArg += strspn(lookArg, "0123456789");
	if(lookArg[0] == '.'){
		lookArg++;
		lookArg += strspn(lookArg, "0123456789");
	}
	if((lookArg[0] == 'e') || (lookArg[0] == 'E')){
		lookArg++;
		if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
		lookArg += strspn(lookArg, "0123456789");
	}
	if(lookArg[0]){
		std::string errMess("Malformed float value (");
			errMess.append(argv[1]);
			errMess.append(") for ");
			errMess.append(name);
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	value = atof(argv[1]);
	return 2;
}
void ArgumentOptionFloat::dumpInfo(OutStream* toStr){
	dumpCommonInfo(toStr, 8);
	char intDump[8];
	BytePacker packI(intDump);
	packI.packBEDbl(value); toStr->write(intDump, 8);
}

ArgumentOptionFloatVector::ArgumentOptionFloatVector(const char* theName) : ArgumentOptionNamed(theName){
	typeCode = "floatvec";
	extTypeCode = "";
}
ArgumentOptionFloatVector::~ArgumentOptionFloatVector(){}
uintptr_t ArgumentOptionFloatVector::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("Float option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	const char* lookArg = argv[1];
	if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
	lookArg += strspn(lookArg, "0123456789");
	if(lookArg[0] == '.'){
		lookArg++;
		lookArg += strspn(lookArg, "0123456789");
	}
	if((lookArg[0] == 'e') || (lookArg[0] == 'E')){
		lookArg++;
		if((lookArg[0] == '+') || (lookArg[0] == '-')){ lookArg++; }
		lookArg += strspn(lookArg, "0123456789");
	}
	if(lookArg[0]){
		std::string errMess("Malformed float value (");
			errMess.append(argv[1]);
			errMess.append(") for ");
			errMess.append(name);
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	value.push_back(atof(argv[1]));
	return 2;
}

ArgumentOptionString::ArgumentOptionString(const char* theName) : ArgumentOptionNamed(theName){
	typeCode = "string";
	extTypeCode = "";
}
ArgumentOptionString::~ArgumentOptionString(){}
uintptr_t ArgumentOptionString::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("String option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	value = argv[1];
	return 2;
}
void ArgumentOptionString::dumpInfo(OutStream* toStr){
	dumpCommonInfo(toStr, 8 + value.size());
	char intDump[8];
	BytePacker packI(intDump);
	packI.packBE64(value.size()); toStr->write(intDump, 8);
	toStr->write(value.c_str(), value.size());
}

ArgumentOptionStringVector::ArgumentOptionStringVector(const char* theName) : ArgumentOptionNamed(theName){
	typeCode = "stringvec";
	extTypeCode = "";
}
ArgumentOptionStringVector::~ArgumentOptionStringVector(){}
uintptr_t ArgumentOptionStringVector::parse(uintptr_t argc, char** argv, StandardProgram* forProg){
	if(argc < 2){
		std::string errMess("String option ");
			errMess.append(name);
			errMess.append(" requires a value.");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
	std::string newValue = argv[1];
	value.push_back(newValue);
	return 2;
}

ArgumentOptionThreadcount::ArgumentOptionThreadcount() : ArgumentOptionInteger("--thread"){
	value = 1;
	summary = "The number of threads to spin up.";
	usage = "--thread 1";
}
ArgumentOptionThreadcount::~ArgumentOptionThreadcount(){}
void ArgumentOptionThreadcount::idiotCheck(){
	if(value <= 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Need at least one thread.", 0, 0);
	}
}

ArgumentOptionThreadgrain::ArgumentOptionThreadgrain() : ArgumentOptionInteger("--threadgrain"){
	value = 0x010000;
	summary = "The number of things to do per thread.";
	usage = "--threadgrain 65536";
}
ArgumentOptionThreadgrain::~ArgumentOptionThreadgrain(){}
void ArgumentOptionThreadgrain::idiotCheck(){
	if(value <= 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Each thread needs to do at least one thing.", 0, 0);
	}
}

ArgumentOptionFileRead::ArgumentOptionFileRead(const char* theName) : ArgumentOptionString(theName){
	extTypeCode = "fileread";
	required = 0;
}
ArgumentOptionFileRead::~ArgumentOptionFileRead(){}
void ArgumentOptionFileRead::idiotCheck(){
	if(required && (value.size() == 0)){
		std::string errMess("No file to read provided for ");
			errMess.append(name);
			errMess.append(".");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
}
void ArgumentOptionFileRead::dumpInfo(OutStream* toStr){
	uintptr_t totNumExts = 0;
	for(uintptr_t i = 0; i<validExts.size(); i++){
		totNumExts += strlen(validExts[i]);
		totNumExts += 8;
	}
	dumpCommonInfo(toStr, 8 + value.size() + 8 + totNumExts);
	char intDump[8];
	BytePacker packI(intDump);
	packI.packBE64(value.size()); toStr->write(intDump, 8);
	toStr->write(value.c_str(), value.size());
	//extensions
	packI.retarget(intDump); packI.packBE64(validExts.size()); toStr->write(intDump, 8);
	for(uintptr_t i = 0; i<validExts.size(); i++){
		uintptr_t curS = strlen(validExts[i]);
		packI.retarget(intDump); packI.packBE64(curS); toStr->write(intDump, 8);
		toStr->write(validExts[i], curS);
	}
}

ArgumentOptionFileWrite::ArgumentOptionFileWrite(const char* theName) : ArgumentOptionString(theName){
	extTypeCode = "filewrite";
	required = 0;
}
ArgumentOptionFileWrite::~ArgumentOptionFileWrite(){}
void ArgumentOptionFileWrite::idiotCheck(){
	if(required && (value.size() == 0)){
		std::string errMess("No file to write provided for ");
			errMess.append(name);
			errMess.append(".");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
}
void ArgumentOptionFileWrite::dumpInfo(OutStream* toStr){
	uintptr_t totNumExts = 0;
	for(uintptr_t i = 0; i<validExts.size(); i++){
		totNumExts += strlen(validExts[i]);
		totNumExts += 8;
	}
	dumpCommonInfo(toStr, 8 + value.size() + 8 + totNumExts);
	char intDump[8];
	BytePacker packI(intDump);
	packI.packBE64(value.size()); toStr->write(intDump, 8);
	toStr->write(value.c_str(), value.size());
	//extensions
	packI.retarget(intDump); packI.packBE64(validExts.size()); toStr->write(intDump, 8);
	for(uintptr_t i = 0; i<validExts.size(); i++){
		uintptr_t curS = strlen(validExts[i]);
		packI.retarget(intDump); packI.packBE64(curS); toStr->write(intDump, 8);
		toStr->write(validExts[i], curS);
	}
}

ArgumentOptionFileReadVector::ArgumentOptionFileReadVector(const char* theName) : ArgumentOptionStringVector(theName){
	extTypeCode = "fileread";
}
ArgumentOptionFileReadVector::~ArgumentOptionFileReadVector(){}
void ArgumentOptionFileReadVector::dumpInfo(OutStream* toStr){
	uintptr_t totNumExts = 0;
	for(uintptr_t i = 0; i<validExts.size(); i++){
		totNumExts += strlen(validExts[i]);
		totNumExts += 8;
	}
	dumpCommonInfo(toStr, 8 + totNumExts);
	char intDump[8];
	BytePacker packI(intDump);
	packI.retarget(intDump); packI.packBE64(validExts.size()); toStr->write(intDump, 8);
	for(uintptr_t i = 0; i<validExts.size(); i++){
		uintptr_t curS = strlen(validExts[i]);
		packI.retarget(intDump); packI.packBE64(curS); toStr->write(intDump, 8);
		toStr->write(validExts[i], curS);
	}
}

ArgumentOptionFileWriteVector::ArgumentOptionFileWriteVector(const char* theName) : ArgumentOptionStringVector(theName){
	extTypeCode = "filewrite";
}
ArgumentOptionFileWriteVector::~ArgumentOptionFileWriteVector(){}
void ArgumentOptionFileWriteVector::dumpInfo(OutStream* toStr){
	uintptr_t totNumExts = 0;
	for(uintptr_t i = 0; i<validExts.size(); i++){
		totNumExts += strlen(validExts[i]);
		totNumExts += 8;
	}
	dumpCommonInfo(toStr, 8 + totNumExts);
	char intDump[8];
	BytePacker packI(intDump);
	packI.retarget(intDump); packI.packBE64(validExts.size()); toStr->write(intDump, 8);
	for(uintptr_t i = 0; i<validExts.size(); i++){
		uintptr_t curS = strlen(validExts[i]);
		packI.retarget(intDump); packI.packBE64(curS); toStr->write(intDump, 8);
		toStr->write(validExts[i], curS);
	}
}

ArgumentOptionFolderRead::ArgumentOptionFolderRead(const char* theName) : ArgumentOptionString(theName){
	extTypeCode = "folderread";
	required = 0;
}
ArgumentOptionFolderRead::~ArgumentOptionFolderRead(){}
void ArgumentOptionFolderRead::idiotCheck(){
	if(required && (value.size() == 0)){
		std::string errMess("No folder to read provided for ");
			errMess.append(name);
			errMess.append(".");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
}

ArgumentOptionFolderWrite::ArgumentOptionFolderWrite(const char* theName) : ArgumentOptionString(theName){
	extTypeCode = "folderwrite";
	required = 0;
}
ArgumentOptionFolderWrite::~ArgumentOptionFolderWrite(){}
void ArgumentOptionFolderWrite::idiotCheck(){
	if(required && (value.size() == 0)){
		std::string errMess("No folder to write provided for ");
			errMess.append(name);
			errMess.append(".");
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
	}
}

StandardProgramSet::StandardProgramSet(){}
StandardProgramSet::~StandardProgramSet(){}
StandardProgram* StandardProgramSet::parseArguments(int argc, char** argv, InStream* useIn, OutStream* useOut, ErrorLog* useErr){
	//figure out the output/error situation
		OutStream* realOut = useOut;
			if(realOut == 0){
				realOut = new ConsoleOutStream();
			}
		ErrorLog* realErr = useErr;
			if(realErr == 0){
				OutStream* tmpErr = new ConsoleErrStream();
				realErr = new StreamErrorLog(tmpErr);
			}
		int wasError = 0;
		WhodunError throwErr;
		StandardProgram* retProgram = 0;
	//figure out which program to run
		try{
			if(argc == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "No arguments provided.", 0, 0); }
			if((strcmp(argv[0],"--help")==0) || (strcmp(argv[0],"-h")==0) || (strcmp(argv[0],"/?")==0)){
				printHelp(realOut);
				goto end_handler;
			}
			if(strcmp(argv[0],"--version")==0){
				printVersion(realOut);
				goto end_handler;
			}
			if(strcmp(argv[0],"--help_argdump")==0){
				dumpPrograms(realOut);
				goto end_handler;
			}
			std::map<std::string,StandardProgram*(*)()>::iterator progIt = hotPrograms.find(argv[0]);
			if(progIt == hotPrograms.end()){
				std::string errMess(argv[0]);
					errMess.append(" is not a known program.");
				throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, errMess.c_str(), 0, 0);
			}
			retProgram = progIt->second();
		}
		catch(WhodunError& errW){
			realErr->logError(errW);
			wasError = 1;
			throwErr = WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Problem getting routine to run.", 0, 0);
			goto end_handler;
		}
		catch(std::exception& errE){
			realErr->logError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_UNKNOWNEXCEPT, __FILE__, __LINE__, errE.what(), 0, 0);
			wasError = 1;
			throwErr = WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Problem getting routine to run.", 0, 0);
			goto end_handler;
		}
	//let that program parse
		retProgram->useIn = useIn;
		retProgram->useOut = realOut;
		retProgram->useErr = realErr;
		retProgram->parseArguments(argc-1, argv+1);
		if(retProgram->wasError){
			delete(retProgram);
			retProgram = 0;
			wasError = 1;
			throwErr = WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Problem parsing program arguments.", 0, 0);
		}
		else{
			if(realErr != useErr){ retProgram->needKill = retProgram->needKill | 4; }
			if(realOut != useOut){ retProgram->needKill = retProgram->needKill | 2; }
		}
	//clean up and return
		end_handler:
		if(!retProgram){
			if(realErr != useErr){
				OutStream* tmpErr = ((StreamErrorLog*)realErr)->tgtStr;
				delete(realErr);
				tmpErr->close();
				delete(tmpErr);
			}
			if(realOut != useOut){
				realOut->close();
				delete(realOut);
			}
		}
		if(wasError){ throw throwErr; }
		return retProgram;
}
void StandardProgramSet::printHelp(OutStream* dumpTgt){
	dumpTgt->write(name.c_str()); dumpTgt->write('\n');
	dumpTgt->write(summary.c_str()); dumpTgt->write('\n');
	dumpTgt->write('\n');
	std::map<std::string,StandardProgram*(*)()>::iterator progIt;
	for(progIt = hotPrograms.begin(); progIt != hotPrograms.end(); progIt++){
		dumpTgt->write(progIt->first.c_str()); dumpTgt->write('\n');
		StandardProgram* curP = progIt->second();
		dumpTgt->write(curP->summary.c_str()); dumpTgt->write('\n');
		delete(curP);
	}
}
void StandardProgramSet::printVersion(OutStream* dumpTgt){
	dumpTgt->write(version.c_str());
}
void StandardProgramSet::dumpPrograms(OutStream* dumpTgt){
	char intDump[8];
	BytePacker packI(intDump);
	//name
		packI.retarget(intDump); packI.packBE64(name.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(name.c_str(), name.size());
	//summary
		packI.retarget(intDump); packI.packBE64(summary.size()); dumpTgt->write(intDump, 8);
		dumpTgt->write(summary.c_str(), summary.size());
	//number of programs
		uintptr_t numProg = hotPrograms.size();
		packI.retarget(intDump); packI.packBE64(numProg); dumpTgt->write(intDump, 8);
	//programs
		std::map<std::string,StandardProgram*(*)()>::iterator progIt;
		for(progIt = hotPrograms.begin(); progIt != hotPrograms.end(); progIt++){
			//name
				packI.retarget(intDump); packI.packBE64(progIt->first.size()); dumpTgt->write(intDump, 8);
				dumpTgt->write(progIt->first.c_str(), progIt->first.size());
			//quick summary
				StandardProgram* tmpProg = progIt->second();
				packI.retarget(intDump); packI.packBE64(tmpProg->summary.size()); dumpTgt->write(intDump, 8);
				dumpTgt->write(tmpProg->summary.c_str(), tmpProg->summary.size());
			delete(tmpProg);
		}
}



