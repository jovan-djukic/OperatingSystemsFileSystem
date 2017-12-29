#include <iostream>
#include <string>
#include <Windows.h>
#include <ctime>
#include <fstream>

#include "Tests.h"
#include "part.h"
#include "MemoryModule.h"
#include "SwapAlgorithm.h"
#include "Thread.h"
#include "Semaphore.h"
#include "Monitor.h"
#include "Queue.h"
#include "MemoryManagment.h"
#include "KernelFile.h"
#include "defs.h"
#include "part.h"
#include "file.h"

ConcurentIO::ConcurentIO(HANDLE _semHandle) : semHandle(_semHandle) {
	//semHandle = CreateSemaphore(NULL, 1, 1, NULL);
}

ConcurentIO::~ConcurentIO() {
	//CloseHandle(semHandle);
}

void ConcurentIO::write(std::string& string) {
	WaitForSingleObject(semHandle, INFINITE);
	std::cout << string << std::endl << std::endl;
	ReleaseSemaphore(semHandle, 1, NULL);
}


LRUSimulator::LRUSimulator(int _capacity) : capacity(_capacity) {
	entries = new int[capacity];
	for (int i = 0; i < capacity; i++) {
		entries[i] = capacity - 1 - i;
	}
}

LRUSimulator::~LRUSimulator() {
	delete[] entries;
}

void LRUSimulator::update(int entrie) {
	for (int i = 0; i < capacity; i++) {
		if (entries[i] < entries[entrie]) {
			entries[i]++;
		}
	}
	entries[entrie] = 0;
}

int LRUSimulator::getSwapEntry() {
	int max = -1, maxEntrie = -1;
	for (int i = 0; i < capacity; i++) {
		if (entries[i] > max) {
			max = entries[i];
			maxEntrie = i;
		}
	}
	return maxEntrie;
}

int tests::lruTest() {
	std::cout << "CAPACITY?" << std::endl;
	int capacity = 0;
	std::cin >> capacity;

	LRUSimulator *lruSim = new LRUSimulator(capacity);
	LRU *lru = new LRU(capacity);
	
	std::cout << "MY LRU INIT" << std::endl;
	for (int i = 0; i < capacity; i++) {
		int entrie = lru->getSwapEntry();
		std::cout << "ENTRIE: " << entrie << std::endl;
		lru->update(entrie);
	}

	while (true) {
		std::cout << "CHOICE: 1 - MANUAL ENTRIE, 2 - RAND ENTRIE, 3 - SWAP ENTRIE" << std::endl;
		int choice = 0;
		std::cin >> choice;
		switch (choice) {
		case 1:	{
			int entrie = 0;
			std::cout << "ENTRIE?" << std::endl;
			std::cin >> entrie;
			lru->update(entrie);
			lruSim->update(entrie);
			break;
		}
		case 2: {
			int numOfIterations = 0;
			std::cout << "NUM OF ITERATIONS" << std::endl;
			std::cin >> numOfIterations;
			for (int i = 0; i < numOfIterations; i++) {
				int entrie = (double)std::rand() / RAND_MAX * capacity;
				std::cout << "ENTRIE: " << entrie << std::endl;
				lru->update(entrie);
				lruSim->update(entrie);
				std::cout << "LRU SWAP ENTRIE: " << lru->getSwapEntry() << std::endl;
				std::cout << "LRUSIM SWAP ENTRIE: " << lruSim->getSwapEntry() << std::endl;
				if (lru->getSwapEntry() != lruSim->getSwapEntry()) {
					std::cout << "EROOR" << std::endl;
					break;
				}
			}
			break;
		}
		case 3: {
			std::cout << "LRU SWAP ENTRIE: " << lru->getSwapEntry() << std::endl;
			std::cout << "LRUSIM SWAP ENTRIE: " << lruSim->getSwapEntry() << std::endl;
			break;
		}
		}

		if (choice <= 0 || choice >= 4) break;
	}

	delete lru;
	delete lruSim;
	return 0;
}

MyThread::MyThread(Semaphore *_ioSem, int _id) : ioSem(_ioSem), id(_id) {

}

void MyThread::run() {
	ioSem->wait();
	std::cout << "THREAD " << id << " WRITING" << std::endl;
	for (int i = 0; i < 10000; i++)
		for (int j = 0; j < 10000; j++);
	ioSem->signal();
}

int tests::ThreadAndSemaphoreTest() {
	std::cout << "NUM OF THREADS?" << std::endl;
	int numOfThreads = 0;
	std::cin >> numOfThreads;

	Semaphore *ioSem = new Semaphore(1, 1);

	MyThread **threads = new MyThread*[numOfThreads];

	for (int i = 0; i < numOfThreads; i++) {
		threads[i] = new MyThread(ioSem, i);
		threads[i]->start();
	}

	ioSem->wait();
	std::cout << "MAIN THREAD WAITING" << std::endl;
	ioSem->signal();

	for (int i = 0; i < numOfThreads; i++) {
		//threads[i]->waitToComplete();
		delete threads[i];
	}

	delete[] threads;
	delete ioSem;

	std::cout << "MAIN THREAD FINISHED" << std::endl;
	
	return 0;
}

