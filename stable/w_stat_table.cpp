#include "whodun_stat_table.h"

#include <algorithm>

#include "whodun_compress.h"

namespace whodun{

/**Do stuff for reading.*/
class DelimitedTableReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	DelimitedTableReadTask();
	/**Clean up.*/
	~DelimitedTableReadTask();
	void doTask();
	
	/**The reader this is for.*/
	DelimitedTableReader* forRead;
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - cut up the stupid things
	/**The index of the first row this should work over.*/
	uintptr_t firstRI;
	/**The index after the last row this should work over.*/
	uintptr_t endRI;
	/**The number of columns in each row.*/
	std::vector<uintptr_t> numColsEachRow;
	/**Store the columns for this run.*/
	std::vector<SizePtrString> colCellTexts;
	/**Save row splits.*/
	StructVector<Token> saveColS;
	
	//phase 2 - pack them into the final target
	/**The table to store in*/
	TextTable* toStore;
	/**The column offset.*/
	uintptr_t colOffset;
};

/**Do stuff for writing.*/
class DelimitedTableWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	DelimitedTableWriteTask();
	/**Clean up.*/
	~DelimitedTableWriteTask();
	void doTask();
	
	/**The table to store in*/
	TextTableView* toStore;
	/**The row delimiter.*/
	char rowDelim;
	/**The column delimiter.*/
	char colDelim;
	/**The index of the first row this should work over.*/
	uintptr_t firstRI;
	/**The index after the last row this should work over.*/
	uintptr_t endRI;
	/**The phase this is running.*/
	uintptr_t phase;
	
	//phase 1 - figure out how much crap there is
	/**The total packed size of all the stuff*/
	uintptr_t totalPackS;
	
	//phase 2 - pack the crap
	/**The place to put it.*/
	char* packTarget;
};

/**Unpack read data.*/
class ChunkyTableReadTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	ChunkyTableReadTask();
	/**Clean up.*/
	~ChunkyTableReadTask();
	void doTask();
	
	/**The phase to run*/
	uintptr_t phase;
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	
	//phase 1 - figure out the total number of columns
	/**The full set of annotation data.*/
	char* annotData;
	/**The full set of packed text data.*/
	SizePtrString textData;
	/**The total number of columns in this chunk.*/
	uintptr_t totalColumns;
	
	//phase 2 - packing the data
	/**The table to fill.*/
	TextTable* toStore;
	/**The offset to the first column*/
	uintptr_t columnOffset;
};

/**Pack data to write.*/
class ChunkyTableWriteTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	ChunkyTableWriteTask();
	/**Clean up.*/
	~ChunkyTableWriteTask();
	void doTask();
	
	/**The phase to run*/
	uintptr_t phase;
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	/**The table this works on.*/
	TextTableView* toStore;
	
	//phase 1 - figure out how many bytes this eats
	/**The number of bytes this chunk will use.*/
	uintptr_t numEatBytes;
	
	//phase 2 - pack it up
	/**The file offset this chunk starts at.*/
	uintmax_t startByteI;
	/**Storage for annotation data for this piece.*/
	char* annotData;
	/**The table data to pack this piece into.*/
	char* tableData;
};

/**Run a filter.*/
class TextTableFilterTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	TextTableFilterTask();
	/**Clean up.*/
	~TextTableFilterTask();
	void doTask();
	
	/**The phase to run*/
	uintptr_t phase;
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	/**The filter this is for.*/
	TextTableFilter* useFilt;
	
	//phase 1 - see who lives
	/**The table this works on.*/
	TextTableView* toMang;
	/**Save the surviving rows.*/
	StructVector<TextTableRow> saveWinners;
	
	//phase 2 - make em live
	/**The offset to write to.*/
	uintptr_t baseOffset;
	/**The table this writes to.*/
	TextTableView* toSave;
};

/**Run a mutation.*/
class TextTableMutateTask : public JoinableThreadTask{
public:
	/**setup an empty*/
	TextTableMutateTask();
	/**Clean up.*/
	~TextTableMutateTask();
	void doTask();
	
	/**The phase to run*/
	uintptr_t phase;
	/**The row this starts at.*/
	uintptr_t fromRI;
	/**The row this ends at.*/
	uintptr_t toRI;
	/**The filter this is for.*/
	TextTableMutate* useFilt;
	
	//phase 1 - figure the size of the mutation
	/**The table this works on.*/
	TextTableView* toMang;
	/**Number of columns needed.*/
	uintptr_t numNeedCols;
	/**Number of bytes needed.*/
	uintptr_t numNeedByte;
	
	//phase 2 - pack it
	/**The place to write to.*/
	TextTable* toSave;
	/**The offset into the string array.*/
	uintptr_t strOffset;
	/**The offset into the text array.*/
	uintptr_t byteOffset;
};

/**Trim and handle escapes.*/
class TSVTableMutator : public TextTableMutate{
public:
	/**
	 * Set up a single-threaded filter.
	 * @param isPack Whether this is packing for output (rather than unpacking during read).
	 * @param wantEscape Whether this will be performing escapes.
	 */
	TSVTableMutator(int isPack, int wantEscape);
	/**
	 * Set up a multi-threaded mutation.
	 * @param isPack Whether this is packing for output (rather than unpacking during read).
	 * @param wantEscape Whether this will be performing escapes.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableMutator(int isPack, int wantEscape, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableMutator();
	
	std::pair<uintptr_t,uintptr_t> neededSpace(TextTableRow* row);
	void mutate(TextTableRow* rowS, TextTableRow* rowD, SizePtrString* nextCol, char* nextByte);
	
	/**The mode this is mutating for.*/
	int mutMode;
};

/**Filter out empty rows.*/
class TSVTableFilter : public TextTableFilter{
public:
	/**
	 * Set up a single-threaded filter.
	 */
	TSVTableFilter();
	/**
	 * Set up a multi-threaded mutation.
	 * @param numThread The number of tasks to spawn.
	 * @param mainPool The threads to use.
	 */
	TSVTableFilter(uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~TSVTableFilter();
	
	int test(TextTableRow* row);
};

};

using namespace whodun;

TextTableView::TextTableView(){}
TextTableView::~TextTableView(){}

TextTable::TextTable(){}
TextTable::~TextTable(){}

