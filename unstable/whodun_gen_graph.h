#ifndef WHODUN_GEN_GRAPH_H
#define WHODUN_GEN_GRAPH_H 1

/**
 * @file
 * @brief Read and write probability annotated sequence graphs.
 */

#include <stdint.h>

#include "whodun_container.h"
#include "whodun_gen_weird.h"

namespace whodun {

/**Basic information about a sequence graph.*/
class SeqGraphHeader{
public:
	/**set up an empty*/
	SeqGraphHeader();
	/**clean up*/
	~SeqGraphHeader();
	/**
	 * Copy another header.
	 * @param toCopy The header to copy.
	 */
	SeqGraphHeader(const SeqGraphHeader& toCopy);
	/**
	 * Assignment operator
	 * @param newVal The value to set to.
	 */
	SeqGraphHeader& operator=(const SeqGraphHeader& newVal);
	
	/**
	 * Read the header data from a stream.
	 * @param readFrom The place to read from.
	 */
	void read(InStream* readFrom);
	
	/**
	 * Write header data to a stream.
	 * @param dumpTo The place to write to.
	 */
	void write(OutStream* dumpTo);
	
	/**The name of the collection.*/
	SizePtrString name;
	/**The version number of the collection.*/
	uintptr_t version;
	/**The base map in use.*/
	SizePtrString baseMap;
	
	/**Storage for bytes.*/
	StructVector<char> saveText;
};

/**A raw collection of sequence graph data.*/
class SeqGraphDataSet{
public:
	/**Make an empty set of contigs.*/
	SeqGraphDataSet();
	/**Clean up.*/
	~SeqGraphDataSet();
	
	/**Lengths of names.*/
	StructVector<uintptr_t> nameLengths;
	/**Number of contigs for each graph.*/
	StructVector<uintptr_t> numContigs;
	/**Number of link nodes for each graph (minus an implicit end node).*/
	StructVector<uintptr_t> numLinks;
	/**The number of link input entries.*/
	StructVector<uintptr_t> numLinkInputs;
	/**The number of link output entries.*/
	StructVector<uintptr_t> numLinkOutputs;
	/**The total number of bases each graph covers (should be the sum of contig sizes, and the length of the representative sequence).*/
	StructVector<uintptr_t> numBases;
	/**Lengths of extra stuff.*/
	StructVector<uintptr_t> extraLengths;
	
	/**Name storage.*/
	StructVector<char> nameTexts;
	/**Number of bases in each contig.*/
	StructVector<uintptr_t> contigSizes;
	/**Pairs of start contig/end link indices, sorted by link, then contig.*/
	StructVector<uintptr_t> inputLinkData;
	/**Pairs of start link/end contig indices, sorted by link, then contig.*/
	StructVector<uintptr_t> outputLinkData;
	/**Probabilities along each contig: (add open,add ext,add close,skip open,skip ext,skip close,A,C,G,T,...)add open,add ext,add close*/
	StructVector<float> contigProbs;
	/**The probabilities of each link output.*/
	StructVector<float> linkProbs;
	/**Representative sequences.*/
	StructVector<char> seqTexts;
	/**The suffix arrays.*/
	StructVector<uintptr_t> suffixIndices;
	/**Extra stuff.*/
	StructVector<char> extraTexts;
};

/**Read stuff from a sequence graph file.*/
class SeqGraphReader{
public:
	/**Set up.*/
	SeqGraphReader();
	/**Allow subcalsses to tear down.*/
	virtual ~SeqGraphReader();
	
	/**The header for this set of data.*/
	SeqGraphHeader thisHead;
	
	/**
	 * Read some graphs.
	 * @param numSeqs The number of graphs to read.
	 * @param toStore The place to store the read data.
	 * @return The number of graphs actually read: if less than numSeqs, have hit eof.
	 */
	virtual uintptr_t read(uintptr_t numSeqs, SeqGraphDataSet* toStore) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Random access to a sequence graph file.*/
class RandacSeqGraphReader : public SeqGraphReader{
public:
	/**
	 * Get the number of sequence graphs in the file.
	 * @return The number of sequence graphs.
	 */
	virtual uintmax_t size() = 0;
	/**
	 * Seek to a given sequence.
	 * @param index The sequence to seek to.
	 */
	virtual void seek(uintmax_t index) = 0;
	
