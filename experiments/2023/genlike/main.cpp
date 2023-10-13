
#include <stdio.h>

#include "whodun_args.h"

#include "genlike_progs.h"

namespace whodun{

/**A set of programs*/
class GenlikeProgramSet : public StandardProgramSet{
public:
	/**Default set up*/
	GenlikeProgramSet();
	/**Tear down*/
	~GenlikeProgramSet();
};

};

using namespace whodun;

int main(int argc, char** argv){
	try{
		int retCode = 0;
		GenlikeProgramSet proggy;
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


GenlikeProgramSet::GenlikeProgramSet(){
	name = "genlike";
	summary = "Test genetic likelihood using pcr simulation.";
	version = "genlike 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	hotPrograms["sampload"] = makeNewProgram<GenlikeSampleFinalLoadings>;
	hotPrograms["drawread"] = makeNewProgram<GenlikeSampleReadProbs>;
	hotPrograms["prototsv"] = makeNewProgram<GenlikeReadProbToTsv>;
	//TODO
}
GenlikeProgramSet::~GenlikeProgramSet(){}



