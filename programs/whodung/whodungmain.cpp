
#include <stdio.h>

#include "whodun_args.h"

#include "whodungmain_programs.h"

namespace whodun{

/**A set of programs*/
class WhodungProgramSet : public StandardProgramSet{
public:
	/**Default set up*/
	WhodungProgramSet();
	/**Tear down*/
	~WhodungProgramSet();
};

};

using namespace whodun;

int main(int argc, char** argv){
	try{
		int retCode = 0;
		WhodungProgramSet proggy;
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


WhodungProgramSet::WhodungProgramSet(){
	name = "whodung";
	summary = "Programs that use the gpu part of the whodun library.";
	version = "whodunu 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	hotPrograms["gpuinfo"] = makeNewProgram<WhodunGPUInfoProgram>;
	//TODO
}
WhodungProgramSet::~WhodungProgramSet(){}


