#ifndef WHODUN_GEN_GRAPH_ALIGN_H
#define WHODUN_GEN_GRAPH_ALIGN_H 1

/**
 * @file
 * @brief Align probability annotated sequence graphs.
 */

#include "whodun_gen_graph.h"

namespace whodun{

/**The type marker for matches.*/
#define WHODUN_JOINT_TYPE_MATCH 'M'
/**The type marker for true insertions.*/
#define WHODUN_JOINT_TYPE_INSERT_REF 'I'
/**The type marker for false insertions.*/
#define WHODUN_JOINT_TYPE_INSERT_READ 'i'
/**The type marker for true deletions.*/
#define WHODUN_JOINT_TYPE_DELETE_REF 'X'
/**The type marker for false deletions.*/
#define WHODUN_JOINT_TYPE_DELETE_READ 'x'
/**The type marker for following a link on the reference.*/
#define WHODUN_JOINT_TYPE_LINK_REF 'L'
/**The type marker for following a link on the read.*/
#define WHODUN_JOINT_TYPE_LINK_READ 'l'

/**An event in a joint graph alignment.*/
typedef struct{
	/**The type of this event.*/
	unsigned int type;
	/**Keep space usage down.*/
	union{
		/**The number of bases that match.*/
		unsigned int matchLength;
		/**The number of bases inserted w.r.t. the reference that represent actual variation.*/
		unsigned int insertRefLength;
		/**The number of bases inserted that represent the sequencer screwing up.*/
		unsigned int insertReadLength;
		/**The number of bases deleted w.r.t. the reference that represent actual variation.*/
		unsigned int deleteRefLength;
		/**The number of bases deleted that represent the sequencer screwing up.*/
		unsigned int deleteReadLength;
		/**The link followed from the reference.*/
		unsigned int linkRefIndex;
		/**The link followed from the read.*/
		unsigned int linkReadIndex;
	};
} JointAlignmentEntry;

/**A single alignment.*/
typedef struct{
	/**The read being aligned.*/
	uintmax_t readIndex;
	/**The reference graph this is aligned against.*/
	uintmax_t refIndex;
	/**The log joint probability of the alignment.*/
	double probability;
	/**The contig the alignment starts on on the reference.*/
	uintptr_t startContigRef;
	/**The index within the reference contig.*/
	uintptr_t startIndexRef;
	/**The contigs the alignment starts on on the read.*/
	uintptr_t startContigRead;
	/**The index within the read contig.*/
	uintptr_t startIndexRead;
	/**The number of entries in the alignment.*/
	uintptr_t numEntries;
	/**The offset of the entry in a larger array.*/
	uintptr_t entryOffset;
} JointAlignment;

/**A set of alignments.*/
class JointAlignmentSet{
public:
	/**Set up an empty.*/
	JointAlignmentSet();
	/**Clean up.*/
	~JointAlignmentSet();
	
	/**The number of alignment entries for each alignment.*/
	StructVector<JointAlignment> alignments;
	/**The entries for the alignments.*/
	StructVector<JointAlignmentEntry> entries;
};

/**Read stuff from a sequence graph alignment file.*/
class SeqGraphAlignmentReader{
public:
	/**Set up.*/
	SeqGraphAlignmentReader();
	/**Allow subcalsses to tear down.*/
	virtual ~SeqGraphAlignmentReader();
	
	/**
	 * Read some graphs.
	 * @param numAlign The number of alignment entries to read.
	 * @param toStore The place to store the alignment data.
	 * @return The number of entries actually read: if less than numSeqs, have hit eof.
	 */
	virtual uintptr_t read(uintptr_t numAlign, JointAlignmentSet* toStore) = 0;
	/** Close anything this thing opened. */
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Random access to a sequence graph alignment file.*/
class RandacSeqGraphAlignmentReader : public SeqGraphAlignmentReader{
public:
	/**
	 * Get the number of alignment entries in the file.
	 * @return The number of entries.
	 */
	virtual uintmax_t size() = 0;
	/**
	 * Seek to a given entry.
	 * @param index The entry to seek to.
	 */
	virtual void seek(uintmax_t index) = 0;
	