TextTableFilter::TextTableFilter(){
	usePool = 0;
	{
		passUnis.push_back(new TextTableFilterTask());
	}
}
TextTableFilter::TextTableFilter(uintptr_t numThread, ThreadPool* mainPool){
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new TextTableFilterTask());
	}
}
TextTableFilter::~TextTableFilter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void TextTableFilter::filter(TextTableView* toMang, TextTableView* toSave){
	uintptr_t numThread = passUnis.size();
	uintptr_t numRow = toMang->saveRows.size();
	uintptr_t numPT = numRow / numThread;
	uintptr_t numET = numRow % numThread;
	//figure out who lives
		uintptr_t curNum = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			TextTableFilterTask* curT = (TextTableFilterTask*)(passUnis[i]);
			curT->phase = 1;
			curT->fromRI = curNum;
			curNum += (numPT + (i<numET));
			curT->toRI = curNum;
			curT->useFilt = this;
			curT->toMang = toMang;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//pack the winners
		uintptr_t totNumWin = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			TextTableFilterTask* curT = (TextTableFilterTask*)(passUnis[i]);
			curT->phase = 2;
			curT->baseOffset = totNumWin;
			curT->toSave = toSave;
			totNumWin += curT->saveWinners.size();
		}
		toSave->saveRows.resize(totNumWin);
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
}

TextTableFilterTask::TextTableFilterTask(){}
TextTableFilterTask::~TextTableFilterTask(){}
void TextTableFilterTask::doTask(){
	if(phase == 1){
		saveWinners.clear();
		TextTableRow* row = toMang->saveRows[fromRI];
		for(uintptr_t i = fromRI; i<toRI; i++){
			if(useFilt->test(row)){
				saveWinners.push_back(row);
			}
			row++;
		}
	}
	else{
		memcpy(toSave->saveRows[baseOffset], saveWinners[0], saveWinners.size() * sizeof(TextTableRow));
	}
}

TextTableMutate::TextTableMutate(){
	usePool = 0;
	{
		passUnis.push_back(new TextTableMutateTask());
	}
}
TextTableMutate::TextTableMutate(uintptr_t numThread, ThreadPool* mainPool){
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new TextTableMutateTask());
	}
}
TextTableMutate::~TextTableMutate(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void TextTableMutate::mutate(TextTableView* toMang, TextTable* toSave){
	uintptr_t numThread = passUnis.size();
	uintptr_t numRow = toMang->saveRows.size();
	uintptr_t numPT = numRow / numThread;
	uintptr_t numET = numRow % numThread;
	//figure out how big things are
		uintptr_t curNum = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			TextTableMutateTask* curT = (TextTableMutateTask*)(passUnis[i]);
			curT->phase = 1;
			curT->fromRI = curNum;
			curNum += (numPT + (i<numET));
			curT->toRI = curNum;
			curT->useFilt = this;
			curT->toMang = toMang;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//pack the winners
		uintptr_t totNumWinC = 0;
		uintptr_t totNumWinB = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			TextTableMutateTask* curT = (TextTableMutateTask*)(passUnis[i]);
			curT->phase = 2;
			curT->strOffset = totNumWinC;
			curT->byteOffset = totNumWinB;
			curT->toSave = toSave;
			totNumWinC += curT->numNeedCols;
			totNumWinB += curT->numNeedByte;
		}
		toSave->saveRows.clear();
		toSave->saveRows.resize(toMang->saveRows.size());
		toSave->saveStrs.clear();
		toSave->saveStrs.resize(totNumWinC);
		toSave->saveText.clear();
		toSave->saveText.resize(totNumWinB);
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
}

TextTableMutateTask::TextTableMutateTask(){}
TextTableMutateTask::~TextTableMutateTask(){}
void TextTableMutateTask::doTask(){
	if(phase == 1){
		numNeedCols = 0;
		numNeedByte = 0;
		TextTableRow* row = toMang->saveRows[fromRI];
		for(uintptr_t i = fromRI; i<toRI; i++){
			std::pair<uintptr_t,uintptr_t> rowN = useFilt->neededSpace(row);
			numNeedCols += rowN.first;
			numNeedByte += rowN.second;
			row++;
		}
	}
	else{
		TextTableRow* row = toMang->saveRows[fromRI];
		TextTableRow* dstR = toSave->saveRows[fromRI];
		SizePtrString* dstC = toSave->saveStrs[strOffset];
		char* dstB = toSave->saveText[byteOffset];
		for(uintptr_t i = fromRI; i<toRI; i++){
			std::pair<uintptr_t,uintptr_t> rowN = useFilt->neededSpace(row);
			useFilt->mutate(row, dstR, dstC, dstB);
			row++;
			dstR++;
			dstC += rowN.first;
			dstB += rowN.second;
		}
	}
}

