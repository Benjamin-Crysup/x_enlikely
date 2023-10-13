#ifndef WHODUN_STAT_TABLE_H
#define WHODUN_STAT_TABLE_H 1

/**
 * @file
 * @brief Text tables.
 */

#include <vector>
#include <stdint.h>

#include "whodun_args.h"
#include "whodun_parse.h"
#include "whodun_string.h"
#include "whodun_thread.h"
#include "whodun_container.h"

namespace whodun {

/**A row in a text table.*/
typedef struct{
	/**The number of columns in the row.*/
	uintptr_t numCols;
	/**The texts of each column.*/
	SizePtrString* texts;
} TextTableRow;

/**A view of a collection of rows from a text table*/
class TextTableView{
public:
	/**Set up an empty view.*/
	TextTableView();
	/**Clean up*/
	~TextTableView();
	
	/**Storage for rows.*/
	StructVector<TextTableRow> saveRows;
};

/**A collection of rows from a text table.*/
class TextTable : public TextTableView{
public:
	/**Set up an empty table.*/
	TextTable();
	/**Clean up*/
	~TextTable();
	
	/**Storage for strings.*/
	StructVector<SizePtrString> saveStrs;
	/**Storage for the text.*/
	StructVector<char> saveText;
};

/**Filter out elements from a text table.*/
class TextTableFilter {
public:
	/**Set up a single-threaded filter.*/
	TextTableFilter();
	/**
	 * Set up a multi-threaded filter.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TextTableFilter(uintptr_t numThread, ThreadPool* mainPool);
	/**Allow sub-classes.*/
	virtual ~TextTableFilter();
	
	/**
	 * Test whether a row should be included.
	 * @param row The row to test.
	 * @return Whether it passes the filter.
	 */
	virtual int test(TextTableRow* row) = 0;
	
	/**
	 * Run the filter on the table.
	 * @param toMang The table to mangle.
	 * @param toSave The place to put the survivors (this operation can be done in place).
	 */
	void filter(TextTableView* toMang, TextTableView* toSave);
	
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
};

/**Mutate rows in a text table.*/
class TextTableMutate {
public:
	/**Set up a single-threaded filter.*/
	TextTableMutate();
	/**
	 * Set up a multi-threaded mutation.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TextTableMutate(uintptr_t numThread, ThreadPool* mainPool);
	/**Allow sub-classes.*/
	virtual ~TextTableMutate();
	
	/**
	 * Get the amount of space needed for the mutated value.
	 * @param row The row to mutate.
	 * @return The needed number of columns and bytes.
	 */
	virtual std::pair<uintptr_t,uintptr_t> neededSpace(TextTableRow* row) = 0;
	
	/**
	 * Actually perform the mutation.
	 * @param rowS The row to mutate.
	 * @param rowD The row to set.
	 * @param nextCol The next column to use for storage.
	 * @param nextByte The next byte to use for storage.
	 */
	virtual void mutate(TextTableRow* rowS, TextTableRow* rowD, SizePtrString* nextCol, char* nextByte) = 0;
	
	/**
	 * Run the filter on the table.
	 * @param toMang The table to mangle.
	 * @param toSave The place to put the results (this operation can't be done in place).
	 */
	void mutate(TextTableView* toMang, TextTable* toSave);
	
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
};

/**Read from a text table.*/
class TextTableReader{
public:
	/**Set up.*/
	TextTableReader();
	/**Allow subcalsses to tear down.*/
	virtual ~TextTableReader();
	
	/**
	 * Read some rows from the table.
	 * @param toStore The place to put them.
	 * @param numRows The number of rows to read.
	 * @return The number of rows actually read: if less than numRows, have hit eof. This may be greater than the number of entries in toStore: some readers may have an intermediate cull.
	 */
	virtual uintptr_t read(TextTable* toStore, uintptr_t numRows) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Random access to a text table.*/
class RandacTextTableReader : public TextTableReader{
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
	void readRange(TextTable* toStore, uintmax_t fromIndex, uintmax_t toIndex);
};

/**Write to a text table.*/
class TextTableWriter{
public:
	/**Set up.*/
	TextTableWriter();
	/**Allow subcalsses to tear down.*/
	virtual ~TextTableWriter();
	
	/**
	 * Write some rows to the table.
	 * @param toStore The rows to write.
	 */
	virtual void write(TextTableView* toStore) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Read a delimited table.*/
class DelimitedTableReader : public TextTableReader{
public:
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 */
	DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~DelimitedTableReader();
	uintptr_t read(TextTable* toStore, uintptr_t numRows);
	void close();
	