	/**
	 * Read a range of graphs.
	 * @param fromIndex The first graph to read.
	 * @param toIndex The graph to stop reading at.
	 * @param toStore The place to store the read data.
	 */
	void readRange(uintmax_t fromIndex, uintmax_t toIndex, SeqGraphDataSet* toStore);
};

/**Write to a sequence graph.*/
class SeqGraphWriter{
public:
	/**Set up.*/
	SeqGraphWriter();
	/**Allow subcalsses to tear down.*/
	virtual ~SeqGraphWriter();
	
	/**
	 * Write some sequence graphs to a file.
	 * @param toStore The data to write.
	 */
	virtual void write(SeqGraphDataSet* toStore) = 0;
	/** Close anything this thing opened.*/
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Read a chunkified sequence graph.*/
class ChunkySeqGraphReader : public RandacSeqGraphReader{
public:
	/**
	 * Prepare to read graph info.
	 * @param fileInd The index of the stuff.
	 * @param fileName The name text.
	 * @param fileCSize The contig sizes.
	 * @param fileILnk The input link data.
	 * @param fileOLnk The output link data.
	 * @param fileCPro The contig probability data.
	 * @param fileLPro The link probability data.
	 * @param fileRSeq The representative sequences.
	 * @param fileSuff The suffix array data.
	 * @param fileExtra The extra crap.
	 */
	ChunkySeqGraphReader(RandaccInStream* fileInd, RandaccInStream* fileName, RandaccInStream* fileCSize, RandaccInStream* fileILnk, RandaccInStream* fileOLnk, RandaccInStream* fileCPro, RandaccInStream* fileLPro, RandaccInStream* fileRSeq, RandaccInStream* fileSuff, RandaccInStream* fileExtra);
	/**
	 * Prepare to read graph info.
	 * @param fileInd The index of the stuff.
	 * @param fileName The name text.
	 * @param fileCSize The contig sizes.
	 * @param fileILnk The input link data.
	 * @param fileOLnk The output link data.
	 * @param fileCPro The contig probability data.
	 * @param fileLPro The link probability data.
	 * @param fileRSeq The representative sequences.
	 * @param fileSuff The suffix array data.
	 * @param fileExtra The extra crap.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySeqGraphReader(RandaccInStream* fileInd, RandaccInStream* fileName, RandaccInStream* fileCSize, RandaccInStream* fileILnk, RandaccInStream* fileOLnk, RandaccInStream* fileCPro, RandaccInStream* fileLPro, RandaccInStream* fileRSeq, RandaccInStream* fileSuff, RandaccInStream* fileExtra, uintptr_t numThread, ThreadPool* mainPool);
	/**Tear down*/
	~ChunkySeqGraphReader();
	
	uintptr_t read(uintptr_t numSeqs, SeqGraphDataSet* toStore);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**Bulk convert integers.*/
	ParallelForLoop* doIConv;
	/**Bulk convert floats.*/
	ParallelForLoop* doFConv;
	/**Actually pack things down.*/
	ParallelForLoop* doConv;
	/**The threads to use.*/
	ThreadPool* usePool;
	/**Save index information.*/
	StructVector<char> saveInd;
	/**Temporary storage for data.*/
	StructVector<char> tmpText;
	/**Whether the next thing needs to seek.*/
	int needSeek;
	/**The offset to the first thing in the index data.*/
	uintmax_t headOffset;
	/**The number of graphs in this thing.*/
	uintmax_t numGraphs;
	/**The next graph to report.*/
	uintmax_t nextGraph;
	
