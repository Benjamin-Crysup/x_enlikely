#ifndef WHODUN_STRING_H
#define WHODUN_STRING_H 1

/**
 * @file
 * @brief Organized string library.
 */

#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>

namespace whodun {

/**A Pascal flavored string.*/
typedef struct{
	/**The length*/
	uintptr_t len;
	/**The text*/
	char* txt;
} SizePtrString;

/**
 * This will package a string constant to a size pointer string (implicitly const).
 * @param toPackage The string to pack.
 * @return As a SizePtrString.
 */
SizePtrString toSizePtr(const char* toPackage);

/**
 * This will package characters in a string.
 * @param toPackage The string to pack.
 * @return As a SizePtrString.
 */
SizePtrString toSizePtr(std::string* toPackage);

/**
 * This will package characters in a vector.
 * @param toPackage The string to pack.
 * @return As a SizePtrString.
 */
SizePtrString toSizePtr(std::vector<char>* toPackage);

/**
 * This will package a string constant to a size pointer string.
 * @param packLen The length of said string.
 * @param toPackage The string to pack.
 * @return As a SizePtrString.
 */
SizePtrString toSizePtr(uintptr_t packLen, char* toPackage);

/**
 * Parse an integer.
 * @param toParse The string to parse.
 * @return The parsed value.
 */
intptr_t parseInt(SizePtrString toParse);

/**
 * Parse a float.
 * @param toParse The string to parse.
 * @return The parsed value.
 */
double parseFloat(SizePtrString toParse);

/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator < (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator > (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator <= (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator >= (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator == (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Compare two strings in lexicographic order.
 * @param strA The first "string": length and characters.
 * @param strB The second "string": length and characters.
 * @return Whether strA is less than strB.
 */
bool operator != (const SizePtrString& strA, const SizePtrString& strB);
/**
 * Output a size pointer string.
 * @param os The place to write.
 * @param toOut The thing to write.
 * @return the stream.
 */
std::ostream& operator<<(std::ostream& os, SizePtrString const & toOut);

/**Heavyweight operations for memory.*/
class MemoryShuttler{
public:
	/**Simple set up.*/
	MemoryShuttler();
	/**Allow subclasses*/
	virtual ~MemoryShuttler();
	/**
	 * Run a memcpy.
	 * @param cpyTo The place to copy to.
	 * @param cpyFrom The place to copy from.
	 * @param copyNum The number of bytes to copy.
	 */
	virtual void memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum) = 0;
	/**
	 * Run a memset.
	 * @param setP The first byte to set.
	 * @param value The avlue to set to.
	 * @param numBts The number of bytes to set.
	 */
	virtual void memset(void* setP, int value, size_t numBts) = 0;
	/**
	 * Run a memswap.
	 * @param arrA The first array.
	 * @param arrB The second array.
	 * @param numBts The number of bytes to swap.
	 */
	virtual void memswap(char* arrA, char* arrB, size_t numBts) = 0;
	
	//TODO chunk_stride_set chunk_stride_copy?
};
/**Use the c standard library to shuttle memory.*/
class StandardMemoryShuttler : public MemoryShuttler{
public:
	/**No special set up.*/
	StandardMemoryShuttler();
	/**No special tear down.*/
	~StandardMemoryShuttler();
	void memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum);
	void memset(void* setP, int value, size_t numBts);
	void memswap(char* arrA, char* arrB, size_t numBts);
};

/**Scan through strings/memory.*/
class MemorySearcher{
public:
	/**Simple set up*/
	MemorySearcher();
	/**Tear down.*/
	virtual ~MemorySearcher();
	
	/**
	 * Compare two chunks of memory.
	 * @param str1 The first chunk.
	 * @param str2 The second chunk.
	 * @param num The number of bytes.
	 * @return The relative comparison.
	 */
	virtual int memcmp(const char* str1, const char* str2, size_t num) = 0;
	/**
	 * Find the location of a character in a block of memory.
	 * @param str1 The block to look through.
	 * @param value The value to look for.
	 * @param num The number of bytes in the block.
	 * @return The found location, or null if not present.
	 */ 
	virtual void* memchr(const void* str1, int value, size_t num) = 0;
	/**
	 * Return the number of characters in str1 until one of the characters in str2 is encountered (or the end is reached).
	 * @param str1 The string to walk along.
	 * @param numB1 The length of said string.
	 * @param str2 The characters to search for.
	 * @param numB2 The number of characters to search for.
	 * @return The number of characters until a break (one of the characters in str2 or end of string).
	 */
	virtual size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2) = 0;
	/**
	 * Return the number of characters in str1 until something not in str2 is encountered.
	 * @param str1 The string to walk along.
	 * @param numB1 The length of said string.
	 * @param str2 The characters to search for.
	 * @param numB2 The number of characters to search for.
	 * @return The number of characters until a break (something not in str2 or end of string).
	 */
	virtual size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2) = 0;
	/**
	 * Returns a pointer to the first occurence of str2 in str1.
	 * @param str1 The string to walk along.
	 * @param numB1 The length of said string.
	 * @param str2 The string to search for.
	 * @param numB2 The length of said string.
	 * @return The location of str2 in str1, or null if not present.
	 */
	virtual char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2) = 0;
	
	//utilities
	
	/**
	 * Determine if one memory block ends with another.
	 * @param str1 The large string.
	 * @param numB1 The size of str1.
	 * @param str2 The suspected ending.
	 * @param numB2 The size of str2.
	 * @return Whether str1 ends with str2.
	 */
	int memendswith(const char* str1, size_t numB1, const char* str2, size_t numB2);
	/**
	 * Determine if one memory block ends with another.
	 * @param str1 The large string.
	 * @param numB1 The size of str1.
	 * @param str2 The suspected ending.
	 * @param numB2 The size of str2.
	 * @return Whether str1 ends with str2.
	 */
	int memstartswith(const char* str1, size_t numB1, const char* str2, size_t numB2);
	
	/**
	 * Determine if one memory block ends with another.
	 * @param str1 The large string.
	 * @param str2 The suspected ending.
	 * @return Whether str1 ends with str2.
	 */
	int memendswith(SizePtrString str1, SizePtrString str2);
	
	/**
	 * Determine if one memory block ends with another.
	 * @param str1 The large string.
	 * @param str2 The suspected ending.
	 * @return Whether str1 ends with str2.
	 */
	int memstartswith(SizePtrString str1, SizePtrString str2);
	
	/**
	 * Trim sequence off of a string.
	 * @param strMain The string to trim.
	 * @param toRem The character(s) to trim off.
	 * @return A substring of strMain.
	 */
	SizePtrString trim(SizePtrString strMain, SizePtrString toRem);
	
	/**
	 * Remove multiple different possible sequences from the start.
	 * @param strMain The string to trim.
	 * @param numRem The number of possible strings to remove.
	 * @param toRem The strings to remove
	 * @return A substring of strMain.
	 */
	SizePtrString trim(SizePtrString strMain, uintptr_t numRem, SizePtrString* toRem);
	
	/**
	 * Remove standard whitespace characters from a string.
	 * @param strMain The string to trim.
	 * @return A substring of strMain.
	 */
	SizePtrString trim(SizePtrString strMain);
	
	/**
	 * Split a string at the first instance of another string.
	 * @param toSplit The thing to split.
	 * @param toSplitOn THe thing to look for.
	 * @return Everything up to the split target, and everything after. If no split target, first is toSplit and second is null and empty.
	 */
	std::pair<SizePtrString,SizePtrString> split(SizePtrString toSplit, SizePtrString toSplitOn);
};
/**Do searches with the standard library.*/
class StandardMemorySearcher : public MemorySearcher{
public:
	/**Simple set up.*/
	StandardMemorySearcher();
	/**Tear down.*/
	~StandardMemorySearcher();
	int memcmp(const char* str1, const char* str2, size_t num);
	void* memchr(const void* str1, int value, size_t num);
	size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
	size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
	char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2);
};

