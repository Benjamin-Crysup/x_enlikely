#include "whodunumain_programs.h"

using namespace whodun;

WhodunFastqConvertProgram::WhodunFastqConvertProgram() : optTabIn(0, "--in", "The reads to read in."), optTabOut(0, "--out", "The converted reads to write out."){
	name = "fqconv";
	summary = "Convert sequence read formats.";
	version = "whodunu fqconv 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "fqconv --in IN.fq --out OUT.fq.gz";
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optTabOut);
}
WhodunFastqConvertProgram::~WhodunFastqConvertProgram(){}
void WhodunFastqConvertProgram::baseRun(){
	FastqReader* inStr = 0;
	FastqWriter* outStr = 0;
	try{
		//open everything
			uintptr_t numThr = optTC.value;
			ThreadPool usePool(optTC.value);
			uintptr_t chunkS = numThr * optChunky.value;
			inStr = new ExtensionFastqReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
			outStr = new ExtensionFastqWriter(optTabOut.value.c_str(), numThr, &usePool, useOut);
		//pump and dump
			FastqSet workTab;
			while(1){
				uintptr_t numGot = inStr->read(&workTab, chunkS);
				if(numGot == 0){ break; }
				outStr->write(&workTab);
			}
		//close it
			outStr->close(); delete(outStr); outStr = 0;
			inStr->close(); delete(inStr); inStr = 0;
	}
	catch(std::exception& errE){
		if(inStr){ inStr->close(); delete(inStr); }
		if(outStr){ outStr->close(); delete(outStr); }
		throw;
	}
}