TextTableReader::TextTableReader(){
	isClosed = 0;
}
TextTableReader::~TextTableReader(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

void RandacTextTableReader::readRange(TextTable* toStore, uintmax_t fromIndex, uintmax_t toIndex){
	uintmax_t numR = toIndex - fromIndex;
	seek(fromIndex);
	read(toStore, numR);
}

TextTableWriter::TextTableWriter(){
	isClosed = 0;
}
TextTableWriter::~TextTableWriter(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}

DelimitedTableReader::DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom){
	theStr = mainFrom;
	rowSplitter = new CharacterSplitTokenizer(rowDelim);
	colSplitter = new CharacterSplitTokenizer(colDelim);
	charMove = new StandardMemoryShuttler();
	haveDrained = 0;
	usePool = 0;
	{
		DelimitedTableReadTask* curT = new DelimitedTableReadTask();
		curT->forRead = this;
		passUnis.push_back(curT);
	}
}
DelimitedTableReader::DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	rowSplitter = new MultithreadedCharacterSplitTokenizer(rowDelim, numThread, mainPool);
	colSplitter = new CharacterSplitTokenizer(colDelim);
	charMove = new ThreadedMemoryShuttler(numThread, mainPool);
	haveDrained = 0;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		DelimitedTableReadTask* curT = new DelimitedTableReadTask();
		curT->forRead = this;
		passUnis.push_back(curT);
	}
}
DelimitedTableReader::~DelimitedTableReader(){
	delete(rowSplitter);
	delete(colSplitter);
	delete(charMove);
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t DelimitedTableReader::read(TextTable* toStore, uintptr_t numRows){
	if(haveDrained){ return 0; }
	if(numRows == 0){ return 0; }
	uintptr_t typeDelim = 0;
	uintptr_t typeText = 1;
	//initialize the text
		toStore->saveText.clear();
		if(saveTexts.size()){
			toStore->saveText.resize(saveTexts.size());
			charMove->memcpy(toStore->saveText[0], saveTexts[0], saveTexts.size());
			saveTexts.clear();
		}
	//cut up into rows (keep reading more until enough had)
		SizePtrString remText;
		int haveHitEOF = 0;
		while(1){
			//find the tokens
				saveRowS.clear();
				remText = rowSplitter->tokenize(toSizePtr(&(toStore->saveText)), &saveRowS);
			//if have hit eof, add some tokens
				if(haveHitEOF){
					Token pushT;
					pushT.text = remText;
						pushT.numTypes = 1;
						pushT.types = &typeText;
						saveRowS.push_back(&pushT);
					pushT.text.len = 0;
						pushT.types = &typeDelim;
						saveRowS.push_back(&pushT);
				}
			//figure out if more neads to be read
				if(haveHitEOF || (saveRowS.size() >= 2*numRows)){
					break;
				}
			//load more and try again
				uintptr_t origTS = toStore->saveText.size();
				uintptr_t newTS = std::max((uintptr_t)80, 2*origTS);
				toStore->saveText.resize(newTS);
				uintptr_t numWantR = newTS - origTS;
				uintptr_t numGotR = theStr->read(toStore->saveText[origTS], numWantR);
				if(numGotR != numWantR){
					toStore->saveText.resize(origTS + numGotR);
					haveHitEOF = 1;
				}
		}
	//back off if too many
		uintptr_t numLines = saveRowS.size() / 2;
		if(numLines > numRows){
			char* remTerm = remText.txt + remText.len;
			char* newRemS = saveRowS[2*numRows]->text.txt;
			remText.txt = newRemS;
			remText.len = remTerm - newRemS;
			numLines = numRows;
		}
		else if(numLines < numRows){
			haveDrained = 1;
		}
	//cut lines into cells
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numLines / numThread;
		uintptr_t numET = numLines % numThread;
		uintptr_t curLineI = 0;
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			DelimitedTableReadTask* curT = (DelimitedTableReadTask*)passUnis[i];
			curT->phase = 1;
			curT->firstRI = curLineI;
			uintptr_t curNum = numPT + (i<numET);
			curLineI += curNum;
			curT->endRI = curLineI;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//merge everything together
		uintptr_t totNumCols = 0;
		for(uintptr_t i = 0; i<passUnis.size(); i++){
			DelimitedTableReadTask* curT = (DelimitedTableReadTask*)passUnis[i];
			curT->phase = 2;
			curT->toStore = toStore;
			curT->colOffset = totNumCols;
			totNumCols += curT->colCellTexts.size();
		}
		toStore->saveRows.resize(numLines);
		toStore->saveStrs.resize(totNumCols);
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//store any overhang for later
		saveTexts.resize(remText.len);
		charMove->memcpy(saveTexts[0], remText.txt, remText.len);
	return numLines;
}
void DelimitedTableReader::close(){
	isClosed = 1;
}

DelimitedTableReadTask::DelimitedTableReadTask(){}
DelimitedTableReadTask::~DelimitedTableReadTask(){}
void DelimitedTableReadTask::doTask(){
	if(phase == 1){
		//cut up each line into columns
		numColsEachRow.clear();
		colCellTexts.clear();
		for(uintptr_t i = firstRI; i<endRI; i++){
			saveColS.clear();
			SizePtrString curLine = forRead->saveRowS[2*i]->text;
			SizePtrString remText = forRead->colSplitter->tokenize(curLine, &saveColS);
			uintptr_t numCol = saveColS.size() / 2;
			for(uintptr_t j = 0; j<numCol; j++){
				colCellTexts.push_back(saveColS[2*j]->text);
			}
			colCellTexts.push_back(remText);
			numColsEachRow.push_back(numCol+1);
		}
	}
	else{
		TextTableRow* curRow = toStore->saveRows[firstRI];
		SizePtrString* curCol = toStore->saveStrs[colOffset];
		uintptr_t curStrI = 0;
		for(uintptr_t i = 0; i<numColsEachRow.size(); i++){
			uintptr_t curNumC = numColsEachRow[i];
			curRow->numCols = curNumC;
			curRow->texts = curCol;
			if(curNumC){
				memcpy(curCol, &(colCellTexts[curStrI]), curNumC*sizeof(SizePtrString));
			}
			curStrI += curNumC;
			curCol += curNumC;
			curRow++;
		}
	}
}

DelimitedTableWriter::DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom){
	theStr = mainFrom;
	usePool = 0;
	{
		DelimitedTableWriteTask* curT = new DelimitedTableWriteTask();
		curT->rowDelim = rowDelim;
		curT->colDelim = colDelim;
		passUnis.push_back(curT);
	}
}
DelimitedTableWriter::DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom, uintptr_t numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	usePool = mainPool;
	for(uintptr_t i = 0; i<numThread; i++){
		DelimitedTableWriteTask* curT = new DelimitedTableWriteTask();
		curT->rowDelim = rowDelim;
		curT->colDelim = colDelim;
		passUnis.push_back(curT);
	}
}
DelimitedTableWriter::~DelimitedTableWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void DelimitedTableWriter::write(TextTableView* toStore){
	//figure out the size of everything
		uintptr_t numLines = toStore->saveRows.size();
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numLines / numThread;
		uintptr_t numET = numLines % numThread;
		uintptr_t curL = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			DelimitedTableWriteTask* curT = (DelimitedTableWriteTask*)(passUnis[i]);
			curT->toStore = toStore;
			curT->firstRI = curL;
			curL += (numPT + (i<numET));
			curT->endRI = curL;
			curT->phase = 1;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
		uintptr_t totalPSize = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			DelimitedTableWriteTask* curT = (DelimitedTableWriteTask*)(passUnis[i]);
			totalPSize += curT->totalPackS;
		}
	//pack it all up
		saveTexts.resize(totalPSize);
		char* curP = saveTexts[0];
		for(uintptr_t i = 0; i<numThread; i++){
			DelimitedTableWriteTask* curT = (DelimitedTableWriteTask*)(passUnis[i]);
			curT->phase = 2;
			curT->packTarget = curP;
			curP += curT->totalPackS;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//and dump
		theStr->write(saveTexts[0], totalPSize);
}
void DelimitedTableWriter::close(){
	isClosed = 1;
}

