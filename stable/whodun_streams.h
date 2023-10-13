#ifndef WHODUN_STREAMS_H
#define WHODUN_STREAMS_H 1

/**
 * @file
 * @brief Extra io (pseudo)streams.
 */

#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>

#include "whodun_oshook.h"
#include "whodun_string.h"

namespace whodun {

/**Go nowhere.*/
class NullInStream : public RandaccInStream{
public:
	/**Set up.*/
	NullInStream();
	/**Tear down.*/
	~NullInStream();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
};

/**Go nowhere.*/
class NullOutStream : public OutStream{
public:
	/**Set up.*/
	NullOutStream();
	/**Tear down.*/
	~NullOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
};

/**Wrap memory as a stream.*/
class MemoryInStream : public RandaccInStream{
public:
	/**
	 * Basic setup
	 * @param numBytes The number of bytes.
	 * @param theBytes The bytes.
	 */
	MemoryInStream(uintptr_t numBytes, const char* theBytes);
	/**Clean up and close.*/
	~MemoryInStream();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	/**The number of bytes.*/
	uintptr_t numBts;
	/**The bytes in question.*/
	const char* theBts;
	/**The next byte.*/
	uintptr_t nextBt;
};

/**Store writes.*/
class MemoryOutStream : public OutStream{
	/**Set up.*/
	MemoryOutStream();
	/**Tear down.*/
	~MemoryOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
	/**The place they are stored.*/
	std::vector<char> saveArea;
};

//TODO Buffered

};

#endif