void Reader::run() {
	for (int i = 0; i < numOfIterations; i++) {
		rwm->startRead();
		ioSem->wait();
		std::cout << "READING" << std::endl;
		ioSem->signal();
		rwm->endRead();
	}
}

Writer::Writer(Semaphore *_ioSem, ReadersWritersMonitor *_rwm, int _num) : ioSem(_ioSem), rwm(_rwm), numOfIterations(_num) {

}

void Writer::run() {
	for (int i = 0; i < numOfIterations; i++) {
		rwm->startWrite();
		ioSem->wait();
		std::cout << "writing" << std::endl;
		ioSem->signal();
		rwm->endWrite();
	}
}

int tests::MonitorTest() {
	std::cout << "NUM OF READERS?" << std::endl;
	int numOfReaders = 0;
	std::cin >> numOfReaders;

	std::cout << "NUM OF WRITERS?" << std::endl;
	int numOfWriters = 0;
	std::cin >> numOfWriters;

	std::cout << "NUM OF ITERATIONS?" << std::endl;
	int numOfIterations = 0;
	std::cin >> numOfIterations;

	Semaphore *ioSem = new Semaphore(1, 1);
	Thread **readers = new Thread*[numOfReaders];
	Thread **writers = new Thread*[numOfWriters];

	std::cout << "testsING EQUAL RIGHTS" << std::endl;

	ReadersWritersMonitor *rwm = new ReaderPreferance();

	std::cout << "CREATING THREADS" << std::endl;
	for (int i = 0; i < numOfReaders; i++) {
		readers[i] = new Reader(ioSem, rwm, numOfIterations);
	}
	for (int i = 0; i < numOfWriters; i++) {
		writers[i] = new Writer(ioSem, rwm, numOfIterations);
	}

	std::cout << "STARTING THREADS" << std::endl;
	for (int i = 0; i < numOfWriters; i++) {
		writers[i]->start();
	}
	for (int i = 0; i < numOfReaders; i++) {
		readers[i]->start();
	}
	
	ioSem->wait();
	std::cout << "WAITING FOR THREADS" << std::endl;
	ioSem->signal();

	for (int i = 0; i < numOfReaders; i++) {
		readers[i]->waitToComplete();
		delete readers[i];
	}
	for (int i = 0; i < numOfWriters; i++) {
		writers[i]->waitToComplete();
		delete writers[i];
	}
	delete rwm;
	
	std::cout << std::endl;
	std::cout << "testsING WRITER PREFERANCE" << std::endl;

	rwm = new WriterPreferance();

	std::cout << "CREATING THREADS" << std::endl;
	for (int i = 0; i < numOfReaders; i++) {
		readers[i] = new Reader(ioSem, rwm, numOfIterations);
	}
	for (int i = 0; i < numOfWriters; i++) {
		writers[i] = new Writer(ioSem, rwm, numOfIterations);
	}

	std::cout << "STARTING THREADS" << std::endl;

	for (int i = 0; i < numOfReaders; i++) {
		readers[i]->start();
	}
	for (int i = 0; i < numOfWriters; i++) {
		writers[i]->start();
	}

	ioSem->wait();
	std::cout << "WAITING FOR THREADS" << std::endl;
	ioSem->signal();

	for (int i = 0; i < numOfReaders; i++) {
		readers[i]->waitToComplete();
		delete readers[i];
	}
	for (int i = 0; i < numOfWriters; i++) {
		writers[i]->waitToComplete();
		delete writers[i];
	}
	delete rwm;

	delete[] readers;
	delete[] writers;
	delete ioSem;
	return 0;
}

Producer::Producer(Semaphore *_ioSem, int _numOfProducts, IntBuffer *_myBuffer) : ioSem(_ioSem), numOfProfucts(_numOfProducts), myBuffer(_myBuffer) {

}

void Producer::run() {
	for (int i = 0; i < numOfProfucts; i++) {
		ioSem->wait();
		std::cout << "PRODUCER: " << id << " PRODUCING: " << i << std::endl;
		ioSem->signal();
		myBuffer->put(i);
	}
}

Consumer::Consumer(Semaphore *_ioSem, int _numOfProducts, IntBuffer *_myBuffer) : ioSem(_ioSem), numOfProducts(_numOfProducts), myBuffer(_myBuffer) {

}
 
void Consumer::run() {
	for (int i = 0; i < numOfProducts; i++) {
		int product = myBuffer->get();
		ioSem->wait();
		std::cout << "CONSUMER: " << id << " CONSUMING: " << product << std::endl;
		ioSem->signal();
	}
}

int Consumer::pos_id = 0;
int Producer::pos_id = 0;

