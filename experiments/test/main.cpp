
#include <iostream>

#include "whodun_oshook.h"

using namespace whodun;

int main(int argc, char** argv){
	printf("Running %s\n", argv[1]);
	SubProcessArguments doArgs;
		doArgs.progName = argv[1];
		doArgs.numArgs = 0;
		doArgs.stdinMeans = WHODUN_SUBPROCESS_NULL;
		doArgs.stdoutMeans = WHODUN_SUBPROCESS_PIPE;
		doArgs.stderrMeans = WHODUN_SUBPROCESS_FILE;
		doArgs.stderrFile = (char*)"lasterror.txt";
		doArgs.numArgs = argc - 2;
		doArgs.progArgs = argv + 2;
	SubProcess doProg(&doArgs);
	#define WORK_BUFF_SIZE 4096
	char workBuff[WORK_BUFF_SIZE];
	while(1){
		uintptr_t numR = doProg.stdOut->read(workBuff, WORK_BUFF_SIZE);
		if(numR == 0){ break; }
		fwrite(workBuff, 1, numR, stdout);
	}
	doProg.stdOut->close();
	printf("\nError Code: %d\n", doProg.wait());
	return 0;
}

