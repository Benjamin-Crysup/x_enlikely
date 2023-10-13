
#include <stdio.h>

#include "whodun_args.h"

#include "whodunmain_programs.h"

namespace whodun{

/**A set of programs*/
class WhodunProgramSet : public StandardProgramSet{
public:
	/**Default set up*/
	WhodunProgramSet();
	/**Tear down*/
	~WhodunProgramSet();
};

};

using namespace whodun;

int main(int argc, char** argv){
	try{
		int retCode = 0;
		WhodunProgramSet proggy;
		StandardProgram* proggers = proggy.parseArguments(argc-1, argv+1, 0, 0, 0);
		if(proggers){
			proggers->run();
			retCode = proggers->wasError;
			delete(proggers);
		}
		return retCode;
	}
	catch(std::exception& errE){
		std::cerr << errE.what() << std::endl;
		return 1;
	}
}


WhodunProgramSet::WhodunProgramSet(){
	name = "whodun";
	summary = "Useful programs for interfacing with the whodun library.";
	version = "whodun 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	hotPrograms["dconv"] = makeNewProgram<WhodunDataConvertProgram>;
	hotPrograms["d2t"] = makeNewProgram<WhodunDataToTableProgram>;
	hotPrograms["tconv"] = makeNewProgram<WhodunTableConvertProgram>;
	hotPrograms["t2d"] = makeNewProgram<WhodunTableToDataProgram>;
	//TODO
}
WhodunProgramSet::~WhodunProgramSet(){}