int tests::BoundedBufferTest() {
	std::cout << "BOUNDED BUFFER CAPACITY?" << std::endl;
	int capacity;
	std::cin >> capacity;

	std::cout << "NUM OF PRODUCERS?" << std::endl;
	int numOfProducers;
	std::cin >> numOfProducers;

	std::cout << "NUM OF PROFUCER PRODUCTS?" << std::endl;
	int producerProducts;
	std::cin >> producerProducts;

	std::cout << "NUM OF CONSUMERS?" << std::endl;
	int numOfConsumers;
	std::cin >> numOfConsumers;

	std::cout << "NUM OF  CONSUMERS PRODUCTS?" << std::endl;
	int consumerProducts;
	std::cin >> consumerProducts;

	if (numOfProducers * producerProducts < numOfConsumers * consumerProducts || numOfProducers * producerProducts - numOfConsumers * consumerProducts > capacity) {
		std::cout << "PARAMETERS ERROR" << std::endl;
	}

	Producer **producers = new Producer*[numOfProducers];
	Consumer **consumers = new Consumer*[numOfConsumers];
	Semaphore *ioSem = new Semaphore(1, 1);
	IntBuffer *buffer = new IntBuffer(capacity);

	for (int i = 0; i < numOfProducers; i++) {
		producers[i] = new Producer(ioSem, producerProducts, buffer);
	}
	for (int i = 0; i < numOfConsumers; i++) {
		consumers[i] = new Consumer(ioSem, consumerProducts, buffer);
	}

	for (int i = 0; i < numOfProducers; i++) {
		producers[i]->start();
	}
	for (int i = 0; i < numOfConsumers; i++) {
		consumers[i]->start();
	}

	ioSem->wait();
	std::cout << "MAIN THREAD WRITING" << std::endl;
	ioSem->signal();

	for (int i = 0; i < numOfProducers; i++) {
		producers[i]->waitToComplete();
		delete producers[i];
	}
	for (int i = 0; i < numOfConsumers; i++) {
		consumers[i]->waitToComplete();
		delete consumers[i];
	}
		 
	delete buffer;
	delete ioSem;
	delete[] producers;
	delete[] consumers;
	return 0;
}

ModuleSimulator::ModuleSimulator(int numOfClusters) : capacity(numOfClusters) {
	memory = new char*[numOfClusters];
	for (int i = 0; i < capacity; i++) {
		memory[i] = new char[ClusterSize];
		memset(memory[i], 0, ClusterSize);
	}
	InitializeCriticalSection(&criticalSection);
}

ModuleSimulator::~ModuleSimulator() {
	for (int i = 0; i < capacity; i++) {
		delete[] memory[i];
	}
	delete[] memory;
	DeleteCriticalSection(&criticalSection);
}

int ModuleSimulator::read(unsigned long adr, char *buffer) {
	EnterCriticalSection(&criticalSection);
	Sleep(10);
	if (adr > capacity) {
		memset(buffer, '\0', ClusterSize);
	}
	else {
		memcpy(buffer, memory[adr], ClusterSize);
	}
	LeaveCriticalSection(&criticalSection);
	return 1;
}

#define debug_adr 8

int ModuleSimulator::write(unsigned long adr, const char *buffer) {
	EnterCriticalSection(&criticalSection);
	Sleep(10);
	if (adr > capacity) return 0;
	memcpy(memory[adr], buffer, ClusterSize);
	LeaveCriticalSection(&criticalSection);
	return 1;
}

int ModuleSimulator::getNumOfCluster() const {
	return capacity;
}

void PrintBuffer(char *buffer, int sizeOfBuffer) {
	for (int i = 0; i < sizeOfBuffer; i++) {
		std::cout << buffer[i] << " ";
	}
	std::cout << std::endl;
}

int tests::BlockCacheMemoryTest() {
	std::cout << "MODULE CAPACITY?" << std::endl;
	int numOfClusters;
	std::cin >> numOfClusters;

	std::cout << "CAHCHE NUM OF ENTRIES, SIZE OF ENTRIE?" << std::endl;
	int numOfEntries, sizeOfEntrie;
	std::cin >> numOfEntries;
	std::cin >> sizeOfEntrie;

	ModuleSimulator *memSim = new ModuleSimulator(numOfClusters);
	CacheMemory *cache = new BlockCacheMemory(numOfEntries, sizeOfEntrie, new LRU(numOfEntries), memSim);

	while (true) {
		std::cout << "1 - READ, 2 - WRITE, 3 - FLUSH?" << std::endl;
		int choice = 0;
		std::cin >> choice;
		switch (choice) {
		case 1: {
			char buffer[ClusterSize];
			std::cout << "ADR?" << std::endl;
			unsigned long adr;
			std::cin >> adr;
			cache->read(adr, buffer);
			PrintBuffer(buffer, ClusterSize);
			break;
		}
		case 2: {
			char buffer[ClusterSize];
			std::cout << "FILL CHAR?" << std::endl;
			char fillChar;
			std::cin >> fillChar;
			memset(buffer, fillChar, ClusterSize);
			std::cout << "ADR?" << std::endl;
			unsigned long adr;
			std::cin >> adr;
			cache->write(adr, buffer);
			break;
		}
		case 3: {
			cache->flush();
			break;
		}
		}
		if (choice == -1) break; 
	}

	delete memSim;
	delete cache;

	return 0;
}