	/**The index of the stuff.*/
	RandaccInStream* fInd;
	/**The name text.*/
	RandaccInStream* fName;
	/**The contig sizes.*/
	RandaccInStream* fCSize;
	/**The input link data.*/
	RandaccInStream* fILnk;
	/**The output link data.*/
	RandaccInStream* fOLnk;
	/**The contig probability data.*/
	RandaccInStream* fCPro;
	/**The link probability data.*/
	RandaccInStream* fLPro;
	/**The representative sequences.*/
	RandaccInStream* fRSeq;
	/**The suffix array data.*/
	RandaccInStream* fSuff;
	/**The extra crap.*/
	RandaccInStream* fExtra;
};

/**Write packed up sequence graph data.*/
class ChunkySeqGraphWriter : public SeqGraphWriter{
public:
	/**
	 * Prepare to write graph info.
	 * @param theHead The header data for the graph.
	 * @param fileInd The index of the stuff.
	 * @param fileName The name text.
	 * @param fileCSize The contig sizes.
	 * @param fileILnk The input link data.
	 * @param fileOLnk The output link data.
	 * @param fileCPro The contig probability data.
	 * @param fileLPro The link probability data.
	 * @param fileRSeq The representative sequences.
	 * @param fileSuff The suffix array data.
	 * @param fileExtra The extra crap.
	 */
	ChunkySeqGraphWriter(SeqGraphHeader* theHead, OutStream* fileInd, OutStream* fileName, OutStream* fileCSize, OutStream* fileILnk, OutStream* fileOLnk, OutStream* fileCPro, OutStream* fileLPro, OutStream* fileRSeq, OutStream* fileSuff, OutStream* fileExtra);
	/**
	 * Prepare to write graph info.
	 * @param theHead The header data for the graph.
	 * @param fileInd The index of the stuff.
	 * @param fileName The name text.
	 * @param fileCSize The contig sizes.
	 * @param fileILnk The input link data.
	 * @param fileOLnk The output link data.
	 * @param fileCPro The contig probability data.
	 * @param fileLPro The link probability data.
	 * @param fileRSeq The representative sequences.
	 * @param fileSuff The suffix array data.
	 * @param fileExtra The extra crap.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySeqGraphWriter(SeqGraphHeader* theHead, OutStream* fileInd, OutStream* fileName, OutStream* fileCSize, OutStream* fileILnk, OutStream* fileOLnk, OutStream* fileCPro, OutStream* fileLPro, OutStream* fileRSeq, OutStream* fileSuff, OutStream* fileExtra, uintptr_t numThread, ThreadPool* mainPool);
	/**Tear down*/
	~ChunkySeqGraphWriter();
	
	void write(SeqGraphDataSet* toStore);
	void close();
	
	/**The header for this set of data.*/
	SeqGraphHeader thisHead;
	/**Bulk convert integers.*/
	ParallelForLoop* doIConv;
	/**Bulk convert floats.*/
	ParallelForLoop* doFConv;
	/**The threads to use.*/
	ThreadPool* usePool;
	/**Save index information.*/
	StructVector<char> saveInd;
	/**Temporary storage for data.*/
	StructVector<char> tmpText;
	
	/**The index of the stuff.*/
	OutStream* fInd;
	/**The name text.*/
	OutStream* fName;
	/**The contig sizes.*/
	OutStream* fCSize;
	/**The input link data.*/
	OutStream* fILnk;
	/**The output link data.*/
	OutStream* fOLnk;
	/**The contig probability data.*/
	OutStream* fCPro;
	/**The link probability data.*/
	OutStream* fLPro;
	/**The representative sequences.*/
	OutStream* fRSeq;
	/**The suffix array data.*/
	OutStream* fSuff;
	/**The extra crap.*/
	OutStream* fExtra;
	
	/**Byte count.*/
	uintmax_t numName;
	/**Byte count.*/
	uintmax_t numCSize;
	/**Byte count.*/
	uintmax_t numILnk;
	/**Byte count.*/
	uintmax_t numOLnk;
	/**Byte count.*/
	uintmax_t numCPro;
	/**Byte count.*/
	uintmax_t numLPro;
	/**Byte count.*/
	uintmax_t numRSeq;
	/**Byte count.*/
	uintmax_t numSuff;
	/**Byte count.*/
	uintmax_t numExtra;
};

/**Read stuff from a sequence graph file.*/
class ExtensionSeqGraphReader : public SeqGraphReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param useStdin The stream to use for standard input, if not default.
	 */
	ExtensionSeqGraphReader(const char* fileName, InStream* useStdin = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The stream to use for standard input, if not default.
	 */
	ExtensionSeqGraphReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin = 0);
	/**Clean up*/
	~ExtensionSeqGraphReader();
	uintptr_t read(uintptr_t numSeqs, SeqGraphDataSet* toStore);
	void close();
	
