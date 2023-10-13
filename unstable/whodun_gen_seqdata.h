#ifndef WHODUN_GEN_SEQDATA_H
#define WHODUN_GEN_SEQDATA_H 1

/**
 * @file
 * @brief Read and write basic sequence data.
 */

#include "whodun_args.h"
#include "whodun_parse.h"
#include "whodun_string.h"
#include "whodun_thread.h"
#include "whodun_container.h"

namespace whodun {

/**A collection of sequences.*/
class SequenceSet{
public:
	/**Set up an empty set.*/
	SequenceSet();
	/**Clean up*/
	~SequenceSet();
	
	/**Storage for names.*/
	StructVector<SizePtrString> saveNames;
	/**Storage for strings.*/
	StructVector<SizePtrString> saveStrs;
	/**Storage for the text itself.*/
	StructVector<char> saveText;
};

/**Read from a sequence file.*/
class SequenceReader{
public:
	/**Set up.*/
	SequenceReader();
	/**Allow subcalsses to tear down.*/
	virtual ~SequenceReader();
	
	/**
	 * Read some rows from the table.
	 * @param toStore The place to put them.
	 * @param numSeqs The number of sequences to read.
	 * @return The number of sequences actually read: if less than numSeqs, have hit eof.
	 */
	virtual uintptr_t read(SequenceSet* toStore, uintptr_t numSeqs) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Random access to a sequence file.*/
class RandacSequenceReader : public SequenceReader{
public:
	/**
	 * Get the number of sequences in the file.
	 * @return The number of sequences.
	 */
	virtual uintmax_t size() = 0;
	/**
	 * Seek to a given sequence.
	 * @param index The sequence to seek to.
	 */
	virtual void seek(uintmax_t index) = 0;
	
	/**
	 * Read a range of sequences.
	 * @param toStore The place to put the sequences.
	 * @param fromIndex The first sequence to read.
	 * @param toIndex The sequence to stop reading at.
	 */
	void readRange(SequenceSet* toStore, uintmax_t fromIndex, uintmax_t toIndex);
};

/**Write to a sequence file.*/
class SequenceWriter{
public:
	/**Set up.*/
	SequenceWriter();
	/**Allow subcalsses to tear down.*/
	virtual ~SequenceWriter();
	