int tests::ByteCacheMemoryTest() {
	std::cout << "MODULE CAPACITY?" << std::endl;
	int numOfClusters;
	std::cin >> numOfClusters;

	std::cout << "CAHCHE NUM OF ENTRIES, SIZE OF ENTRIE?" << std::endl;
	int numOfEntries, sizeOfEntrie;
	std::cin >> numOfEntries;
	std::cin >> sizeOfEntrie;

	ModuleSimulator *memSim = new ModuleSimulator(numOfClusters);
	CacheMemory *cache = new ByteCacheMemory(numOfEntries, sizeOfEntrie, new LRU(numOfEntries), memSim);

	while (true) {
		std::cout << "1 - READ, 2 - WRITE, 3 - FLUSH?" << std::endl;
		int choice = 0;
		std::cin >> choice;
		switch (choice) {
		case 1: {
			char buffer[ClusterSize];
			std::cout << "POSITION?" << std::endl;
			unsigned long pos = 0;
			std::cin >> pos;
			std::cout << "NUM OF BYTEST?" << std::endl;	
			unsigned long cnt = 0;
			std::cin >> cnt;
			for (int i = 0; i < cnt; i++) {
				cache->read(pos + i, buffer + i);
			}
			PrintBuffer(buffer, cnt);
			break;
		}
		case 2: {
			char buffer[ClusterSize];
			std::cout << "FILL CHAR?" << std::endl;
			char fillChar = 0;
			std::cin >> fillChar;
			memset(buffer, fillChar, ClusterSize);
			std::cout << "POSITION?" << std::endl;
			unsigned long pos = 0;
			std::cin >> pos;
			std::cout << "NUM OF BYTEST?" << std::endl;
			unsigned long cnt = 0;
			std::cin >> cnt;
			for (int i = 0; i < cnt; i++) {
				cache->write(pos + i, buffer + i);
			}
			break;
		}
		case 3: {
			cache->flush();
			break;
		}
		}
		if (choice == -1) break;
	}

	delete memSim;
	delete cache;

	return 0;
}

MyHandler::MyHandler() : requests(0) {

}

MyHandler::~MyHandler() {
	waitToComplete();
}

void MyHandler::handleRequest() {
	std::cout << "REQUEST " << requests << " HANDELED" << std::endl;
	requests++;
}

int tests::RequestsHandlerTest() {
	std::cout << "NUM OF ITERATIONS?" << std::endl;
	int numOfIterations = 0;
	std::cin >> numOfIterations;

	RequestHandler *handler = new MyHandler();
	handler->start();

	for (int i = 0; i < numOfIterations; i++) {
		handler->putRequest();
	}

	handler->stop();
	handler->waitToComplete();
	delete handler;

	std::cout << "MAIN THREAD FINISHED" << std::endl;

	return 0;
}

int tests::SingleThreadSafeLRUTest() {
	std::cout << "NUM OF ITERATIONS?" << std::endl;
	int numOfIterations = 0;
	std::cin >> numOfIterations;

	std::cout << "NUM OF ITERATIONS PRE SWAP ENTRY REQUEST?" << std::endl;
	int numOfIterationsPerSwapEntryRequest = 0;
	std::cin >> numOfIterationsPerSwapEntryRequest;

	std::cout << "LRU CAPACITY?" << std::endl;
	int lruCapacity = 0;
	std::cin >> lruCapacity;

	std::cout << "LRU REQUEST BUFFER CAPACITY?" << std::endl;
	int lruRequestBufferCapacity = 0;
	std::cin >> lruRequestBufferCapacity;

	LRUSimulator *lruSimulator = new LRUSimulator(lruCapacity);
	int *updateEntries = new int[numOfIterations];
	int *swapEntries = new int[numOfIterations / numOfIterationsPerSwapEntryRequest];

	for (int i = 0; i < numOfIterations; i++) {
		updateEntries[i] = (double)std::rand() / RAND_MAX * lruCapacity;
		lruSimulator->update(updateEntries[i]);
		if (i % numOfIterationsPerSwapEntryRequest == 0) {
			swapEntries[i / numOfIterationsPerSwapEntryRequest] = lruSimulator->getSwapEntry();
		}
	}

	SwapAlgorithm *threadSafeLRU = new ThreadSafeLRU(lruCapacity, lruRequestBufferCapacity);
	
	std::cout << "INITIALIZING LRU" << std::endl;
	for (int i = 0; i < lruCapacity; i++) {
		int entry = threadSafeLRU->getSwapEntry();
		threadSafeLRU->update(entry);
	}

	for (int i = 0; i < numOfIterations; i++) {
		threadSafeLRU->update(updateEntries[i]);
		if (i % numOfIterationsPerSwapEntryRequest == 0) {
			if (threadSafeLRU->getSwapEntry() != swapEntries[i / numOfIterationsPerSwapEntryRequest]) {
				std::cout << "ERROR" << std::endl;
			}
		}
	}

	delete threadSafeLRU;
	delete lruSimulator;
	delete updateEntries;
	delete swapEntries;

	return 0;
}