	/**
	 * Convenience method to open things up.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdin The stream to use for standard input, if not default.
	 */
	void openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin);
	
	/**The base streams.*/
	std::vector<InStream*> baseStrs;
	/**The wrapped reader.*/
	SeqGraphReader* wrapStr;
};

/**Random access to a sequence graph file.*/
class ExtensionRandacSeqGraphReader : public RandacSeqGraphReader{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 */
	ExtensionRandacSeqGraphReader(const char* fileName);
	/**
	 * Figure out what to do based on the file name.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 */
	ExtensionRandacSeqGraphReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~ExtensionRandacSeqGraphReader();
	uintptr_t read(uintptr_t numSeqs, SeqGraphDataSet* toStore);
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
	std::vector<RandaccInStream*> baseStrs;
	/**The wrapped reader.*/
	RandacSeqGraphReader* wrapStr;
};

/**Write to a sequence graph.*/
class ExtensionSeqGraphWriter : public SeqGraphWriter{
public:
	/**
	 * Figure out what to do based on the file name.
	 * @param theHead The header data for the graph.
	 * @param fileName The name of the file to read.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionSeqGraphWriter(SeqGraphHeader* theHead, const char* fileName, OutStream* useStdout = 0);
	/**
	 * Figure out what to do based on the file name.
	 * @param theHead The header data for the graph.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	ExtensionSeqGraphWriter(SeqGraphHeader* theHead, const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout = 0);
	/**Clean up*/
	~ExtensionSeqGraphWriter();
	void write(SeqGraphDataSet* toStore);
	void close();
	
	/**
	 * Convenience method to open things up.
	 * @param theHead The header data for the graph.
	 * @param fileName The name of the file to read.
	 * @param numThread The number of threads to use for the base reader.
	 * @param mainPool The threads to use for reading.
	 * @param useStdout The stream to use for standard output, if not default.
	 */
	void openUp(SeqGraphHeader* theHead, const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout);
	
	/**The base streams.*/
	std::vector<OutStream*> baseStrs;
	/**The wrapped reader.*/
	SeqGraphWriter* wrapStr;
};

/**Open a sequence graph reader.*/
class ArgumentOptionSeqGraphRead : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSeqGraphRead(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSeqGraphRead();
};

/**Open a sequence graph randacc reader.*/
class ArgumentOptionSeqGraphRandac : public ArgumentOptionFileRead{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSeqGraphRandac(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSeqGraphRandac();
};

/**Open a sequence graph writer.*/
class ArgumentOptionSeqGraphWrite : public ArgumentOptionFileWrite{
public:
	/**
	 * Set up
	 * @param needed Whether this value must be provided.
	 * @param theName The name of the argument.
	 * @param useDesc What this thing is.
	 */
	ArgumentOptionSeqGraphWrite(int needed, const char* theName, const char* useDesc);
	/**Tear down.*/
	~ArgumentOptionSeqGraphWrite();
};

//TODO
/*
Reverse complement?
*/

};

/*

Need contigs and contig connectivity map.
Each contig needs a probability array.
Each contig link node needs a probability map.

Contig files themselves need
OVERALL:
	Reference name.
	Version number.
	Base map : list of bases.
PER-ENTRY:
	Name.
	Contigs.
	Link nodes between contigs : no input can be >= any output, link node 0 is the start, link node NumNode is the end.

Probability files need
OVERALL:
	Reference name.
	Version number.
	Base map
PER-ENTRY:
	PER-CONTIG:
		(P(OEC_skip)P(ACGT))[N] : probability of skipping bases, and probabiilty of bases
		(P(OEC_add))[N+1] : probability of adding bases
	PER-LINKNODE:
		P(Link)[] : probability of going to each output
		//the inverse can be figured in reverse

Annotation files need
OVERALL:
	Reference name.
	Version number.
	Base map
PER-ENTRY:
	Representative sequence (concatenate contigs)
	Suffix array (of the representative sequence)

PE_skip[i] + PC_skip[i] = 1
PE_add[i] + PC_add[i] = 1
PO_skip[i] + PO_add[i] <= 1
P_A + P_B + ... = 1.
Note that PO_add[N] < 1

Also want a way to get contigs and probabilities from fastq (or convert a fastq to such).
*/

#endif

