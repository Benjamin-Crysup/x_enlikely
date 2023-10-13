#ifndef WHODUN_STAT_DATA_H
#define WHODUN_STAT_DATA_H 1

/**
 * @file
 * @brief Databases
 */

#include <map>
#include <string>
#include <vector>
#include <stdint.h>

#include "whodun_args.h"
#include "whodun_thread.h"
#include "whodun_container.h"

namespace whodun {

/**Categorical data.*/
#define WHODUN_DATA_CAT 1
/**Integer data.*/
#define WHODUN_DATA_INT 2
/**Real data.*/
#define WHODUN_DATA_REAL 3
/**(Fixed-length) string data.*/
#define WHODUN_DATA_STR 4

/**An entry in a data table.*/
typedef struct{
	/**Whether the entry is not known.*/
	int isNA;
	/**Save space.*/
	union{
		/**The value, for categorical entries.*/
		uint64_t valC;
		/**The value, for integer entries.*/
		int64_t valI;
		/**The value, for real entries.*/
		double valR;
		/**The value, for string entries.*/
		char* valS;
	};
} DataTableEntry;

/**A header for a data table.*/
class DataTableDescription{
public:
	/**Set up.*/
	DataTableDescription();
	/**Tear down.*/
	~DataTableDescription();
	
	/**The types of data in each column.*/
	std::vector<uintptr_t> colTypes;
	/**The names of the columns.*/
	std::vector<std::string> colNames;
	/**For categorical data, the valid levels.*/
	std::vector< std::map<std::string,uintptr_t> > factorColMap;
	/**The lengths of any string variables.*/
	std::vector<uintptr_t> strLengths;
};

/**A collection of rows from a data table.*/
class DataTable{
public:
	/**Set up an empty table.*/
	DataTable();
	/**Clean up*/
	~DataTable();
	
	/**Storage for the parsed data.*/
	StructVector<DataTableEntry> saveData;
	/**Storage for any text.*/
	StructVector<char> saveText;
};

/**Read from a data table.*/
class DataTableReader{
public:
	/**Set up.*/
	DataTableReader();
	/**Allow subcalsses to tear down.*/
	virtual ~DataTableReader();
	/**
	 * Get the header of this table.
	 * @param toFill The place to put the description.
	 */
	void getDescription(DataTableDescription* toFill);
	/**
	 * Read some rows from the table.
	 * @param toStore The place to put them.
	 * @param numRows The number of rows to read.
	 * @return The number of rows actually read: if less than numRows, have hit eof.
	 */
	virtual uintptr_t read(DataTable* toStore, uintptr_t numRows) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
	/**The description of the table.*/
	DataTableDescription tabDesc;
};

/**Random access to data tables.*/
class RandacDataTableReader : public DataTableReader{
public:
	/**
	 * Get the number of rows in the file.
	 * @return The number of rows.
	 */
	virtual uintmax_t size() = 0;
	/**
	 * Seek to a given row.
	 * @param index The row to seek to.
	 */
	virtual void seek(uintmax_t index) = 0;
	/**
	 * Read a range of rows.
	 * @param toStore The place to put the rows.
	 * @param fromIndex The first row to read.
	 * @param toIndex The row to stop reading at.
	 */
	void readRange(DataTable* toStore, uintmax_t fromIndex, uintmax_t toIndex);
};

/**Write rows to a data table.*/
class DataTableWriter{
public:
	/**
	 * Set up
	 * @param forData The type of data this will be writing.
	 */
	DataTableWriter(DataTableDescription* forData);
	/**Allow subclasses to tear down.*/
	virtual ~DataTableWriter();
	/**
	 * Write some rows to the table.
	 * @param toStore The rows to write.
	 */
	virtual void write(DataTable* toStore) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
	/**The description of the table.*/
	DataTableDescription tabDesc;
};

/**Read from a binary data table file.*/
class BinaryDataTableReader : public DataTableReader{
public:
	/**
	 * Open a data table file.
	 * @param dataFile The file with the actual data.
	 */
	BinaryDataTableReader(InStream* dataFile);
	/**
	 * Open a data table file.
	 * @param dataFile The file with the actual data.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	BinaryDataTableReader(InStream* dataFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~BinaryDataTableReader();
	uintptr_t read(DataTable* toStore, uintptr_t numRows);
	void close();
	
	/**
	 * Read the header data of this table.
	 */
	void readHeader();
	