PartitionThread::PartitionThread(unsigned long _startBlock, unsigned long _numOfBlocks, MemoryInterface *_memoryModule) : memoryModule(_memoryModule), startBlock(_startBlock), numOfBlocks(_numOfBlocks) {

}

PartitionThread::~PartitionThread() {
	waitToComplete();
}

void PartitionThread::run() {
	char *buffer = new char[ClusterSize];
	for (int i = 0; i < numOfBlocks; i++) {
		operation(memoryModule, startBlock + i, buffer);
	}
	delete[] buffer;
}

PartitionReader::PartitionReader(unsigned long _startBlock, unsigned long _numOfBlocks, MemoryInterface *_memoryModule) : PartitionThread(_startBlock, _numOfBlocks, _memoryModule) {

}

PartitionReader::~PartitionReader() {
	waitToComplete();
}

PartitionWriter::PartitionWriter(unsigned long _startBlock, unsigned long _numOfBlocks, MemoryInterface *_memoryModule) : PartitionThread(_startBlock, _numOfBlocks, _memoryModule) {

}

PartitionWriter::~PartitionWriter() {
	waitToComplete();
}

void PartitionReader::operation(MemoryInterface *memoryModule, unsigned long blockNo, char *buffer) {
	char tmpBuffer[ClusterSize];
	memset(tmpBuffer, 'a' + blockNo, ClusterSize);
	memoryModule->read(blockNo, buffer);
	if (memcmp(tmpBuffer, buffer, ClusterSize)) {
		std::cout << "ERROR" << std::endl;
		std::cout << 'a' + blockNo << "    " << buffer[0] << std::endl;
	}
}

void PartitionWriter::operation(MemoryInterface *memoryModule, unsigned long blockNo, char *buffer) {
	memoryModule->write(blockNo, buffer);
}

int tests::PartitionCacheCompareAndTest() {
	std::cout << "MODULE CAPACITY?" << std::endl;
	int numOfClusters;
	std::cin >> numOfClusters;

	std::cout << "CACHE NUM OF ENTRIES, SIZE OF ENTRIE?" << std::endl;
	int numOfEntries, sizeOfEntrie;
	std::cin >> numOfEntries;
	std::cin >> sizeOfEntrie;

	std::cout << "NUM OF READERS?" << std::endl;
	int numOfReaders = 0;
	std::cin >> numOfReaders;

	std::cout << "START BLOCK?" << std::endl;
	unsigned long readerStartBlock = 0;
	std::cin >> readerStartBlock;

	std::cout << "NUM OF BLOCKS?" << std::endl;
	unsigned long readerNumOfBlocks = 0;
	std::cin >> readerNumOfBlocks;
	
	std::cout << "NUM OF WRITERS IS 1" << std::endl;
	int numOfWriters = 1;

	std::cout << "START BLOCK?" << std::endl;
	unsigned long writerStartBlock = 0;
	std::cin >> writerStartBlock;

	std::cout << "NUM OF BLOCKS?" << std::endl;
	unsigned long writerNumOfBlocks = 0;
	std::cin >> writerNumOfBlocks;

	PartitionThread **readers = new PartitionThread*[numOfReaders];
	PartitionThread *writer = nullptr;

	std::cout << "testsING WITHOUT CACHE MEMORY OF ANY KIND" << std::endl;

	{//da bi mogao iste nazive promenljivih da koristim
		clock_t startClocks = clock();
		MemoryInterface *memoryModule = new ModuleSimulator(numOfClusters);
		for (int i = 0; i < numOfReaders; i++) {
			readers[i] = new PartitionReader(readerStartBlock, readerNumOfBlocks, memoryModule);
			readers[i]->start();
		}
		writer = new PartitionWriter(writerStartBlock, writerNumOfBlocks, memoryModule);
		writer->start();
		std::cout << "WAITING FOR THREADS" << std::endl;
		for (int i = 0; i < numOfReaders; i++) {
			delete readers[i];
		}
		delete writer;
		delete memoryModule;
		clock_t endClocks = clock();
		double ms = double(endClocks - startClocks) / CLOCKS_PER_SEC * 1000;
		std::cout << "TIME: " << ms << std::endl;
	}


	std::cout << "TESTING WITH PARTITION CACHE" << std::endl;
	{
		clock_t startClocks = clock();
		MemoryInterface *memorySimulator = new ModuleSimulator(numOfClusters);
		MemoryInterface *memoryModule = new ThreadSafeBlockCache(numOfEntries, sizeOfEntrie, 50, memorySimulator);
		for (int i = 0; i < numOfReaders; i++) {
			readers[i] = new PartitionReader(readerStartBlock, readerNumOfBlocks, memoryModule);
			readers[i]->start();
		}
		writer = new PartitionWriter(writerStartBlock, writerNumOfBlocks, memoryModule);
		writer->start();
		std::cout << "WAITING FOR THREADS" << std::endl;
		for (int i = 0; i < numOfReaders; i++) {
			delete readers[i];
		}
		delete writer;
		delete memoryModule;
		delete memorySimulator;
		clock_t endClocks = clock();
		double ms = double(endClocks - startClocks) / CLOCKS_PER_SEC * 1000;
		std::cout << "TIME: " << ms << std::endl;
	}

	delete[] readers;
	return 0;
}

