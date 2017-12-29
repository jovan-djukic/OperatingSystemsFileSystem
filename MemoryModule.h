#ifndef _MEMORY_MODULE_H_
#define _MEMORY_MODULE_H_

#include "SwapAlgorithm.h"
#include "TagMemory.h"
#include "Monitor.h"
#include "BoundedBuffer.h"
#include "part.h"

class MemoryInterface {
public:
	virtual int read(unsigned long, char*) = 0;
	virtual int write(unsigned long, const char*) = 0;
	virtual ~MemoryInterface() = 0;
};

class MemoryModule : public MemoryInterface {
public:
	virtual void flush() = 0;
	virtual ~MemoryModule() = 0;
};

class CacheMemory : public MemoryModule {
public:
	CacheMemory(int, int, SwapAlgorithm*, MemoryInterface*);
	virtual void flush() override;
	virtual ~CacheMemory();
protected:
	int preOperation(unsigned long);
	virtual int getNewBlock(unsigned long);

	char ***data;
	int numOfEntries, sizeOfEntry;
	char *D, *V;
	int Vcapacity, Dcapacity;
	TagMemory *tagMemory;
	SwapAlgorithm * swapAlgorithm;
	MemoryInterface *memoryModule;
};

class ByteCacheMemory : public CacheMemory {
public:
	ByteCacheMemory(int, int, MemoryInterface*);
	ByteCacheMemory(int, int, SwapAlgorithm*, MemoryInterface*);
	virtual int read(unsigned long, char*) override;
	virtual int write(unsigned long, const char*) override;
private:
	unsigned long lastAccessedBlock, lastAccessedEntry;
};

class BlockCacheMemory : public CacheMemory {
public:
	BlockCacheMemory(int, int, MemoryInterface*);
	BlockCacheMemory(int, int, SwapAlgorithm*, MemoryInterface*);
	virtual int read(unsigned long, char*) override;
	virtual int write(unsigned long, const char*) override;
};

class ThreadSafeBlockCache : public BlockCacheMemory {
public:
	ThreadSafeBlockCache(int, int, int, MemoryInterface*);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
	void flush() override;
	~ThreadSafeBlockCache();
protected:
	int getNewBlock(unsigned long) override;
private:
	ReadersWritersMonitor *monitor;
};

struct WriteRequest {
	unsigned long blockNo;
	const char *buffer;
	WriteRequest(unsigned long, const char*);
	~WriteRequest();
};

typedef BoundedBuffer<WriteRequest*> BlockBuffer;

class WriteRequestHandler : public RequestHandler {
public:
	WriteRequestHandler(BlockBuffer*, MemoryInterface*, ModuleMonitor*);
	~WriteRequestHandler();
protected:
	void handleRequest() override;
private:
	BlockBuffer *buffer;
	MemoryInterface *memoryModule;
	ModuleMonitor *monitor;
};

class BufferedWriter : public MemoryInterface {
public:
	BufferedWriter(int, MemoryInterface*);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
	~BufferedWriter();
private:
	BlockBuffer *buffer;
	RequestHandler *handler;
	MemoryInterface *memoryModule;
	ModuleMonitor *monitor;
};

class PartitionWrapper : public MemoryInterface {
public:
	PartitionWrapper(Partition*, unsigned long);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
private:
	unsigned long numOfClusters;
	Partition *partition;
};

class MemoryMapper : public MemoryInterface {
public:
	MemoryMapper(MemoryInterface*, CacheMemory*, unsigned long);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
	void setIndexCluster(unsigned long);
private:
	unsigned long LogToPh(unsigned long);

	unsigned long indexCluster;
	CacheMemory *indexCache;
	MemoryInterface *memoryModule;
};

class DataMemoryChecker : public MemoryInterface {
public:
	DataMemoryChecker(MemoryInterface*);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
private:
	MemoryInterface *memoryModule;
};

class IndexMemoryChecker : public MemoryInterface {
public:
	IndexMemoryChecker(MemoryInterface*);
	int read(unsigned long, char *) override;
	int write(unsigned long, const char *) override;
private:
	MemoryInterface *memoryModule;
};

namespace utilities {
	int getBitFromBitvector(char*, int);
	void setBitInBitvector(char*, int, int);
}


#endif