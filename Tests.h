#ifndef _TEST_H_
#define _TEST_H_

#include <Windows.h>
#include <string>

class ConcurentIO {
public:
	ConcurentIO(HANDLE);
	~ConcurentIO();
	void write(std::string&);
private:
	HANDLE semHandle;
};


#include "MemoryModule.h"
#include "Thread.h"
#include "Semaphore.h"
#include "BoundedBuffer.h"
#include "Monitor.h"
#include "MemoryManagment.h"

class ModuleSimulator : public MemoryInterface {
public:
	ModuleSimulator(int);
	int read(unsigned long, char*) override;
	int write(unsigned long, const char*) override;
	int getNumOfCluster() const;
	~ModuleSimulator();
private:
	char **memory;
	int capacity;
	CRITICAL_SECTION criticalSection;
};

class LRUSimulator : public SwapAlgorithm {
public:
	LRUSimulator(int);
	void update(int) override;
	int getSwapEntry() override;
	~LRUSimulator();
private:
	int *entries;
	int capacity;
};

class MyThread : public Thread {
public:
	MyThread(Semaphore*, int);
	void run() override;
private:
	Semaphore *ioSem;
	int id;
};

class Reader : public Thread {
public:
	Reader(Semaphore *_ioSem, ReadersWritersMonitor *_rwm, int _num) : ioSem(_ioSem), rwm(_rwm), numOfIterations(_num) {

	}
	void run() override;
private:
	Semaphore *ioSem;
	ReadersWritersMonitor *rwm;
	int numOfIterations;
};

class Writer : public Thread {
public:
	Writer(Semaphore*, ReadersWritersMonitor*, int);
	void run() override;
private:
	Semaphore *ioSem;
	ReadersWritersMonitor *rwm;
	int numOfIterations;
};

typedef BoundedBuffer<int> IntBuffer;

class Producer : public Thread {
public:
	Producer(Semaphore*, int, IntBuffer*);
	void run() override;
private:
	static int pos_id;
	int id = pos_id++;
	Semaphore *ioSem;
	int numOfProfucts;
	IntBuffer *myBuffer;
};

class Consumer : public Thread {
public:
	Consumer(Semaphore*, int, IntBuffer*);
	void run() override;
private:
	static int pos_id;
	int id = pos_id++;

	Semaphore *ioSem;
	int numOfProducts;
	IntBuffer *myBuffer;
};

class MyHandler : public RequestHandler {
public:
	MyHandler();
	~MyHandler();
protected:
	void handleRequest() override;
private:
	int requests;
};

class PartitionThread : public Thread {
public:
	PartitionThread(unsigned long, unsigned long, MemoryInterface*);
	virtual ~PartitionThread();
protected:
	virtual void operation(MemoryInterface*,unsigned long, char*) = 0;
	void run() override;
private:
	unsigned long startBlock, numOfBlocks;
	MemoryInterface *memoryModule;
};

class PartitionReader : public PartitionThread {
public:
	PartitionReader(unsigned long, unsigned long, MemoryInterface*);
	~PartitionReader();
protected:
	void operation(MemoryInterface*,unsigned long, char*) override;
};

class PartitionWriter : public PartitionThread {
public:
	PartitionWriter(unsigned long, unsigned long, MemoryInterface*);
	~PartitionWriter();
protected:
	void operation(MemoryInterface*, unsigned long, char*) override;
};

class BufferedWriterThread : public Thread {
public:
	BufferedWriterThread(int, int, MemoryInterface*);
	~BufferedWriterThread();
protected:
	void run() override;
private:
	MemoryInterface *memoryModule;
	int numOfOperations, lineSize;
};

class KernelFileThread : public Thread {
public:
	KernelFileThread(MemoryInterface*, MemoryManagmentUnit*, int);
	~KernelFileThread();
protected:
	void run() override;
private:
	MemoryInterface *memoryModule;
	MemoryManagmentUnit *partitionMemoryManagment;
	int numOfDataClusters;
};

namespace tests {
	int lruTest();
	int BlockCacheMemoryTest();
	int ByteCacheMemoryTest();
	int ThreadAndSemaphoreTest();
	int MonitorTest();
	int BoundedBufferTest();
	int SingleThreadSafeLRUTest();
	int RequestsHandlerTest();
	int PartitionCacheCompareAndTest();
	int BufferedWriterTest();
	int QueueTest();
	int KernelFileTest();
	int MultiThreadKernelFileTest();//includes partition memory managment test
	int BitVectorTest();
	int BasicPartitionFunctionTest();
}

#endif