BufferedWriterThread::BufferedWriterThread(int _numOfOperations, int _lineSize, MemoryInterface *_memoryModule) : numOfOperations(_numOfOperations), lineSize(_lineSize), memoryModule(_memoryModule) {

}

void BufferedWriterThread::run() {
	char *buffer = new char[ClusterSize];
	char *tmpBuffer = new char[ClusterSize];
	for (int i = 0; i < numOfOperations; i++) {
		for (int j = 0; j < lineSize; j++) {
			memoryModule->read(0 + j, buffer);
			memset(tmpBuffer, 'a' + j, ClusterSize);
			/*if (i!= 0 && memcmp(buffer, tmpBuffer, ClusterSize)) {
				std::cout << "ERORR" << std::endl;
			}*/
		}
		for (int i = 0; i < 10000; i++)
			for (int j = 0; j < 2000; j++);//OVO JE DA BI CISTO DOBIO UTISAK DA LI SE NESTO DOBIJA OVIM, pa je uzeto da ova nit aradi nesto sa ovim buffer 10ms
		for (int j = 0; j < lineSize; j++) {
			memset(buffer, 'a' + j, ClusterSize);
			memoryModule->write(0 + j, buffer);
		}
	}
	delete[] buffer;
	delete[] tmpBuffer;
}

BufferedWriterThread::~BufferedWriterThread() {
	waitToComplete();
}

int tests::BufferedWriterTest() {
	std::cout << "MODULE CAPACITY" << std::endl;
	int numOfClusters = 0;
	std::cin >> numOfClusters;

	std::cout << "LINE SIZE?" << std::endl;
	int lineSize = 0;
	std::cin >> lineSize;

	std::cout << "NUM OF OPERATIONS?" << std::endl;
	int numOfOperations = 0;
	std::cin >> numOfOperations;

	std::cout << "NUM OF THREADS?" << std::endl;
	int numOfThreads = 0;
	std::cin >> numOfThreads;

	BufferedWriterThread **writers = new BufferedWriterThread*[numOfThreads];
	BufferedWriter **bufferedWriters = new BufferedWriter*[numOfThreads];
	
	std::cout << "TESTIING WITHOUT BUFFERED WRITERS" << std::endl;
	{
		clock_t startClocks = clock();
		ModuleSimulator *moduleSimulator = new ModuleSimulator(numOfClusters);
		for (int i = 0;i < numOfThreads; i++) {
			writers[i] = new BufferedWriterThread(numOfOperations, lineSize, moduleSimulator);
			writers[i]->start();
		}
		std::cout << "MAIN THREAD WAITING" << std::endl;
		for (int i = 0; i < numOfThreads; i++) {
			delete writers[i];
		}
		delete moduleSimulator;
		clock_t endClocks = clock();
		double ms = (double)(endClocks - startClocks) / CLOCKS_PER_SEC * 1000;
		std::cout << "TIME: " << ms << std::endl;
	}

	std::cout << "TESTINGING WITH BUFFERED WRITERS" << std::endl;
	{
		clock_t startClocks = clock();
		ModuleSimulator *moduleSimulator = new ModuleSimulator(numOfClusters);
		for (int i = 0;i < numOfThreads; i++) {
			bufferedWriters[i] = new BufferedWriter(lineSize, moduleSimulator);
			writers[i] = new BufferedWriterThread(numOfOperations, lineSize, bufferedWriters[i]);
			writers[i]->start();
		}
		std::cout << "MAIN THREAD WAITING" << std::endl;
		for (int i = 0; i < numOfThreads; i++) {
			delete writers[i];
			delete bufferedWriters[i];
		}
		delete moduleSimulator;
		clock_t endClocks = clock();
		double ms = (double)(endClocks - startClocks) / CLOCKS_PER_SEC * 1000;
		std::cout << "TIME: " << ms << std::endl;
	}

	delete[] writers;
	delete[] bufferedWriters;
	std::cout << "MAIN THREAD DONE" << std::endl;
	return 0;
}

