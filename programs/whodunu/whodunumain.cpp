
#include <stdio.h>

#include "whodun_args.h"

#include "whodunumain_programs.h"

namespace whodun{

/**A set of programs*/
class WhodunuProgramSet : public StandardProgramSet{
public:
	/**Default set up*/
	WhodunuProgramSet();
	/**Tear down*/
	~WhodunuProgramSet();
};

};

using namespace whodun;

int main(int argc, char** argv){
	try{
		int retCode = 0;
		WhodunuProgramSet proggy;
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


WhodunuProgramSet::WhodunuProgramSet(){
	name = "whodunu";
	summary = "Programs that use the unstable part of the whodun library.";
	version = "whodunu 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	hotPrograms["sconv"] = makeNewProgram<WhodunSequenceConvertProgram>;
	hotPrograms["fqconv"] = makeNewProgram<WhodunFastqConvertProgram>;
	hotPrograms["fq2sg"] = makeNewProgram<WhodunFastqToGraphProgram>;
	hotPrograms["s2sg"] = makeNewProgram<WhodunSequenceToGraphProgram>;
	//TODO
}
WhodunuProgramSet::~WhodunuProgramSet(){}