/**Pack things into bytes.*/
class BytePacker{
public:
	
	/**
	 * Set up an uninitialized packer.
	 */
	BytePacker();
	/**
	 * Set up a packer.
	 * @param toFill The place to fill.
	 */
	BytePacker(char* toFill);
	/**Tear down.*/
	~BytePacker();
	
	/**
	 * Set where this will be writing to.
	 * @param toFill The place to fill.
	 */
	void retarget(char* toFill);
	
	/**The place to pack to.*/
	char* target;
	
	/**
	 * Skip some bytes.
	 * @param numSkip The number to skip.
	 */
	void skip(uintptr_t numSkip);
	/**
	 * Pack a 64-bit integer big endian.
	 * @param toPack The integer to pack.
	 */
	void packBE64(uint64_t toPack);
	/**
	 * Pack a 32-bit integer big endian.
	 * @param toPack The integer to pack.
	 */
	void packBE32(uint32_t toPack);
	/**
	 * Pack a 16-bit integer big endian.
	 * @param toPack The integer to pack.
	 */
	void packBE16(uint16_t toPack);
	/**
	 * Pack a 64-bit float.
	 * @param toPack The float to pack.
	 */
	void packBEDbl(double toPack);
	/**
	 * Pack a 32-bit float.
	 * @param toPack The float to pack.
	 */
	void packBEFlt(float toPack);
	
