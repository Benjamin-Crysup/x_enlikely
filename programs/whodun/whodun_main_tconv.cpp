#include "whodunmain_programs.h"

using namespace whodun;

WhodunTableConvertProgram::WhodunTableConvertProgram() : optTabIn(0, "--in", "The table to read in."), optTabOut(0, "--out", "The converted table to write out."){
	name = "tconv";
	summary = "Convert text table formats.";
	version = "whodun tconv 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "tconv --in IN.tsv --out OUT.zlib.bctab";
	allOptions.push_back(&optTC);
	allOptions.push_back(&optChunky);
	allOptions.push_back(&optTabIn);
	allOptions.push_back(&optTabOut);
}
WhodunTableConvertProgram::~WhodunTableConvertProgram(){}
void WhodunTableConvertProgram::baseRun(){
	TextTableReader* inStr = 0;
	TextTableWriter* outStr = 0;
	try{
		//open everything
			uintptr_t numThr = optTC.value;
			ThreadPool usePool(optTC.value);
			uintptr_t chunkS = numThr * optChunky.value;
			inStr = new ExtensionTextTableReader(optTabIn.value.c_str(), numThr, &usePool, useIn);
			outStr = new ExtensionTextTableWriter(optTabOut.value.c_str(), numThr, &usePool, useOut);
		//pump and dump
			TextTable workTab;
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

