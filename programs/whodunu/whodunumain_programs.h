#ifndef WHODUNUMAIN_PROGRAMS_H
#define WHODUNUMAIN_PROGRAMS_H 1

#include "whodun_args.h"
#include "whodun_gen_weird.h"
#include "whodun_stat_data.h"
#include "whodun_gen_graph.h"
#include "whodun_stat_table.h"
#include "whodun_gen_seqdata.h"

namespace whodun {

/**A program to convert between types of string files.*/
class WhodunSequenceConvertProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunSequenceConvertProgram();
	/**Tear down*/
	~WhodunSequenceConvertProgram();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionSequenceRead optTabIn;
	/**The table to convert to.*/
	ArgumentOptionSequenceWrite optTabOut;
};

/**A program to convert between types of fastq files.*/
class WhodunFastqConvertProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunFastqConvertProgram();
	/**Tear down*/
	~WhodunFastqConvertProgram();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionFastqRead optTabIn;
	/**The table to convert to.*/
	ArgumentOptionFastqWrite optTabOut;
};

/**Conver a fastq file to a sequence graph.*/
class WhodunFastqToGraphProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunFastqToGraphProgram();
	/**Tear down*/
	~WhodunFastqToGraphProgram();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionFastqRead optTabIn;
	/**The place to write the graph.*/
	ArgumentOptionSeqGraphWrite optGraphOut;
	
	/**How indels are handled.*/
	ArgumentOptionEnumValue indelModel;
	/**Flat indel rate.*/
	ArgumentOptionEnum indelFlatModel;
	/**Any required options.*/
	ArgumentOptionFloatVector indelOptionsD;
};

/**Convert a basic sequence file to a sequence graph.*/
class WhodunSequenceToGraphProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunSequenceToGraphProgram();
	/**Tear down*/
	~WhodunSequenceToGraphProgram();
	void idiotCheck();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionSequenceRead optTabIn;
	/**The place to write the graph.*/
	ArgumentOptionSeqGraphWrite optGraphOut;
	
	/**The characters expected in the sequence.*/
	ArgumentOptionString expectChars;
	/**The first character goes to any of the later characters.*/
	ArgumentOptionStringVector translateMap;
	/**Flat probability of variance/error/mutation.*/
	ArgumentOptionFloat pError;
	/**Flat probability of opening an indel.*/
	ArgumentOptionFloat pOpen;
	/**Flat probability of extending an indel.*/
	ArgumentOptionFloat pExtend;
	/**Flat probability of closing an indel.*/
	ArgumentOptionFloat pClose;
};

//TODO

};

#endif