int tests::QueueTest() {
	std::cout << "NUM OF ITERATIONS?" << std::endl;
	int numOfIterations = 0;
	std::cin >> numOfIterations;
	
	Queue<int> *queue = new Queue<int>();
	int *a = new int[numOfIterations];
	for (int i = 0; i < numOfIterations; i++) {
		queue->push(i);
		a[i] = i;
	}
	std::cout << "CHECKING" << std::endl;

	for (int i = 0; i < numOfIterations; i++) {
		if (queue->pop() != a[i]) {
			std::cout << "ERROR" << std::endl;
		}
	}

	delete queue;
	delete[] a;

	return 0;
}

int tests::KernelFileTest() {
	std::cout << "MODULE CAPACITY" << std::endl;
	int numOfClusters = 0;
	std::cin >> numOfClusters;

	std::cout << "NUM OF DATA CLUSTERS?" << std::endl;
	int numOfDataClusters = 0;
	std::cin >> numOfDataClusters;

	ModuleSimulator *memoryModule = new ModuleSimulator(numOfClusters);
	PartitionMemoryManagmentUnit *partitionMemoryManagmentUnit = new PartitionMemoryManagmentUnit(memoryModule->getNumOfCluster(), memoryModule);


	KernelFile *file = new WritableKernelFile(0, 0, 0, memoryModule, partitionMemoryManagmentUnit);

	for (int i = 0; i < numOfDataClusters; i++) {
		char buffer[ClusterSize];
		memset(buffer, i % 255, ClusterSize);
		file->write(ClusterSize, buffer);
	} 

	delete file;

	file = new ReadableKernelFile(numOfDataClusters * ClusterSize, 2, memoryModule);

	for (int i = 0; i < numOfDataClusters; i++) {
		char buffer[ClusterSize], tmpBuffer[ClusterSize];
		memset(buffer, i % 255, ClusterSize);
		file->read(ClusterSize, tmpBuffer);
		if (memcmp(buffer, tmpBuffer, ClusterSize)) {
			std::cout << "ERROR" << std::endl;
		}
	}

	delete file;

	file = new WritableKernelFile(numOfDataClusters * ClusterSize, 0, 2, memoryModule, partitionMemoryManagmentUnit);
	
	file->seek(numOfDataClusters * ClusterSize);
	file->seek(numOfDataClusters * ClusterSize - 1);
	file->truncate();
	file->seek(numOfDataClusters * ClusterSize - ClusterSize + 1);
	file->truncate();
	file->seek(-1);
	file->seek(0);
	file->truncate();

	delete file;

	delete partitionMemoryManagmentUnit;
	delete memoryModule;
	return 0;
}

KernelFileThread::KernelFileThread(MemoryInterface *_memoryModule, MemoryManagmentUnit *_partitionMemoryManagment, int _numOfDataClusters) : memoryModule(_memoryModule), partitionMemoryManagment(_partitionMemoryManagment), numOfDataClusters(_numOfDataClusters) {

}

KernelFileThread::~KernelFileThread() {
	waitToComplete();
}

void KernelFileThread::run() {
	KernelFile *file = new WritableKernelFile(0, 0, 0, memoryModule, partitionMemoryManagment);
	int numOfErrors = 0;
	for (int i = 0; i < numOfDataClusters; i++) {
		char buffer[ClusterSize];
		memset(buffer, i % 256, ClusterSize);
		file->write(ClusterSize, buffer);
	}

	unsigned long indexCluster = file->getIndexCluster();
	delete file;

	file = new ReadableKernelFile(numOfDataClusters * ClusterSize, indexCluster, memoryModule);
	
	for (int i = 0; i < numOfDataClusters; i++) {
		char buffer[ClusterSize], tmpBuffer[ClusterSize];
		memset(tmpBuffer, i % 256, ClusterSize);
		file->read(ClusterSize, buffer);
		if (memcmp(buffer, tmpBuffer, ClusterSize)) {
			std::cout << "CLUSTER NO: " << i << std::endl;
		}
	}

	file->seek(0);
	file->truncate();
	delete file;
} 