	/**The main data.*/
	InStream* tsvStr;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**The number of bytes for a row.*/
	uintptr_t rowBytes;
};

/**Read from a binary data table file.*/
class BinaryRandacDataTableReader : public RandacDataTableReader{
public:
	/**
	 * Open a data table file.
	 * @param dataFile The file with the actual data.
	 */
	BinaryRandacDataTableReader(RandaccInStream* dataFile);
	/**
	 * Open a data table file.
	 * @param dataFile The file with the actual data.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	BinaryRandacDataTableReader(RandaccInStream* dataFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~BinaryRandacDataTableReader();
	uintptr_t read(DataTable* toStore, uintptr_t numRows);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**The main data.*/
	RandaccInStream* tsvStr;
	/**The real reader.*/
	BinaryDataTableReader* realRead;
	/**The offset of the first row.*/
	uintmax_t baseLoc;
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The total number of rows in the file.*/
	uintmax_t totalNInd;
};

/**Write to a binary data table file.*/
class BinaryDataTableWriter : public DataTableWriter{
public:
	/**
	 * Prepare to write a data table file.
	 * @param forData The type of data this will be writing.
	 * @param dataFile The file with the actual data.
	 */
	BinaryDataTableWriter(DataTableDescription* forData, OutStream* dataFile);
	/**
	 * Prepare to write a data table file.
	 * @param forData The type of data this will be writing.
	 * @param dataFile The file with the actual data.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	BinaryDataTableWriter(DataTableDescription* forData, OutStream* dataFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~BinaryDataTableWriter();
	void write(DataTable* toStore);
	void close();
	
	/**
	 * Write the header data of this table.
	 */
	void writeHeader();
	
	/**The main data.*/
	OutStream* tsvStr;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Storage for packed data.*/
	StructVector<char> saveText;
	/**The number of bytes for a row.*/
	uintptr_t rowBytes;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionDataTableReader : public DataTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionDataTableReader(const char* fileName, InStream* useStdin = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionDataTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin = 0);
	/**Clean up.*/
	~ExtensionDataTableReader();
	uintptr_t read(DataTable* toStore, uintptr_t numRows);
	void close();
	
	/**
	 * Convenience method to open things up.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The input stream to use for standard input.
	 */
	void openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	DataTableReader* wrapStr;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionRandacDataTableReader : public RandacDataTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacDataTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacDataTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacDataTableReader();
	uintptr_t read(DataTable* toStore, uintptr_t numRows);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**
	 * Convenience method to open things up.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	void openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	RandacDataTableReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionDataTableWriter : public DataTableWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param forData The type of data this will be writing.
	 * @param fileName The name of the file to read.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionDataTableWriter(DataTableDescription* forData, const char* fileName, OutStream* useStdout = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param forData The type of data this will be writing.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionDataTableWriter(DataTableDescription* forData, const char* fileName, int numThread, ThreadPool* mainPool, OutStream* useStdout = 0);
	/**Clean up.*/
	~ExtensionDataTableWriter();
	void write(DataTable* toStore);
	void close();
	
	/**
	 * Convenience method to open things up.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	void openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout);
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The wrapped writer.*/
	DataTableWriter* wrapStr;
};

/**Open a data table reader.*/
class ArgumentOptionDataTableRead : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionDataTableRead(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionDataTableRead();
};

/**Open a data table randacc reader.*/
class ArgumentOptionDataTableRandac : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionDataTableRandac(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionDataTableRandac();
};

/**Open a data table writer.*/
class ArgumentOptionDataTableWrite : public ArgumentOptionFileWrite{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionDataTableWrite(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionDataTableWrite();
};

};

#endif


