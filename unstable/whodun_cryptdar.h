#ifndef WHODUN_CRYPTDAR_H
#define WHODUN_CRYPTDAR_H 1

/**
 * @file
 * @brief Cryptography for data at rest.
 */
 
 #include <stdint.h>
 
 #include "whodun_oshook.h"
 #include "whodun_string.h"
 #include "whodun_thread.h"

namespace whodun {

/*************************************************************/
/* Encryption */

/**The number of bytes in an AES block.*/
#define WHODUN_AESBLOCK 16
/**The number of bytes in an AES256 key.*/
#define WHODUN_AES256KEYSIZE 32
/**The number of rounds in AES256.*/
#define WHODUN_AES256ROUNDS 14

/**The actual implementation of AES 256.*/
class AES256CryptoKernel{
public:
	
	/**
	 * Set up the kernel.
	 * @param useKey The 32 byte key to use.
	 */
	AES256CryptoKernel(char* useKey);
	/**
	 * Clean up.
	 */
	~AES256CryptoKernel();
	
	/**
	 * Encrypt some data.
	 * @param toEncrypt The 16 bytes to encrypt.
	 */
	void encrypt(char* toEncrypt);
	
	/**
	 * Decrypt some data.
	 * @param toDecrypt The 16 bytes to decrypt.
	 */
	void decrypt(char* toDecrypt);
	
	/**The key.*/
	char savedKey[(WHODUN_AES256ROUNDS+1)*WHODUN_AESBLOCK];
};

/**The number of bytes in a nonce for aes in ctr mode.*/
#define WHODUN_AESCTRNONCE 8

/**Read an AES256 stream, encrypted in CTR mode.*/
class AES256CTRInStream : public InStream{
public:
	
	/**
	 * Decrypt a stream.
	 * @param wrapStr The stream to decrypt.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 */
	AES256CTRInStream(InStream* wrapStr, char* useKey, char* useNonce);
	/**
	 * Decrypt a stream.
	 * @param wrapStr The stream to decrypt.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	AES256CTRInStream(InStream* wrapStr, char* useKey, char* useNonce, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~AES256CTRInStream();
	
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	
	/**The base stream*/
	InStream* baseStr;
	/**The index of the next block.*/
	uintptr_t nextBlockIndex;
	
	/**The base encryption.*/
	AES256CryptoKernel baseCrypt;
	/**The nonce for this thing.*/
	char nonce[WHODUN_AESCTRNONCE];
	
	/**Space to store data read but not yet requested.*/
	char overhang[WHODUN_AESBLOCK];
	/**The number of bytes in the overhang.*/
	uintptr_t numOverhang;
	/**The offset of the first thing in the overhang.*/
	uintptr_t overhangOff;
	
	/**Save the thread handler.*/
	ParallelForLoop* saveDo;
	/**The threads to use, if any.*/
	ThreadPool* usePool;
	/**The number of thread to use.*/
	uintptr_t numThr;
};

/**Random access to an encrypted stream.*/
class AES256CTRRandaccInStream : public RandaccInStream{
public:
	/**
	 * Decrypt a stream.
	 * @param wrapStr The stream to decrypt.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 */
	AES256CTRRandaccInStream(RandaccInStream* wrapStr, char* useKey, char* useNonce);
	/**
	 * Decrypt a stream.
	 * @param wrapStr The stream to decrypt.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	AES256CTRRandaccInStream(RandaccInStream* wrapStr, char* useKey, char* useNonce, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~AES256CTRRandaccInStream();
	
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	uintmax_t tell();
	void seek(uintmax_t toLoc);
	uintmax_t size();
	
	/**Helper method to actually perform a seek.*/
	void fixSeek();
	
	/**The base stream*/
	RandaccInStream* baseStr;
	/**The actual reader ().*/
	AES256CTRInStream* realRead;
	/**Whether the streams need a seek to happen (i.e. a random access function was called).*/
	int needSeek;
	/**The next byte to report.*/
	uintmax_t focusInd;
	/**The total number of bytes in the file.*/
	uintmax_t totalNInd;
};

/**Output encrypted data.*/
class AES256CTROutStream : public OutStream{
public:
	
	/**
	 * Encrypt a stream.
	 * @param wrapStr The stream to dump data to.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 */
	AES256CTROutStream(OutStream* wrapStr, char* useKey, char* useNonce);
	/**
	 * Decrypt a stream.
	 * @param wrapStr The stream to dump data to.
	 * @param useKey The aes key.
	 * @param useNonce The nonce: 8 bytes.
	 * @param numThread The number of threads to use.
	 * @param mainPool The threads to use.
	 */
	AES256CTROutStream(OutStream* wrapStr, char* useKey, char* useNonce, uintptr_t numThread, ThreadPool* mainPool);
	/**Clean up.*/
	~AES256CTROutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void close();
	
	/**The base stream*/
	OutStream* baseStr;
	/**The index of the next block.*/
	uintptr_t nextBlockIndex;
	
	/**The base encryption.*/
	AES256CryptoKernel baseCrypt;
	/**The nonce for this thing.*/
	char nonce[WHODUN_AESCTRNONCE];
	
	/**Save the thread handler.*/
	ParallelForLoop* saveDo;
	/**The threads to use, if any.*/
	ThreadPool* usePool;
	/**The number of thread to use.*/
	uintptr_t numThr;
	
	/**The place to pack things for encryption*/
	char* packBuffer;
	/**The size of said place.*/
	uintptr_t packSize;
	/**Space to store data read but not yet requested.*/
	char overhang[WHODUN_AESBLOCK];
	/**The number of bytes in the overhang.*/
	uintptr_t numOverhang;
};

/*************************************************************/
/* Hashing */

/**The amount of data sha256 works with in one go.*/
#define WHODUN_SHA256_CHUNKSIZE 64
/**The size of the hash resulting from sha256.*/
#define WHODUN_SHA256_HASHSIZE 32

/**Compute a hash.*/
class SHA256HashKernel{
public:
	
	/**Set up an empty hash.*/
	SHA256HashKernel();
	/**Clean up.*/
	~SHA256HashKernel();
	
	/**
	 * Add bytes to the hash.
	 * @param toAdd The bytes to add.
	 */
	void addBytes(SizePtrString toAdd);
	
	/**
	 * Finish off the stream, get the hash and reset.
	 * @param toSave The place to put the hash (32 bytes).
	 */
	void getHash(char* toSave);
	
	/**
	 * Reset the hash.
	 */
	void reset();
	
	/**
	 * Add a 64 byte chunk to the hash.
	 * @param toAdd The chunk to add.
	 */
	void addChunk(char* toAdd);
	
	/**The total length of the message*/
	uint64_t totalLength;
	/**The value of the current hash.*/
	uint32_t hashVals[8];
	/**The amount of text waiting to go.*/
	int numStore;
	/**Store text waiting to be hashed.*/
	char tmpStoreBuff[WHODUN_SHA256_HASHSIZE];
};



};

#endif

