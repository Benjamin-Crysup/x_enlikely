#ifndef WHODUN_GEN_WEIRD_H
#define WHODUN_GEN_WEIRD_H 1

/**
 * @file
 * @brief A collection of weird genetics formats.
 */

#include <stdint.h>

#include "whodun_args.h"
#include "whodun_parse.h"
#include "whodun_string.h"
#include "whodun_thread.h"
#include "whodun_container.h"

namespace whodun {

/**A collection of sequences and qualities.*/
class FastqSet{
public:
	/**Set up an empty set.*/
	FastqSet();
	/**Clean up*/
	~FastqSet();
	
	/**Storage for names.*/
	StructVector<SizePtrString> saveNames;
	/**Storage for strings.*/
	StructVector<SizePtrString> saveStrs;
	/**Storage for phred scores.*/
	StructVector<SizePtrString> savePhreds;
	/**Storage for the text itself.*/
	StructVector<char> saveText;
};

/**Read fastq data.*/
class FastqReader{
public:
	/**Set up*/
	FastqReader();
	/**Allow subclasses.*/
	virtual ~FastqReader();
	
	/**
	 * Read some rows from the table.
	 * @param toStore The place to put them.
	 * @param numSeqs The number of sequences to read.
	 * @return The number of sequences actually read: if less than numSeqs, have hit eof.
	 */
	virtual uintptr_t read(FastqSet* toStore, uintptr_t numSeqs) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Write to a fastq file.*/
class FastqWriter{
public:
	/** Set up */
	FastqWriter();
	/**Allow subcalsses to tear down.*/
	virtual ~FastqWriter();
	
	/**
	 * Write some sequences to the file.
	 * @param toStore The sequences to write.
	 */
	virtual void write(FastqSet* toStore) = 0;
	/** Close anything this thing opened.*/
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};


/**Read from a text fastq file.*/
class AsciiFastqReader : public FastqReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 */
	AsciiFastqReader(InStream* mainFrom);
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	AsciiFastqReader(InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~AsciiFastqReader();
	uintptr_t read(FastqSet* toStore, uintptr_t numSeqs);
	void close();
	
	/**The thing to parse.*/
	InStream* theStr;
	/**Split on newline.*/
	Tokenizer* rowSplitter;
	/**Move overflow text.*/
	MemoryShuttler* charMove;
	/**Save row splits.*/
	StructVector<Token> saveRowS;
	/**Save sequence starts.*/
	StructVector<uintptr_t> saveSeqHS;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Save any overflow text.*/
	StructVector<char> saveTexts;
};

/**Write to a fastq file.*/
class AsciiFastqWriter : public FastqWriter{
public:
	/**
	 * Set up a writer for the thing.
	 * @param mainFrom The thing to write to.
	 */
	AsciiFastqWriter(OutStream* mainFrom);
	/**
	 * Set up a writer for the thing.
	 * @param mainFrom The thing to write to.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	AsciiFastqWriter(OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Allow subcalsses to tear down.*/
	~AsciiFastqWriter();
	void write(FastqSet* toStore);
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

/**Choose how to open a thing based on its extension.*/
class ExtensionFastqReader : public FastqReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionFastqReader(const char* fileName, InStream* useStdin = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionFastqReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin = 0);
	/**Clean up.*/
	~ExtensionFastqReader();
	uintptr_t read(FastqSet* toStore, uintptr_t numSeqs);
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
	FastqReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionFastqWriter : public FastqWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionFastqWriter(const char* fileName, OutStream* useStdout = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionFastqWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout = 0);
	/**Clean up.*/
	~ExtensionFastqWriter();
	void write(FastqSet* toStore);
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
	FastqWriter* wrapStr;
};


/**Open a fastq reader.*/
class ArgumentOptionFastqRead : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionFastqRead(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionFastqRead();
};

/**Open a fastq writer.*/
class ArgumentOptionFastqWrite : public ArgumentOptionFileWrite{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionFastqWrite(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionFastqWrite();
};

};

#endif