DelimitedTableWriteTask::DelimitedTableWriteTask(){}
DelimitedTableWriteTask::~DelimitedTableWriteTask(){}
void DelimitedTableWriteTask::doTask(){
	if(phase == 1){
		totalPackS = 0;
		for(uintptr_t i = firstRI; i<endRI; i++){
			TextTableRow* curRow = toStore->saveRows[i];
			for(uintptr_t j = 0; j<curRow->numCols; j++){
				if(j){ totalPackS++; }
				totalPackS += curRow->texts[j].len;
			}
			totalPackS++;
		}
	}
	else{
		char* curPT = packTarget;
		for(uintptr_t i = firstRI; i<endRI; i++){
			TextTableRow* curRow = toStore->saveRows[i];
			for(uintptr_t j = 0; j<curRow->numCols; j++){
				if(j){ *curPT = colDelim; curPT++; }
				SizePtrString curColT = curRow->texts[j];
				memcpy(curPT, curColT.txt, curColT.len);
				curPT += curColT.len;
			}
			*curPT = rowDelim; curPT++;
		}
	}
}

TSVTableReader::TSVTableReader(InStream* mainFrom, int useEscapes){
	baseRead = new DelimitedTableReader('\n', '\t', mainFrom);
	trimEscape = new TSVTableMutator(0, useEscapes ? 1 : 0);
	killEmpties = new TSVTableFilter();
}
TSVTableReader::TSVTableReader(InStream* mainFrom, int useEscapes, uintptr_t numThread, ThreadPool* mainPool){
	baseRead = new DelimitedTableReader('\n', '\t', mainFrom, numThread, mainPool);
	trimEscape = new TSVTableMutator(0, useEscapes ? 1 : 0, numThread, mainPool);
	killEmpties = new TSVTableFilter(numThread, mainPool);
}
TSVTableReader::~TSVTableReader(){
	delete(killEmpties);
	delete(trimEscape);
	delete(baseRead);
}
uintptr_t TSVTableReader::read(TextTable* toStore, uintptr_t numRows){
	uintptr_t numReadB = baseRead->read(&baseStage, numRows);
	trimEscape->mutate(&baseStage, toStore);
	killEmpties->filter(toStore, toStore);
	return numReadB;
}
void TSVTableReader::close(){
	isClosed = 1;
	baseRead->close();
}

TSVTableWriter::TSVTableWriter(OutStream* mainFrom, int useEscapes){
	baseRead = new DelimitedTableWriter('\n', '\t', mainFrom);
	trimEscape = 0;
	if(useEscapes){
		trimEscape = new TSVTableMutator(1,1);
	}
}
TSVTableWriter::TSVTableWriter(OutStream* mainFrom, int useEscapes, uintptr_t numThread, ThreadPool* mainPool){
	baseRead = new DelimitedTableWriter('\n', '\t', mainFrom, numThread, mainPool);
	trimEscape = 0;
	if(useEscapes){
		trimEscape = new TSVTableMutator(1,1, numThread, mainPool);
	}
}
TSVTableWriter::~TSVTableWriter(){
	if(trimEscape){ delete(trimEscape); }
	delete(baseRead);
}
void TSVTableWriter::write(TextTableView* toStore){
	TextTableView* dumpSrc = toStore;
	if(trimEscape){
		trimEscape->mutate(dumpSrc, &baseStage);
		dumpSrc = &baseStage;
	}
	baseRead->write(dumpSrc);
}
void TSVTableWriter::close(){
	isClosed = 1;
	baseRead->close();
}

