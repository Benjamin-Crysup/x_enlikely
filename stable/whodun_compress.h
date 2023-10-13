#ifndef WHODUN_COMPRESS_H
#define WHODUN_COMPRESS_H 1

/**
 * @file
 * @brief Organized compression.
 */

#include <string>
#include <vector>
#include <stdint.h>

#include <zlib.h>

#include "whodun_ermac.h"
#include "whodun_oshook.h"
#include "whodun_string.h"
#include "whodun_thread.h"

namespace whodun {

/**Perform compression.*/
class CompressionMethod{
public:
	/**Prepare some space.*/
	CompressionMethod();
	/**Allow subclasses to deconstruct.*/
	virtual ~CompressionMethod();
	
	/**
	 * Compress some data.
	 * @param theData The data to compress.
	 */
	virtual void compressData(SizePtrString theData) = 0;
	
	/**The compressed data.*/
	SizePtrString compData;
	/**The number of bytes allocated for the compressed data.*/
	uintptr_t allocSize;
};

/**Perform decompression.*/
class DecompressionMethod{
public:
	/**Prepare some space.*/
	DecompressionMethod();
	/**Allow subclasses to deconstruct.*/
	virtual ~DecompressionMethod();
	
	/**
	 * Decompress some data.
	 * @param theComp The data to decompress.
	 */
	virtual void expandData(SizePtrString theComp) = 0;
	
	/**The decompressed data.*/
	SizePtrString theData;
	/**The number of bytes allocated for the compressed data.*/
	uintptr_t allocSize;
};

/**Generate Compression and Decompression Methods*/
class CompressionFactory{
public:
	/**Allow subclasses to tear down.*/
	virtual ~CompressionFactory();
	/**
	 * Make a new compression method.
	 * @return The allocated item.
	 */
	virtual CompressionMethod* makeZip() = 0;
	/**
	 * Make a new decompression method.
	 * @return The allocated item.
	 */
	virtual DecompressionMethod* makeUnzip() = 0;
};

/**Compress by doing nothing.*/
class RawCompressionFactory : public CompressionFactory{
public:
	CompressionMethod* makeZip();
	DecompressionMethod* makeUnzip();
};

/**Compress with the deflate algorithm.*/
class DeflateCompressionFactory : public CompressionFactory{
public:
	CompressionMethod* makeZip();
	DecompressionMethod* makeUnzip();
};

/**Compress with gzip (optionally with block compression info).*/
class GZipCompressionFactory : public CompressionFactory{
public:
	/**Set up the defaults.*/
	GZipCompressionFactory();
	CompressionMethod* makeZip();
	DecompressionMethod* makeUnzip();
	/**Add block compression metadata.*/
	int addBlockComp;
};

/**Out to gzip file.*/
class GZipOutStream : public OutStream{
public:
	/**
	 * Open the file.
	 * @param append Whether to append to a file if it is already there.
	 * @param fileName The name of the file.
	 */
	GZipOutStream(int append, const char* fileName);
	/**Clean up and close.*/
	~GZipOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**In from gzip file.*/
class GZipInStream : public InStream{
public:
	/**
	 * Open the file.
	 * @param fileName The name of the file.
	 */
	GZipInStream(const char* fileName);
	/**Clean up and close.*/
	~GZipInStream();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	/**The base file.*/
	gzFile baseFile;
	/**The name of the file.*/
	std::string myName;
};

/**Bytes for an annotation entry.*/
#define WHODUN_BLOCKCOMP_ANNOT_ENTLEN 32
/**The number of entries to load in memory: less jumping.*/
#define WHODUN_BLOCKCOMPIN_LASTLINESEEK 64

/**
 * Get the size of a block compressed file.
 * @param annotFN The name of the annotation file.
 * @return The size.
 */
uintmax_t blockCompressedFileGetSize(const char* annotFN);

/**Block compress output.*/
class BlockCompOutStream : public OutStream{
public:
	/**
	 * Open up the file to dump to.
	 * @param append Whether to append to a file if it is already there.
	 * @param blockSize The size of the compressed blocks.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth);
	/**
	 * Open up the file to dump to.
	 * @param append Whether to append to a file if it is already there.
	 * @param blockSize The size of the compressed blocks.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 * @param numThreads The number of threads to spawn.
	 * @param useThreads The threads to use.
	 */
	BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth, uintptr_t numThreads, ThreadPool* useThreads);
	/**Clean up and close.*/
	~BlockCompOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void flush();
	void close();
	/**
	 * Get the number of (uncompressed) bytes already written.
	 * @return The number of uncompressed bytes.
	 */
	uintmax_t tell();
	
	/**The number of bytes to accumulate before dumping a block.*/
	uintptr_t chunkSize;
	/**The total number of uncompressed bytes written to the start of the current block.*/
	uintmax_t preCompBS;
	/**The total number of compressed bytes written to the start of the current block.*/
	uintmax_t postCompBS;
	/**The running total of bytes written: preCompBS can get out of sync with this.*/
	uintmax_t totalWrite;
	/**The data file.*/
	OutStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	OutStream* annotF;
	
	/**The threads to use for compression.*/
	ThreadPool* compThreads;
	/**The number of bytes marshalled.*/
	uintptr_t numMarshal;
	/**A place to store data before compression.*/
	char* chunkMarshal;
	/**Compression tasks.*/
	std::vector<JoinableThreadTask*> threadPass;
	/**
	 * Compress everything in and dump.
	 * @param numDump The number of bytes to dump.
	 * @param dumpFrom The bytes to dump.
	 */
	void compressAndOut(uintptr_t numDump, const char* dumpFrom);
};

/**Read a block compressed input stream.*/
class BlockCompInStream : public RandaccInStream{
public:
	/**
	 * Open up a blcok compressed file.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 */
	BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth);
	/**
	 * Open up a blcok compressed file.
	 * @param mainFN The name of the data file.
	 * @param annotFN The name of the annotation file.
	 * @param compMeth The compression method to use for the blocks.
	 * @param numThreads The number of threads to spawn.
	 * @param useThreads The threads to use.
	 */
	BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth, uintptr_t numThreads, ThreadPool* useThreads);
	/**Clean up and close.*/
	~BlockCompInStream();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	void seek(uintmax_t toAddr);
	uintmax_t tell();
	uintmax_t size();
	
	/**The number of blocks in the file.*/
	uintmax_t numBlocks;
	/**The next block to read.*/
	uintmax_t nextBlock;
	/**The current location in the file.*/
	uintmax_t totalReads;
	/**The data file.*/
	RandaccInStream* mainF;
	/**The annotation file. Quads of pre-comp address, post-comp address, pre-comp len, post-comp len.*/
	RandaccInStream* annotF;
	/**The number of bytes allocated for loading compressed data.*/
	uintptr_t numMarshal;
	/**A place to store data during decompression.*/
	char* chunkMarshal;
	
	/**The threads to use for compression.*/
	ThreadPool* compThreads;
	/**Compression tasks.*/
	std::vector<JoinableThreadTask*> threadPass;
	/**The index of the block containing remaining bytes.*/
	uintptr_t blockRemainIndex;
	/**The number of remaining bytes from the last block.*/
	uintptr_t blockRemainSize;
	/**The offset to the first remaining byte in the last block.*/
	uintptr_t blockRemainOffset;
	
	/**Whether the next read should handle a seek in the main file.*/
	int seekOutstanding;
	/**Whether the size has already been calculated.*/
	int knowSize;
	/**Save the size for later.*/
	uintmax_t saveSize;
};

};

#endif

