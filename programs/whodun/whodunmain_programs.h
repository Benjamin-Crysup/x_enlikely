#ifndef WHODUNMAIN_PROGRAMS_H
#define WHODUNMAIN_PROGRAMS_H 1

#include "whodun_args.h"
#include "whodun_stat_data.h"
#include "whodun_stat_table.h"

namespace whodun {

/**A program to convert between types of data tables.*/
class WhodunDataConvertProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunDataConvertProgram();
	/**Tear down*/
	~WhodunDataConvertProgram();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionDataTableRead optTabIn;
	/**The table to convert to.*/
	ArgumentOptionDataTableWrite optTabOut;
};

/**Convert a data table to a text table.*/
class WhodunDataToTableProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunDataToTableProgram();
	/**Tear down*/
	~WhodunDataToTableProgram();
	void baseRun();
	
	/**Whether the text table should have a header.*/
	ArgumentOptionFlag textHeader;
	/**Whether the text table header should have type sigils.*/
	ArgumentOptionFlag textHeadSigil;
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionDataTableRead optTabIn;
	/**The database to write to.*/
	ArgumentOptionTextTableWrite optDatOut;
};

/**A program to convert between types of text tables.*/
class WhodunTableConvertProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunTableConvertProgram();
	/**Tear down*/
	~WhodunTableConvertProgram();
	void baseRun();
	
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionTextTableRead optTabIn;
	/**The table to convert to.*/
	ArgumentOptionTextTableWrite optTabOut;
};

/**Convert a text table to a data table.*/
class WhodunTableToDataProgram : public StandardProgram{
public:
	/**Set up*/
	WhodunTableToDataProgram();
	/**Tear down*/
	~WhodunTableToDataProgram();
	void baseRun();
	
	/**Whether the text table has a header (names come from the first line).*/
	ArgumentOptionFlag textHeader;
	/**Whether the text table has a header with type sigils (type annotations are in the header).*/
	ArgumentOptionFlag textHeadSigil;
	/**The number of threads to spin up.*/
	ArgumentOptionThreadcount optTC;
	/**How many to do in one go, per thread.*/
	ArgumentOptionThreadgrain optChunky;
	/**The table to convert from.*/
	ArgumentOptionTextTableRead optTabIn;
	/**The database to write to.*/
	ArgumentOptionDataTableWrite optDatOut;
};

//TODO

};

#endif