TSVTableMutator::TSVTableMutator(int isPack, int wantEscape) : TextTableMutate(){
	mutMode = (isPack << 1) | wantEscape;
}
TSVTableMutator::TSVTableMutator(int isPack, int wantEscape, uintptr_t numThread, ThreadPool* mainPool) : TextTableMutate(numThread, mainPool){
	mutMode = (isPack << 1) | wantEscape;
}
TSVTableMutator::~TSVTableMutator(){}
std::pair<uintptr_t,uintptr_t> TSVTableMutator::neededSpace(TextTableRow* row){
	uintptr_t numCol = row->numCols;
	uintptr_t numByte = 0;
	for(uintptr_t i = 0; i<numCol; i++){
		numByte += row->texts[i].len;
	}
	if(mutMode == 0x03){ /*are packing, and escaping*/
		numByte = 2*numByte;
	}
	return std::pair<uintptr_t,uintptr_t>(numCol,numByte);
}
void TSVTableMutator::mutate(TextTableRow* rowS, TextTableRow* rowD, SizePtrString* nextCol, char* nextByte){
	StandardMemorySearcher cutUpS;
	uintptr_t numCol = rowS->numCols;
	rowD->numCols = numCol;
	rowD->texts = nextCol;
	char* nextT = nextByte;
	switch(mutMode){
		case 0:{ /*trim only*/
			for(uintptr_t i = 0; i<numCol; i++){
				SizePtrString curEnt = cutUpS.trim(rowS->texts[i]);
				nextCol[i].txt = nextT;
				nextCol[i].len = curEnt.len;
				memcpy(nextT, curEnt.txt, curEnt.len);
				nextT += curEnt.len;
			}
		} break;
		case 1:{ /*trim and fix up escapes*/
			for(uintptr_t i = 0; i<numCol; i++){
				SizePtrString curEnt = cutUpS.trim(rowS->texts[i]);
				nextCol[i].txt = nextT;
				uintptr_t finLen = 0;
				uintptr_t j = 0;
				while(j<curEnt.len){
					if(curEnt.txt[j] == '\\'){
						j++;
						char winC;
						if(j>curEnt.len){ winC = ' '; }
						else{
							switch(curEnt.txt[j]){
								case 'n': winC = '\n'; break;
								case 'r': winC = '\r'; break;
								case 't': winC = '\t'; break;
								default: winC = curEnt.txt[j];
							}
						}
						*nextT = winC;
					}
					else{
						*nextT = curEnt.txt[j];
					}
					j++;
					nextT++;
					finLen++;
				}
				nextCol[i].len = finLen;
			}
		} break;
		case 2:{ /*simple copy*/
			for(uintptr_t i = 0; i<numCol; i++){
				SizePtrString curEnt = rowS->texts[i];
				nextCol[i].txt = nextT;
				nextCol[i].len = curEnt.len;
				memcpy(nextT, curEnt.txt, curEnt.len);
				nextT += curEnt.len;
			}
		} break;
		case 3:{ /*escape weirdness*/
			for(uintptr_t i = 0; i<numCol; i++){
				SizePtrString curEnt = rowS->texts[i];
				char* startT = nextT;
				nextCol[i].txt = nextT;
				//handle any whitespace at the front
					uintptr_t j = 0;
					while(j<curEnt.len){
						switch(curEnt.txt[j]){
							case ' ':
								*nextT = '\\'; nextT++; *nextT = ' '; nextT++; break;
							case '\t':
								*nextT = '\\'; nextT++; *nextT = 't'; nextT++; break;
							case '\r':
								*nextT = '\\'; nextT++; *nextT = 'r'; nextT++; break;
							case '\n':
								*nextT = '\\'; nextT++; *nextT = 'n'; nextT++; break;
							default:
								goto front_no_ws;
						}
						j++;
					}
					front_no_ws:
				//figure out where the ending whitespace ends
					uintptr_t k = curEnt.len;
					while(k > j){
						char curC = curEnt.txt[k-1];
						if((curC == ' ') || (curC == '\t') || (curC == '\r') || (curC == '\n')){ k--; }
						else{ break; }
					}
				//escape the text in the middle
					while(j < k){
						switch(curEnt.txt[j]){
							case '\t':
								*nextT = '\\'; nextT++; *nextT = 't'; nextT++; break;
							case '\r':
								*nextT = '\\'; nextT++; *nextT = 'r'; nextT++; break;
							case '\n':
								*nextT = '\\'; nextT++; *nextT = 'n'; nextT++; break;
							default:
								*nextT = curEnt.txt[j]; nextT++;
						}
						j++;
					}
				//escape text at the end
					while(j<curEnt.len){
						switch(curEnt.txt[j]){
							case ' ':
								*nextT = '\\'; nextT++; *nextT = ' '; nextT++; break;
							case '\t':
								*nextT = '\\'; nextT++; *nextT = 't'; nextT++; break;
							case '\r':
								*nextT = '\\'; nextT++; *nextT = 'r'; nextT++; break;
							case '\n':
								*nextT = '\\'; nextT++; *nextT = 'n'; nextT++; break;
							default:
								throw std::runtime_error("Da fuq?");
						}
						j++;
					}
				nextCol[i].len = nextT - startT;
			}
		} break;
	};
}

TSVTableFilter::TSVTableFilter() : TextTableFilter(){}
TSVTableFilter::TSVTableFilter(uintptr_t numThread, ThreadPool* mainPool) : TextTableFilter(numThread,mainPool){}
TSVTableFilter::~TSVTableFilter(){}
int TSVTableFilter::test(TextTableRow* row){
	if(row->numCols > 1){ return 1; }
	if(row->numCols == 0){ return 0; }
	return row->texts->len > 0;
}

//annotation for block-compressed table is just the address of the entry
//entries are stored N_cell {len text[]}[]
#define BLOCKCOMPTAB_ANNOT_ENTLEN 8

ChunkyTextTableReader::ChunkyTextTableReader(RandaccInStream* annotationFile, RandaccInStream* dataFile){
	indStr = annotationFile;
	tsvStr = dataFile;
	usePool = 0;
	needSeek = 1;
	focusInd = 0;
	totalNInd = indStr->size();
		if(totalNInd % BLOCKCOMPTAB_ANNOT_ENTLEN){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not even.", 0, 0);
		}
		totalNInd = totalNInd / BLOCKCOMPTAB_ANNOT_ENTLEN;
	totalNByte = tsvStr->size();
	{
		passUnis.push_back(new ChunkyTableReadTask());
	}
}
ChunkyTextTableReader::ChunkyTextTableReader(RandaccInStream* annotationFile, RandaccInStream* dataFile, uintptr_t numThread, ThreadPool* mainPool){
	indStr = annotationFile;
	tsvStr = dataFile;
	usePool = 0;
	needSeek = 1;
	focusInd = 0;
	totalNInd = indStr->size();
		if(totalNInd % BLOCKCOMPTAB_ANNOT_ENTLEN){
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not even.", 0, 0);
		}
		totalNInd = totalNInd / BLOCKCOMPTAB_ANNOT_ENTLEN;
	totalNByte = tsvStr->size();
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new ChunkyTableReadTask());
	}
}
ChunkyTextTableReader::~ChunkyTextTableReader(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
uintptr_t ChunkyTextTableReader::read(TextTable* toStore, uintptr_t numRows){
	if(numRows == 0){ return 0; }
	//seek if necessary
		if(needSeek){
			if(focusInd < totalNInd){
				indStr->seek(BLOCKCOMPTAB_ANNOT_ENTLEN*focusInd);
				saveARB.resize(BLOCKCOMPTAB_ANNOT_ENTLEN);
				indStr->forceRead(saveARB[0], BLOCKCOMPTAB_ANNOT_ENTLEN);
				ByteUnpacker getOffV(saveARB[0]);
				tsvStr->seek(getOffV.unpackBE64());
			}
			needSeek = 0;
		}
	//figure out how many rows will actually be read
		uintmax_t endFocInd = focusInd + numRows;
			endFocInd = std::min(endFocInd, totalNInd);
		uintptr_t numRealRead = endFocInd - focusInd;
		int willHitEOF = endFocInd == totalNInd;
		if(numRealRead == 0){ return 0; }
	//load the annotation
		saveARB.resize(BLOCKCOMPTAB_ANNOT_ENTLEN*(numRealRead + 1));
		indStr->forceRead(saveARB[BLOCKCOMPTAB_ANNOT_ENTLEN], BLOCKCOMPTAB_ANNOT_ENTLEN*(numRealRead - willHitEOF));
		if(willHitEOF){
			BytePacker packOffV(saveARB[BLOCKCOMPTAB_ANNOT_ENTLEN*numRealRead]);
			packOffV.packBE64(totalNByte);
		}
	//load the text
		uintmax_t startTAddr;
		uintmax_t endTAddr;
		{
			ByteUnpacker getOffV(saveARB[0]);
			startTAddr = getOffV.unpackBE64();
			getOffV.retarget(saveARB[BLOCKCOMPTAB_ANNOT_ENTLEN*numRealRead]);
			endTAddr = getOffV.unpackBE64();
		}
		toStore->saveText.clear();
		toStore->saveText.resize(endTAddr - startTAddr);
		tsvStr->forceRead(toStore->saveText[0], endTAddr - startTAddr);
	//figure out how many columns there are
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numRealRead / numThread;
		uintptr_t numET = numRealRead % numThread;
		uintptr_t curRN = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkyTableReadTask* curT = (ChunkyTableReadTask*)(passUnis[i]);
			curT->phase = 1;
			curT->fromRI = curRN;
			curRN += (numPT + (i<numET));
			curT->toRI = curRN;
			curT->annotData = saveARB[0];
			curT->textData = toSizePtr(&(toStore->saveText));
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//let everything pack it up
		uintptr_t totalNC = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkyTableReadTask* curT = (ChunkyTableReadTask*)(passUnis[i]);
			curT->phase = 2;
			curT->toStore = toStore;
			curT->columnOffset = totalNC;
			totalNC += curT->totalColumns;
		}
		toStore->saveStrs.clear();
		toStore->saveRows.clear();
		toStore->saveStrs.resize(totalNC);
		toStore->saveRows.resize(numRealRead);
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//and prepare for the next round
		memcpy(saveARB[0], saveARB[BLOCKCOMPTAB_ANNOT_ENTLEN*numRealRead], BLOCKCOMPTAB_ANNOT_ENTLEN);
		focusInd = endFocInd;
	return numRealRead;
}
void ChunkyTextTableReader::close(){
	isClosed = 1;
}
uintmax_t ChunkyTextTableReader::size(){
	return totalNInd;
}
void ChunkyTextTableReader::seek(uintmax_t index){
	if(index != focusInd){
		focusInd = index;
		needSeek = 1;
	}
}

