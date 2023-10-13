#ifndef WHODUNGMAIN_PROGRAMS_H
#define WHODUNGMAIN_PROGRAMS_H 1

#include "whodun_args.h"
#include "whodun_gen_weird.h"
#include "whodun_stat_data.h"
#include "whodun_stat_table.h"
#include "whodun_gen_seqdata.h"

namespace whodun {

/**Get information on gpus.*/
class WhodunGPUInfoProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunGPUInfoProgram();
	/**Tear down*/
	~WhodunGPUInfoProgram();
	void baseRun();
	
};

//TODO

};

#endif