	/**The thing to parse.*/
	InStream* theStr;
	/**Split up rows.*/
	Tokenizer* rowSplitter;
	/**Split up columns.*/
	Tokenizer* colSplitter;
	/**Move overflow text.*/
	MemoryShuttler* charMove;
	/**Save row splits.*/
	StructVector<Token> saveRowS;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Save any overflow text.*/
	StructVector<char> saveTexts;
	/**Whether the file has drained.*/
	int haveDrained;
};

/**Write a delimited table.*/
class DelimitedTableWriter : public TextTableWriter{
public:
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 */
	DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom);
	/**
	 * Set up a delimited input.
	 * @param rowDelim The delimiter for rows.
	 * @param colDelim The delimiter for columns.
	 * @param mainFrom The stream to delimit.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~DelimitedTableWriter();
	void write(TextTableView* toStore);
	void close();
	
	/**The thing to parse.*/
	OutStream* theStr;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Save text to dump.*/
	StructVector<char> saveTexts;
};

/**Set up a TSV reader.*/
class TSVTableReader : public TextTableReader{
public:
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param useEscapes Whether to respect escape sequences.
	 */
	TSVTableReader(InStream* mainFrom, int useEscapes);
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param useEscapes Whether to respect escape sequences.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableReader(InStream* mainFrom, int useEscapes, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableReader();
	uintptr_t read(TextTable* toStore, uintptr_t numRows);
	void close();
	
	/**The base reader.*/
	TextTableReader* baseRead;
	/**Trim and handle any escapes.*/
	TextTableMutate* trimEscape;
	/**Drop empty rows.*/
	TextTableFilter* killEmpties;
	/**The base staging ground for raw loads.*/
	TextTable baseStage;
};

/**Set up a TSV writer.*/
class TSVTableWriter : public TextTableWriter{
public:
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param useEscapes Whether to fix up escape sequences.
	 */
	TSVTableWriter(OutStream* mainFrom, int useEscapes);
	/**
	 * Set up a delimited input.
	 * @param mainFrom The stream to delimit.
	 * @param useEscapes Whether to fix up escape sequences.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableWriter(OutStream* mainFrom, int useEscapes, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableWriter();
	void write(TextTableView* toStore);
	void close();
	
	/**The base reader.*/
	TextTableWriter* baseRead;
	/**Trim and handle any escapes.*/
	TextTableMutate* trimEscape;
	/**The base staging ground for raw loads.*/
	TextTable baseStage;
};

/**Quick sequential access to a bctsv file.*/
class ChunkyTextTableReader : public RandacTextTableReader{
public:
	/**
	 * Open a block compressed text table file.
	 * @param annotationFile The annotation file.
	 * @param dataFile The file with the actual data.
	 */
	ChunkyTextTableReader(RandaccInStream* annotationFile, RandaccInStream* dataFile);
	/**
	 * Open a block compressed text table file.
	 * @param annotationFile The annotation file.
	 * @param dataFile The file with the actual data.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	ChunkyTextTableReader(RandaccInStream* annotationFile, RandaccInStream* dataFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~ChunkyTextTableReader();
	uintptr_t read(TextTable* toStore, uintptr_t numRows);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**The index data.*/
	RandaccInStream* indStr;
	/**The main data.*/
	RandaccInStream* tsvStr;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Save annotation data.*/
	StructVector<char> saveARB;
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The total number of rows in the file.*/
	uintmax_t totalNInd;
	/**The total number of bytes in the data.*/
	uintmax_t totalNByte;
};

/**Write to a bctsv file.*/
class ChunkyTextTableWriter : public TextTableWriter{
public:
	/**
	 * Open a block compressed text table file.
	 * @param annotationFile The annotation file.
	 * @param dataFile The file with the actual data.
	 */
	ChunkyTextTableWriter(OutStream* annotationFile, OutStream* dataFile);
	/**
	 * Open a block compressed text table file.
	 * @param annotationFile The annotation file.
	 * @param dataFile The file with the actual data.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	ChunkyTextTableWriter(OutStream* annotationFile, OutStream* dataFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~ChunkyTextTableWriter();
	void write(TextTableView* toStore);
	void close();
	
	/**The index data.*/
	OutStream* indStr;
	/**The main data.*/
	OutStream* tsvStr;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**The total amount of written data.*/
	uintmax_t totalOutData;
	/**Staging ground for annotation data.*/
	StructVector<char> packARB;
	/**Staging ground for sending data to output.*/
	StructVector<char> packDatums;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionTextTableReader : public TextTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionTextTableReader(const char* fileName, InStream* useStdin = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionTextTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin = 0);
	/**Clean up.*/
	~ExtensionTextTableReader();
	uintptr_t read(TextTable* toStore, uintptr_t numRows);
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
	TextTableReader* wrapStr;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionRandacTextTableReader : public RandacTextTableReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacTextTableReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacTextTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacTextTableReader();
	uintptr_t read(TextTable* toStore, uintptr_t numRows);
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
	RandacTextTableReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionTextTableWriter : public TextTableWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionTextTableWriter(const char* fileName, OutStream* useStdout = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionTextTableWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout = 0);
	/**Clean up.*/
	~ExtensionTextTableWriter();
	void write(TextTableView* toStore);
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
	TextTableWriter* wrapStr;
};

/**Open a text table reader.*/
class ArgumentOptionTextTableRead : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionTextTableRead(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionTextTableRead();
};

/**Open a text table randacc reader.*/
class ArgumentOptionTextTableRandac : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionTextTableRandac(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionTextTableRandac();
};

/**Open a text table writer.*/
class ArgumentOptionTextTableWrite : public ArgumentOptionFileWrite{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionTextTableWrite(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionTextTableWrite();
};

}

#endif