	/**
	 * Write some sequences to the file.
	 * @param toStore The sequences to write.
	 */
	virtual void write(SequenceSet* toStore) = 0;
	/** Close anything this thing opened.*/
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Read from a fasta file.*/
class FastaSequenceReader : public SequenceReader{
public:
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 */
	FastaSequenceReader(InStream* mainFrom);
	/**
	 * Set up a parser for the thing.
	 * @param mainFrom The thing to parse.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	FastaSequenceReader(InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~FastaSequenceReader();
	
	uintptr_t read(SequenceSet* toStore, uintptr_t numSeqs);
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

/**Write to a fasta file.*/
class FastaSequenceWriter : public SequenceWriter{
public:
	/**
	 * Set up a writer for the thing.
	 * @param mainFrom The thing to write to.
	 */
	FastaSequenceWriter(OutStream* mainFrom);
	/**
	 * Set up a writer for the thing.
	 * @param mainFrom The thing to write to.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	FastaSequenceWriter(OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~FastaSequenceWriter();
	
	void write(SequenceSet* toStore);
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

/**Read a chunked up set of sequence data.*/
class ChunkySequenceReader : public RandacSequenceReader{
public:
	/**
	 * Prepare to read chunky sequence.
	 * @param nameAnnotationFile The offsets of the names in the name file.
	 * @param nameFile The name data.
	 * @param sequenceAnnotationFile The offsets of the sequences in the sequence file.
	 * @param sequenceFile The sequence data.
	 */
	ChunkySequenceReader(RandaccInStream* nameAnnotationFile, RandaccInStream* nameFile, RandaccInStream* sequenceAnnotationFile, RandaccInStream* sequenceFile);
	/**
	 * Prepare to read chunky sequence.
	 * @param nameAnnotationFile The offsets of the names in the name file.
	 * @param nameFile The name data.
	 * @param sequenceAnnotationFile The offsets of the sequences in the sequence file.
	 * @param sequenceFile The sequence data.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySequenceReader(RandaccInStream* nameAnnotationFile, RandaccInStream* nameFile, RandaccInStream* sequenceAnnotationFile, RandaccInStream* sequenceFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ChunkySequenceReader();
	
	uintptr_t read(SequenceSet* toStore, uintptr_t numSeqs);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**The name indices.*/
	RandaccInStream* nAtt;
	/**The names*/
	RandaccInStream* nDat;
	/**The sequence indices.*/
	RandaccInStream* sAtt;
	/**The sequence*/
	RandaccInStream* sDat;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Save annotation data for names.*/
	StructVector<char> saveNARB;
	/**Save annotation data for sequences.*/
	StructVector<char> saveSARB;
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next index to report.*/
	uintmax_t focusInd;
	/**The total number of rows in the file.*/
	uintmax_t totalNInd;
	/**The total number of bytes in the name data.*/
	uintmax_t totalNByte;
	/**The total number of bytes in the sequence data.*/
	uintmax_t totalSByte;
};

/**Write a chunked up set of sequence data.*/
class ChunkySequenceWriter : public SequenceWriter{
public:
	/**
	 * Prepare to read chunky sequence.
	 * @param nameAnnotationFile The offsets of the names in the name file.
	 * @param nameFile The name data.
	 * @param sequenceAnnotationFile The offsets of the sequences in the sequence file.
	 * @param sequenceFile The sequence data.
	 */
	ChunkySequenceWriter(OutStream* nameAnnotationFile, OutStream* nameFile, OutStream* sequenceAnnotationFile, OutStream* sequenceFile);
	/**
	 * Prepare to read chunky sequence.
	 * @param nameAnnotationFile The offsets of the names in the name file.
	 * @param nameFile The name data.
	 * @param sequenceAnnotationFile The offsets of the sequences in the sequence file.
	 * @param sequenceFile The sequence data.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySequenceWriter(OutStream* nameAnnotationFile, OutStream* nameFile, OutStream* sequenceAnnotationFile, OutStream* sequenceFile, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ChunkySequenceWriter();
	
	void write(SequenceSet* toStore);
	void close();
	
	/**The name indices.*/
	OutStream* nAtt;
	/**The names*/
	OutStream* nDat;
	/**The sequence indices.*/
	OutStream* sAtt;
	/**The sequence*/
	OutStream* sDat;
	/**The things to run in threads.*/
	std::vector<JoinableThreadTask*> passUnis;
	/**The pool to use, if any.*/
	ThreadPool* usePool;
	/**Staging ground for annotation data.*/
	StructVector<char> packARB;
	/**Staging ground for sending data to output.*/
	StructVector<char> packDatums;
	/**The total number of bytes in the name data.*/
	uintmax_t totalNByte;
	/**The total number of bytes in the sequence data.*/
	uintmax_t totalSByte;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionSequenceReader : public SequenceReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionSequenceReader(const char* fileName, InStream* useStdin = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The input stream to use for standard input, if not default.
	 */
	ExtensionSequenceReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin = 0);
	/**Clean up.*/
	~ExtensionSequenceReader();
	uintptr_t read(SequenceSet* toStore, uintptr_t numRows);
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
	SequenceReader* wrapStr;
};

/**Choose how to open a thing based on its extension.*/
class ExtensionRandacSequenceReader : public RandacSequenceReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacSequenceReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacSequenceReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~ExtensionRandacSequenceReader();
	uintptr_t read(SequenceSet* toStore, uintptr_t numRows);
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
	RandacSequenceReader* wrapStr;
};

/**Open a file based on a file name.*/
class ExtensionSequenceWriter : public SequenceWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionSequenceWriter(const char* fileName, OutStream* useStdout = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionSequenceWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout = 0);
	/**Clean up.*/
	~ExtensionSequenceWriter();
	void write(SequenceSet* toStore);
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
	SequenceWriter* wrapStr;
};

/**Open a sequence reader.*/
class ArgumentOptionSequenceRead : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSequenceRead(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSequenceRead();
};

/**Open a sequence randacc reader.*/
class ArgumentOptionSequenceRandac : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSequenceRandac(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSequenceRandac();
};

/**Open a sequence writer.*/
class ArgumentOptionSequenceWrite : public ArgumentOptionFileWrite{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSequenceWrite(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSequenceWrite();
};

};

#endif