int tests::MultiThreadKernelFileTest() {
	std::cout << "MODULE CAPACITY" << std::endl;
	int numOfClusters = 0;
	std::cin >> numOfClusters;

	std::cout << "NUM OF DATA CLUSTERS?" << std::endl;
	int numOfDataClusters = 0;
	std::cin >> numOfDataClusters;

	std::cout << "NUM OF THREADS?" << std::endl;
	int numOfThreads = 0;
	std::cin >> numOfThreads;
	
	KernelFileThread **threads = new KernelFileThread*[numOfThreads];

	{
		std::cout << "TESTING WITHOUT PARTITION CAHCE" << std::endl;
		clock_t start = clock();

		ModuleSimulator *memoryModule = new ModuleSimulator(numOfClusters);
		PartitionMemoryManagmentUnit *partitionMemoryManagmentUnit = new PartitionMemoryManagmentUnit(memoryModule->getNumOfCluster(), memoryModule);

		for (int i = 0; i < numOfThreads; i++) {
			threads[i] = new KernelFileThread(memoryModule, partitionMemoryManagmentUnit, numOfDataClusters);
			threads[i]->start();
		}
		for (int i = 0; i < numOfThreads; i++) {
			delete threads[i];
		}

		delete partitionMemoryManagmentUnit;
		delete memoryModule;

		clock_t end = clock();
		std::cout << "TIME NEEDED: " << end - start << std::endl;
	}

	{
		std::cout << "TESTING WITH PARTITION CAHCE" << std::endl;
		clock_t start = clock();

		ModuleSimulator *memoryModuleSimulator = new ModuleSimulator(numOfClusters);
		ThreadSafeBlockCache * partitionCache = new ThreadSafeBlockCache(PARTITION_CACHE_NUM_OF_ENTRIES, PARTITION_CACHE_SIZE_OF_ENTRY, PARTITION_CACHE_LRU_REQUEST_BUFFER_SIZE, memoryModuleSimulator);
		PartitionMemoryManagmentUnit *partitionMemoryManagmentUnit = new PartitionMemoryManagmentUnit(memoryModuleSimulator->getNumOfCluster(), partitionCache);

		for (int i = 0; i < numOfThreads; i++) {
			threads[i] = new KernelFileThread(partitionCache, partitionMemoryManagmentUnit, numOfDataClusters);
			threads[i]->start();
		}
		for (int i = 0; i < numOfThreads; i++) {
			delete threads[i];
		}

		delete partitionMemoryManagmentUnit;
		delete partitionCache;
		delete memoryModuleSimulator;

		clock_t end = clock();
		std::cout << "TIME NEEDED: " << end - start << std::endl;
	}

	delete[] threads;
	return 0;
}

int tests::BitVectorTest() {
	std::cout << "CAPCITY?" << std::endl;
	int capacity = 0;
	std::cin >> capacity;

	char *vector = new char[capacity];
	memset(vector, 0, capacity);

	for (int i = 0; i < capacity * 8; i++) {
		utilities::setBitInBitvector(vector, i, 1);
	}

	for (int i = 0; i < capacity * 8; i++) {
		if (utilities::getBitFromBitvector(vector, i) != 1) {
			std::cout << "ERROR" << std::endl;
		}
	}

	for (int i = 0; i < capacity * 8; i++) {
		utilities::setBitInBitvector(vector, i, 0);
	}

	for (int i = 0; i < capacity * 8; i++) {
		if (utilities::getBitFromBitvector(vector, i) == 1) {
			std::cout << "ERROR" << std::endl;
		}
	}

	delete[] vector;
	return 0;
}

void printString(const char *string, int len) {
	for (int i = 0; i < len; i++) {
		std::cout << string[i];
	}
	std::cout << std::endl;
}

int tests::BasicPartitionFunctionTest() {
	Partition *partition = new Partition("p1.ini");

	char p = FS::mount(partition);
	std::cout << "PARTICIJA MONTIRANA I FORMATIRA" << std::endl;

	FS::format(p);

	FILE *f = fopen("ulaz.dat", "rb");
	if (f == 0) {
		std::cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << std::endl;
		system("PAUSE");
		return 0;//exit program
	}
	char *ulazBuffer = new char[32 * 1024 * 1024];//32MB
	unsigned long ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, f);
	std::cout << "ULASSIZE: " << ulazSize << std::endl;
	fclose(f);

	char fname[] = "1:\\ulaz.dat";
	fname[0] = p;

	File *file = FS::open(fname, 'w');
	for (unsigned long int i = 0; i < ulazSize; i++) {
		file->write(1, ulazBuffer + i);
	}

	std::cout << "FILE SIZE: " << file->getFileSize() << std::endl;

	delete file;

	file = FS::open(fname, 'r');
	FILE *output = fopen("IZLAZ.dat", "wb");
	while (!file->eof()) {
		char data = 0;
		file->read(1, &data);
		fwrite(&data, sizeof(char), 1, output);
	}
	delete file;
	fclose(output);

	delete[] ulazBuffer;
	FS::unmount(p);

	p = FS::mount(partition);
	Directory dir;
	EntryNum num = FS::readRootDir(p, 0, dir);
	for (int i = 0; i < num; i++) {
		printString(dir[i].name, 8);
		printString(dir[i].ext, 3);
		std::cout << dir[i].indexCluster << std::endl << dir[i].size << std::endl;
	}

	file = FS::open(fname, 'r');
	FILE *output1 = fopen("IZLAZ1.dat", "wb");
	while (!file->eof()) {
		char data = 0;
		file->read(1, &data);
		fwrite(&data, sizeof(char), 1, output);
	}
	delete file;
	fclose(output1);


	FS::unmount(p);
	delete partition;
	return 0;
}