	/**
	 * Pack a 64-bit integer little endian.
	 * @param toPack The integer to pack.
	 */
	void packLE64(uint64_t toPack);
	/**
	 * Pack a 32-bit integer little endian.
	 * @param toPack The integer to pack.
	 */
	void packLE32(uint32_t toPack);
	/**
	 * Pack a 16-bit integer little endian.
	 * @param toPack The integer to pack.
	 */
	void packLE16(uint16_t toPack);
	/**
	 * Pack a 64-bit float.
	 * @param toPack The float to pack.
	 */
	void packLEDbl(double toPack);
	/**
	 * Pack a 32-bit float.
	 * @param toPack The float to pack.
	 */
	void packLEFlt(float toPack);
};

/**Unpack things from bytes.*/
class ByteUnpacker{
public:
	
	/**
	 * Set up an uninitialized unpacker.
	 */
	ByteUnpacker();
	/**
	 * Set up an unpacker.
	 * @param toFill The place to read.
	 */
	ByteUnpacker(char* toFill);
	/**Tear down.*/
	~ByteUnpacker();
	
	/**
	 * Set where this will be reading from.
	 * @param toFill The place to read.
	 */
	void retarget(char* toFill);
	
	/**The place to pack to.*/
	char* target;
	
	/**
	 * Skip some bytes.
	 * @param numSkip The number to skip.
	 */
	void skip(uintptr_t numSkip);
	/**
	 * Unack a 64-bit integer big endian.
	 * @return The integer
	 */
	uint64_t unpackBE64();
	/**
	 * Unack a 64-bit integer big endian.
	 * @return The integer
	 */
	uint32_t unpackBE32();
	/**
	 * Unack a 64-bit integer big endian.
	 * @return The integer
	 */
	uint16_t unpackBE16();
	/**
	 * Unack a 64-bit float big endian.
	 * @return The float
	 */
	double unpackBEDbl();
	/**
	 * Unack a 32-bit float big endian.
	 * @return The float
	 */
	float unpackBEFlt();
	/**
	 * Unack a 64-bit integer little endian.
	 * @return The integer
	 */
	uint64_t unpackLE64();
	/**
	 * Unack a 64-bit integer little endian.
	 * @return The integer
	 */
	uint32_t unpackLE32();
	/**
	 * Unack a 64-bit integer little endian.
	 * @return The integer
	 */
	uint16_t unpackLE16();
	/**
	 * Unack a 64-bit float little endian.
	 * @return The float
	 */
	double unpackLEDbl();
	/**
	 * Unack a 32-bit float little endian.
	 * @return The float
	 */
	float unpackLEFlt();
};

//odd things missing from string.h

/**
 * Return the number of characters in str1 until one of the characters in str2 is encountered (or the end is reached).
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (one of the characters in str2 or end of string).
 */
size_t memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
/**
 * Return the number of characters in str1 until something not in str2 is encountered.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The characters to search for.
 * @param numB2 The number of characters to search for.
 * @return The number of characters until a break (something not in str2 or end of string).
 */
size_t memspn(const char* str1, size_t numB1, const char* str2, size_t numB2);
/**
 * Returns a pointer to the first occurence of str2 in str1.
 * @param str1 The string to walk along.
 * @param numB1 The length of said string.
 * @param str2 The string to search for.
 * @param numB2 The length of said string.
 * @return The location of str2 in str1, or null if not present.
 */
char* memmem(const char* str1, size_t numB1, const char* str2, size_t numB2);

/**
 * This will swap, byte by byte, the contents of two (non-overlapping) arrays.
 * @param arrA The first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap(char* arrA, char* arrB, size_t numBts);

};

#endif