ChunkyTableReadTask::ChunkyTableReadTask(){}
ChunkyTableReadTask::~ChunkyTableReadTask(){}
void ChunkyTableReadTask::doTask(){
	if(fromRI == toRI){ return; }
	if(phase == 1){
		totalColumns = 0;
		ByteUnpacker getOffV(annotData);
		uintmax_t baseAddr = getOffV.unpackBE64();
		getOffV.retarget(annotData + BLOCKCOMPTAB_ANNOT_ENTLEN*fromRI);
		uintmax_t curAddr = getOffV.unpackBE64();
		if(curAddr < baseAddr){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not monotonic.", 0, 0); }
		for(uintptr_t i = fromRI; i<toRI; i++){
			uintmax_t nextAddr = getOffV.unpackBE64();
			if(nextAddr < curAddr){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file not monotonic.", 0, 0); }
			if((nextAddr - curAddr) < 8){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file entry too short.", 0, 0); }
			if((nextAddr - baseAddr) > textData.len){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file out of order.", 0, 0); }
			ByteUnpacker getCCnt(textData.txt + (curAddr - baseAddr));
			totalColumns += getCCnt.unpackBE64();
			curAddr = nextAddr;
		}
	}
	else{
		TextTableRow* fillRow = toStore->saveRows[fromRI];
		SizePtrString* fillCol = toStore->saveStrs[columnOffset];
		ByteUnpacker getOffV(annotData);
		uintmax_t baseAddr = getOffV.unpackBE64();
		getOffV.retarget(annotData + BLOCKCOMPTAB_ANNOT_ENTLEN*fromRI);
		uintmax_t curAddr = getOffV.unpackBE64();
		for(uintptr_t i = fromRI; i<toRI; i++){
			uintmax_t nextAddr = getOffV.unpackBE64();
			uintptr_t totalRowS = nextAddr - curAddr;
			uintptr_t totalEatS = 0;
			ByteUnpacker getCCnt(textData.txt + (curAddr - baseAddr));
			uintptr_t curNumCs = getCCnt.unpackBE64(); totalEatS += 8;
			fillRow->numCols = curNumCs;
			fillRow->texts = fillCol;
			for(uintptr_t j = 0; j<curNumCs; j++){
				totalEatS += 8;
				if(totalEatS > totalRowS){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file entry too short.", 0, 0); }
				uintptr_t curStrLen = getCCnt.unpackBE64();
				fillCol->len = curStrLen;
				fillCol->txt = getCCnt.target;
				totalEatS += curStrLen;
				if(totalEatS > totalRowS){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_FILEMANG, __FILE__, __LINE__, "Index file text too short.", 0, 0); }
				getCCnt.skip(curStrLen);
				fillCol++;
			}
			curAddr = nextAddr;
			fillRow++;
		}
	}
}


ChunkyTextTableWriter::ChunkyTextTableWriter(OutStream* annotationFile, OutStream* dataFile){
	indStr = annotationFile;
	tsvStr = dataFile;
	usePool = 0;
	totalOutData = 0;
	{
		passUnis.push_back(new ChunkyTableWriteTask());
	}
}
ChunkyTextTableWriter::ChunkyTextTableWriter(OutStream* annotationFile, OutStream* dataFile, uintptr_t numThread, ThreadPool* mainPool){
	indStr = annotationFile;
	tsvStr = dataFile;
	usePool = mainPool;
	totalOutData = 0;
	for(uintptr_t i = 0; i<numThread; i++){
		passUnis.push_back(new ChunkyTableWriteTask());
	}
}
ChunkyTextTableWriter::~ChunkyTextTableWriter(){
	for(uintptr_t i = 0; i<passUnis.size(); i++){
		delete(passUnis[i]);
	}
}
void ChunkyTextTableWriter::write(TextTableView* toStore){
	uintptr_t numRows = toStore->saveRows.size();
	if(numRows == 0){ return; }
	//figure out how many bytes will be output
		uintptr_t numThread = passUnis.size();
		uintptr_t numPT = numRows / numThread;
		uintptr_t numET = numRows % numThread;
		uintptr_t curRN = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkyTableWriteTask* curT = (ChunkyTableWriteTask*)(passUnis[i]);
			curT->phase = 1;
			curT->fromRI = curRN;
			curRN += (numPT + (i<numET));
			curT->toRI = curRN;
			curT->toStore = toStore;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
		uintptr_t totalNB = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkyTableWriteTask* curT = (ChunkyTableWriteTask*)(passUnis[i]);
			totalNB += curT->numEatBytes;
		}
	//pack for the output
		packARB.resize(BLOCKCOMPTAB_ANNOT_ENTLEN*numRows);
		packDatums.resize(totalNB);
		uintmax_t curByteOff = totalOutData;
		char* curARB = packARB[0];
		char* curDatum = packDatums[0];
		for(uintptr_t i = 0; i<numThread; i++){
			ChunkyTableWriteTask* curT = (ChunkyTableWriteTask*)(passUnis[i]);
			curT->phase = 2;
			curT->startByteI = curByteOff;
			curT->annotData = curARB;
			curT->tableData = curDatum;
			curByteOff += curT->numEatBytes;
			curARB += (BLOCKCOMPTAB_ANNOT_ENTLEN*(curT->toRI - curT->fromRI));
			curDatum += curT->numEatBytes;
		}
		if(usePool){
			usePool->addTasks(numThread, (JoinableThreadTask**)&(passUnis[0]));
			joinTasks(numThread, &(passUnis[0]));
		}
		else{
			passUnis[0]->doTask();
		}
	//and dump
		indStr->write(packARB[0], BLOCKCOMPTAB_ANNOT_ENTLEN*numRows);
		tsvStr->write(packDatums[0], totalNB);
}
void ChunkyTextTableWriter::close(){
	isClosed = 1;
}

ChunkyTableWriteTask::ChunkyTableWriteTask(){}
ChunkyTableWriteTask::~ChunkyTableWriteTask(){}
void ChunkyTableWriteTask::doTask(){
	if(phase == 1){
		numEatBytes = 0;
		TextTableRow* curFocR = toStore->saveRows[fromRI];
		for(uintptr_t i = fromRI; i<toRI; i++){
			numEatBytes += 8;
			SizePtrString* curFocC = curFocR->texts;
			for(uintptr_t j = 0; j<curFocR->numCols; j++){
				numEatBytes += 8;
				numEatBytes += curFocC[j].len;
			}
			curFocR++;
		}
	}
	else{
		BytePacker curPackA(annotData);
		uintmax_t curByteI = startByteI;
		BytePacker curPackD(tableData);
		TextTableRow* curFocR = toStore->saveRows[fromRI];
		for(uintptr_t i = fromRI; i<toRI; i++){
			curPackA.packBE64(curByteI);
			curPackD.packBE64(curFocR->numCols); curByteI += 8;
			SizePtrString* curFocC = curFocR->texts;
			for(uintptr_t j = 0; j<curFocR->numCols; j++){
				uintptr_t curLen = curFocC[j].len;
				curPackD.packBE64(curLen); curByteI += 8;
				memcpy(curPackD.target, curFocC[j].txt, curLen);
				curPackD.skip(curLen); curByteI += curLen;
			}
			curFocR++;
		}
	}
}

ExtensionTextTableReader::ExtensionTextTableReader(const char* fileName, InStream* useStdin){
	openUp(fileName, 1, 0, useStdin);
}
ExtensionTextTableReader::ExtensionTextTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	openUp(fileName, numThread, mainPool, useStdin);
}
void ExtensionTextTableReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, InStream* useStdin){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			InStream* useBase = useStdin;
			if(!useBase){
				useBase = new ConsoleInStream();
				baseStrs.push_back(useBase);
			}
			wrapStr = mainPool ? new TSVTableReader(useBase, 1, numThread, mainPool) : new TSVTableReader(useBase, 1);
			return;
		}
		//anything explicitly tsv
		{
			int isTsv = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv"));
			int isTsvGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv.gz"));
			int isTsvGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv.gzip"));
			if(isTsv || isTsvGz || isTsvGzip){
				InStream* useBase;
				if(isTsv){
					useBase = new FileInStream(fileName);
				}
				else{
					useBase = new GZipInStream(fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new TSVTableReader(useBase, 1, numThread, mainPool) : new TSVTableReader(useBase, 1);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bctab"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bctab"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bctab"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bctab"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indFileName(fileName); indFileName.append(".ind");
				std::string indBFileName(fileName); indBFileName.append(".ind.blk");
				std::string bFileName(fileName); bFileName.append(".blk");
				RandaccInStream* indexS = mainPool ? new BlockCompInStream(indFileName.c_str(), indBFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(indFileName.c_str(), indBFileName.c_str(), compMeth);
				baseStrs.push_back(indexS);
				RandaccInStream* dataS = mainPool ? new BlockCompInStream(fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new ChunkyTextTableReader(indexS, dataS, numThread, mainPool) : new ChunkyTextTableReader(indexS, dataS);
				delete(compMeth);
				return;
			}
		}
		//fallback to tsv for anything weird
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = mainPool ? new TSVTableReader(baseStrs[0], 1, numThread, mainPool) : new TSVTableReader(baseStrs[0], 1);
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionTextTableReader::~ExtensionTextTableReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionTextTableReader::read(TextTable* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionTextTableReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}

ExtensionRandacTextTableReader::ExtensionRandacTextTableReader(const char* fileName){
	openUp(fileName, 1, 0);
}
ExtensionRandacTextTableReader::ExtensionRandacTextTableReader(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	openUp(fileName, numThread, mainPool);
}
ExtensionRandacTextTableReader::~ExtensionRandacTextTableReader(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
uintptr_t ExtensionRandacTextTableReader::read(TextTable* toStore, uintptr_t numRows){
	return wrapStr->read(toStore, numRows);
}
void ExtensionRandacTextTableReader::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
uintmax_t ExtensionRandacTextTableReader::size(){
	return wrapStr->size();
}
void ExtensionRandacTextTableReader::seek(uintmax_t index){
	wrapStr->seek(index);
}
void ExtensionRandacTextTableReader::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bctab"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bctab"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bctab"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bctab"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ compMeth = new GZipCompressionFactory(); }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indFileName(fileName); indFileName.append(".ind");
				std::string indBFileName(fileName); indBFileName.append(".ind.blk");
				std::string bFileName(fileName); bFileName.append(".blk");
				RandaccInStream* indexS = mainPool ? new BlockCompInStream(indFileName.c_str(), indBFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(indFileName.c_str(), indBFileName.c_str(), compMeth);
				baseStrs.push_back(indexS);
				RandaccInStream* dataS = mainPool ? new BlockCompInStream(fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompInStream(fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new ChunkyTextTableReader(indexS, dataS, numThread, mainPool) : new ChunkyTextTableReader(indexS, dataS);
				delete(compMeth);
				return;
			}
		}
		//complain on anything weird
		{
			const char* packExt[] = {fileName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown file formath for text table.", 1, packExt);
		}
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}

#define TABLE_PREFER_CHUNK_SIZE 0x010000

ExtensionTextTableWriter::ExtensionTextTableWriter(const char* fileName, OutStream* useStdout){
	openUp(fileName, 1, 0, useStdout);
}
ExtensionTextTableWriter::ExtensionTextTableWriter(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	openUp(fileName, numThread, mainPool, useStdout);
}
ExtensionTextTableWriter::~ExtensionTextTableWriter(){
	delete(wrapStr);
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionTextTableWriter::write(TextTableView* toStore){
	wrapStr->write(toStore);
}
void ExtensionTextTableWriter::close(){
	isClosed = 1;
	wrapStr->close();
	uintptr_t i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
void ExtensionTextTableWriter::openUp(const char* fileName, uintptr_t numThread, ThreadPool* mainPool, OutStream* useStdout){
	StandardMemorySearcher strMeth;
	CompressionFactory* compMeth = 0;
	try{
		//check for stdin
		if((fileName == 0) || (strlen(fileName)==0) || (strcmp(fileName,"-")==0)){
			OutStream* useBase = useStdout;
			if(!useBase){
				useBase = new ConsoleOutStream();
				baseStrs.push_back(useBase);
			}
			wrapStr = mainPool ? new TSVTableWriter(useBase, 1, numThread, mainPool) : new TSVTableWriter(useBase, 1);
			return;
		}
		//anything explicitly tsv
		{
			int isTsv = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv"));
			int isTsvGz = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv.gz"));
			int isTsvGzip = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".tsv.gzip"));
			if(isTsv || isTsvGz || isTsvGzip){
				OutStream* useBase;
				if(isTsv){
					useBase = new FileOutStream(0,fileName);
				}
				else{
					useBase = new GZipOutStream(0,fileName);
				}
				baseStrs.push_back(useBase);
				wrapStr = mainPool ? new TSVTableWriter(useBase, 1, numThread, mainPool) : new TSVTableWriter(useBase, 1);
				return;
			}
		}
		//anything block compressed
		{
			int isBctab = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".bctab"));
			int isRaw = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".raw.bctab"));
			int isGzip = 0; //strMeth.memendswith(toSizePtr(fileName), toSizePtr(".gzip.bctab"));
			int isDeflate = strMeth.memendswith(toSizePtr(fileName), toSizePtr(".zlib.bctab"));
			if(isBctab){
				if(isRaw){ compMeth = new RawCompressionFactory(); }
				else if(isGzip){ GZipCompressionFactory* compMethG = new GZipCompressionFactory(); compMethG->addBlockComp = 1; compMeth = compMethG; }
				else if(isDeflate){ compMeth = new DeflateCompressionFactory(); }
				else{
					const char* packExt[] = {fileName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Unknown compression method for block compressed table.", 1, packExt);
				}
				std::string indFileName(fileName); indFileName.append(".ind");
				std::string indBFileName(fileName); indBFileName.append(".ind.blk");
				std::string bFileName(fileName); bFileName.append(".blk");
				OutStream* indexS = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, indFileName.c_str(), indBFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, indFileName.c_str(), indBFileName.c_str(), compMeth);
				baseStrs.push_back(indexS);
				OutStream* dataS = mainPool ? new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, fileName, bFileName.c_str(), compMeth, numThread, mainPool) : new BlockCompOutStream(0, TABLE_PREFER_CHUNK_SIZE, fileName, bFileName.c_str(), compMeth);
				baseStrs.push_back(dataS);
				wrapStr = mainPool ? new ChunkyTextTableWriter(indexS, dataS, numThread, mainPool) : new ChunkyTextTableWriter(indexS, dataS);
				delete(compMeth);
				return;
			}
		}
		//fallback to tsv for anything weird
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = mainPool ? new TSVTableWriter(baseStrs[0], 1, numThread, mainPool) : new TSVTableWriter(baseStrs[0], 1);
	}
	catch(std::exception& errE){
		isClosed = 1;
		if(compMeth){ delete(compMeth); }
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}

ArgumentOptionTextTableRead::ArgumentOptionTextTableRead(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" table.tsv");
	summary = useDesc;
	validExts.push_back(".tsv");
	validExts.push_back(".tsv.gz");
	validExts.push_back(".tsv.gzip");
	validExts.push_back(".raw.bctab");
	//validExts.push_back(".gzip.bctab");
	validExts.push_back(".zlib.bctab");
}
ArgumentOptionTextTableRead::~ArgumentOptionTextTableRead(){}

ArgumentOptionTextTableRandac::ArgumentOptionTextTableRandac(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileRead(theName){
	required = needed;
	usage = theName;
		usage.append(" table.gzip.bctab");
	summary = useDesc;
	validExts.push_back(".raw.bctab");
	//validExts.push_back(".gzip.bctab");
	validExts.push_back(".zlib.bctab");
}
ArgumentOptionTextTableRandac::~ArgumentOptionTextTableRandac(){}

ArgumentOptionTextTableWrite::ArgumentOptionTextTableWrite(int needed, const char* theName, const char* useDesc) : ArgumentOptionFileWrite(theName){
	required = needed;
	usage = theName;
		usage.append(" table.tsv");
	summary = useDesc;
	validExts.push_back(".tsv");
	validExts.push_back(".tsv.gz");
	validExts.push_back(".tsv.gzip");
	validExts.push_back(".raw.bctab");
	//validExts.push_back(".gzip.bctab");
	validExts.push_back(".zlib.bctab");
}
ArgumentOptionTextTableWrite::~ArgumentOptionTextTableWrite(){}