	/**
	 * Read a range of alignment entries.
	 * @param fromIndex The first entry to read.
	 * @param toIndex The entry to stop reading at.
	 * @param toStore The place to store the read data.
	 */
	void readRange(uintmax_t fromIndex, uintmax_t toIndex, JointAlignmentSet* toStore);
};

/**Write to a sequence graph.*/
class SeqGraphAlignmentWriter{
public:
	/**Set up.*/
	SeqGraphAlignmentWriter();
	/**Allow subcalsses to tear down.*/
	virtual ~SeqGraphAlignmentWriter();
	
	/**
	 * Write some alignment entries to a file.
	 * @param toStore The data to write.
	 */
	virtual void write(JointAlignmentSet* toStore) = 0;
	/** Close anything this thing opened.*/
	virtual void close() = 0;
	
	/**Whether this has been closed.*/
	int isClosed;
};

/**Random access to a sequence graph alignment file.*/
class ChunkySeqGraphAlignmentReader : public RandacSeqGraphAlignmentReader{
public:
	/**
	 * Prepare to read alignment data.
	 * @param fileInd The index.
	 * @param fileEntries The alignment data file.
	 */
	ChunkySeqGraphAlignmentReader(RandaccInStream* fileInd, RandaccInStream* fileEntries);
	/**
	 * Prepare to read alignment data.
	 * @param fileInd The index.
	 * @param fileEntries The alignment data file.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySeqGraphAlignmentReader(RandaccInStream* fileInd, RandaccInStream* fileEntries, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~ChunkySeqGraphAlignmentReader();
	uintptr_t read(uintptr_t numAlign, JointAlignmentSet* toStore);
	void close();
	uintmax_t size();
	void seek(uintmax_t index);
	
	/**Bulk convert the common data.*/
	ParallelForLoop* doIConv;
	/**Bulk convert base entry data.*/
	ParallelForLoop* doEConv;
	/**The threads to use.*/
	ThreadPool* usePool;
	/**Save index information.*/
	StructVector<char> saveInd;
	/**Temporary storage for data.*/
	StructVector<char> tmpText;
	/**Whether the next thing needs to seek.*/
	int needSeek;
	/**The number of graphs in this thing.*/
	uintmax_t numGraphs;
	/**The next graph to report.*/
	uintmax_t nextGraph;
	
	/**The index of the stuff.*/
	RandaccInStream* fInd;
	/**The alignment data.*/
	RandaccInStream* fEnt;
};

/**Write a sequence graph alignment file.*/
class ChunkySeqGraphAlignmentWriter : public SeqGraphAlignmentWriter{
public:
	/**
	 * Prepare to write alignment data.
	 * @param fileInd The index.
	 * @param fileEntries The alignment data file.
	 */
	ChunkySeqGraphAlignmentWriter(OutStream* fileInd, OutStream* fileEntries);
	/**
	 * Prepare to write alignment data.
	 * @param fileInd The index.
	 * @param fileEntries The alignment data file.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	ChunkySeqGraphAlignmentWriter(OutStream* fileInd, OutStream* fileEntries, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up*/
	~ChunkySeqGraphAlignmentWriter();
	void write(JointAlignmentSet* toStore);
	void close();
	
	/**Bulk convert the common data.*/
	ParallelForLoop* doIConv;
	/**Bulk convert base entry data.*/
	ParallelForLoop* doEConv;
	/**The threads to use.*/
	ThreadPool* usePool;
	/**Save index information.*/
	StructVector<char> saveInd;
	/**Temporary storage for data.*/
	StructVector<char> tmpText;
	
	/**The index of the stuff.*/
	OutStream* fInd;
	/**The alignment data.*/
	OutStream* fEnt;
	
	/**Byte count.*/
	uintmax_t numEnt;
};

//TODO

}

#endif


