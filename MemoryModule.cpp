#include "MemoryModule.h"
#include "part.h"
#include <cmath>

int utilities::getBitFromBitvector(char *vector, int bitNo) {
		int entry = bitNo / (sizeof(char) * 8);
		return ((vector[entry] >> (bitNo % (8 * sizeof(char)))) & 1);
	}

void utilities::setBitInBitvector(char *vector, int bitNo, int value) {
	int entry = bitNo / (sizeof(char) * 8);
		char mask = 1 << (bitNo % (8 * sizeof(char)));
		if (value == 0) {
			vector[entry] &= (~mask);
		} else {
			vector[entry] |= mask;
		}
	}

MemoryInterface::~MemoryInterface() {
	
}

MemoryModule::~MemoryModule() {

}

CacheMemory::CacheMemory(int _numOfEntries, int _sizeOfEntry, SwapAlgorithm *_swapAlgorithm, MemoryInterface *_memoryModule) : numOfEntries(_numOfEntries), sizeOfEntry(_sizeOfEntry), swapAlgorithm(_swapAlgorithm), memoryModule(_memoryModule) {
	data = new char**[numOfEntries];
	for (int i = 0; i < numOfEntries; i++) {
		data[i] = new char*[sizeOfEntry];
		for (int j = 0; j < sizeOfEntry; j++) {
			data[i][j] = new char[ClusterSize];
		}
	}
	//allocata V and D bitvectors
	Vcapacity = static_cast<int>(ceil(static_cast<double>(numOfEntries) / (sizeof(char) * 8)));
	V = new char[Vcapacity];
	memset(V, 0, Vcapacity);
	
	Dcapacity = static_cast<int>(ceil(static_cast<double>(numOfEntries * sizeOfEntry) / (sizeof(char) * 8)));
	D = new char[Dcapacity];
	memset(D, 0, Dcapacity);
	//allocate tag memory;
	tagMemory = new TagMemory(numOfEntries);
}

int CacheMemory::getNewBlock(unsigned long blockNo) {
	//get swap entry
	int swapEntry = swapAlgorithm->getSwapEntry();
	//check if block in swap entry was modified
	if (utilities::getBitFromBitvector(V, swapEntry) == 1) {
		for (int i = 0; i < sizeOfEntry; i++) {
			if (utilities::getBitFromBitvector(D, swapEntry * sizeOfEntry + i) == 1) {
				memoryModule->write(tagMemory->get(swapEntry) + i, data[swapEntry][i]);
			}
		}
	}
	//first get tag
	unsigned long tag = (blockNo / sizeOfEntry) * sizeOfEntry;
	//get blocks 
	for (int i = 0; i < sizeOfEntry; i++) {
		memoryModule->read(tag + i, data[swapEntry][i]);
	}
	//update tag memory
	tagMemory->write(swapEntry, tag);
	//update V and D bits
	utilities::setBitInBitvector(V, swapEntry, 1);
	for (int i = 0; i < sizeOfEntry; i++) {
		utilities::setBitInBitvector(D, swapEntry * sizeOfEntry + i, 0);
	}
	//return block entry
	return swapEntry * sizeOfEntry + (blockNo % sizeOfEntry);
}

int CacheMemory::preOperation(unsigned long blockNo) {
	//get entry from tag memory
	int entry = tagMemory->compare(blockNo, sizeOfEntry);
	//check if block is in cache
	if (entry == -1 || utilities::getBitFromBitvector(V, entry / sizeOfEntry) == 0) {
		entry = getNewBlock(blockNo);
	}
	swapAlgorithm->update(entry / sizeOfEntry);

	return entry;
}

void CacheMemory::flush() {
	//return all modified blocks
	for (int i = 0; i < numOfEntries; i++) {
		for (int j = 0; j < sizeOfEntry; j++) {
			if (utilities::getBitFromBitvector(D, i * sizeOfEntry + j) == 1) {
				memoryModule->write(tagMemory->get(i) + j, data[i][j]);
			}
		}
	}
	//invalidate V and D bit
	for (int i = 0; i < numOfEntries; i++) {
		utilities::setBitInBitvector(V, i, 0);
		for (int j = 0; j < sizeOfEntry; j++) {
			utilities::setBitInBitvector(D, i * sizeOfEntry + j, 0);
		}
	}
}

CacheMemory::~CacheMemory() {
	//first flush()
	flush();
	//free data
	for (int i = 0; i < numOfEntries; i++) {
		for (int j = 0; j < sizeOfEntry; j++) {
			delete[] data[i][j];
		}
		delete[] data[i];
	}
	delete[] data;
	data = nullptr;
	//free D and V bitvectors
	delete[] V;
	delete[] D;
	//free tag memory
	delete tagMemory;
	//free swap algorithm
	delete swapAlgorithm;
}

ByteCacheMemory::ByteCacheMemory(int _numOfEntries, int _sizeOfEntry, MemoryInterface *_memoryModule) : ByteCacheMemory(_numOfEntries, _sizeOfEntry, new LRU(_numOfEntries), _memoryModule) {

}

ByteCacheMemory::ByteCacheMemory(int _numOfEntries, int _sizeOfEntry, SwapAlgorithm *_swapAlgorithm, MemoryInterface *_memoryModule) : CacheMemory(_numOfEntries, _sizeOfEntry, _swapAlgorithm, _memoryModule), lastAccessedBlock(~0), lastAccessedEntry(~0) {
	
}

int ByteCacheMemory::read(unsigned long position, char *buffer) {
	unsigned long blockNo = position / ClusterSize;
	if (lastAccessedBlock != blockNo) {
		lastAccessedEntry = preOperation(blockNo);
		lastAccessedBlock = blockNo;
	}
	(*buffer) = data[lastAccessedEntry / sizeOfEntry][lastAccessedEntry % sizeOfEntry][position % ClusterSize];
	return 0;
}

int ByteCacheMemory::write(unsigned long position, const char *buffer) {
	unsigned long blockNo = position / ClusterSize;
	if (lastAccessedBlock != blockNo) {
		lastAccessedEntry = preOperation(blockNo);
		lastAccessedBlock = blockNo;
	}
	data[lastAccessedEntry / sizeOfEntry][lastAccessedEntry % sizeOfEntry][position % ClusterSize] = (*buffer);
	utilities::setBitInBitvector(D, lastAccessedEntry, 1);
	return 0;
}

BlockCacheMemory::BlockCacheMemory(int _numOfEntries, int _sizeOfEntry, MemoryInterface *_memoryModule) : BlockCacheMemory(_numOfEntries, _sizeOfEntry, new LRU(_numOfEntries), _memoryModule) {

}

BlockCacheMemory::BlockCacheMemory(int _numOfEntries, int _sizeOfEntry, SwapAlgorithm *_swapAlgorithm, MemoryInterface *_memoryModule) : CacheMemory(_numOfEntries, _sizeOfEntry, _swapAlgorithm, _memoryModule) {

}

int BlockCacheMemory::read(unsigned long blockNo, char *buffer) {
	int entry = preOperation(blockNo);
	memcpy(buffer, data[entry / sizeOfEntry][entry % sizeOfEntry], ClusterSize);
	return 0;
}

int BlockCacheMemory::write(unsigned long blockNo, const char *buffer) {
	int entry = preOperation(blockNo);
	memcpy(data[entry / sizeOfEntry][entry % sizeOfEntry], buffer, ClusterSize);
	utilities::setBitInBitvector(D, entry, 1);
	return 0;
}

ThreadSafeBlockCache::ThreadSafeBlockCache(int _numOfEntries, int _sizeOfEntrie, int _lruRequestBufferCapacity, MemoryInterface *_memoryModule) : BlockCacheMemory(_numOfEntries, _sizeOfEntrie, new ThreadSafeLRU(_numOfEntries, _lruRequestBufferCapacity), _memoryModule) {
	monitor = new UpgradeMonitor();
}

ThreadSafeBlockCache::~ThreadSafeBlockCache() {
	delete monitor;
}

int ThreadSafeBlockCache::read(unsigned long blockNo, char *buffer) {
	monitor->startRead();
	int ret = BlockCacheMemory::read(blockNo, buffer);
	monitor->endRead();
	return ret;
}

int ThreadSafeBlockCache::write(unsigned long blockNo, const char *buffer) {
	monitor->startRead();
	int ret = BlockCacheMemory::write(blockNo, buffer);
	monitor->endRead();
	return ret;
}

int ThreadSafeBlockCache::getNewBlock(unsigned long blockNo) {
	monitor->startWrite();
	unsigned long tag = (blockNo / sizeOfEntry) * sizeOfEntry;
	int entry = tagMemory->compare(tag, sizeOfEntry);
	if (entry == -1) {
		entry = CacheMemory::getNewBlock(blockNo);
	}
	monitor->endWrite();
	return entry;
}

void ThreadSafeBlockCache::flush() {
	monitor->startRead();
	monitor->startWrite();
	CacheMemory::flush();
	monitor->endWrite();
	monitor->endRead();
}

WriteRequest::WriteRequest(unsigned long _blockNo, const char *_buffer) : blockNo(_blockNo), buffer(_buffer) {

}

WriteRequest::~WriteRequest() {
	delete[] buffer;
}

WriteRequestHandler::WriteRequestHandler(BlockBuffer *_buffer, MemoryInterface *_memoryModule, ModuleMonitor *_monitor) : buffer(_buffer), memoryModule(_memoryModule), monitor(_monitor) {

}

WriteRequestHandler::~WriteRequestHandler() {
	waitToComplete();
}

void WriteRequestHandler::handleRequest() {
	monitor->startWrite();
	WriteRequest *request = buffer->get();
	memoryModule->write(request->blockNo, request->buffer);
	delete request;
	//this thread is responsible for block and request
	monitor->endWrite();
}

BufferedWriter::BufferedWriter(int _lineSize, MemoryInterface *_memoryModule) : memoryModule(_memoryModule) {
	buffer = new BlockBuffer(_lineSize);
	monitor = new ModuleMonitor(_lineSize);
	handler = new WriteRequestHandler(buffer, memoryModule, monitor);
	handler->start();
}

BufferedWriter::~BufferedWriter() {
	handler->stop();
	delete handler;
	delete monitor;
	delete buffer;
}

int BufferedWriter::read(unsigned long blockNo, char *buffer) {
	monitor->startRead();
	int ret = memoryModule->read(blockNo, buffer);
	monitor->endRead();
	return ret;
}

int BufferedWriter::write(unsigned long blockNo, const char *bbuffer) {
	char *tmpBuffer = new char[ClusterSize];
	memcpy(tmpBuffer, bbuffer, ClusterSize);
	WriteRequest *request = new WriteRequest(blockNo, tmpBuffer);
	buffer->put(request);
	monitor->putBlock();
	handler->putRequest();
	return 0;
}

PartitionWrapper::PartitionWrapper(Partition *_partition, unsigned long _numOfClusters) : partition(_partition), numOfClusters(_numOfClusters) {
	
}

int PartitionWrapper::read(unsigned long clusterNo, char *buffer) {
	int ret = 1;
	if (clusterNo > numOfClusters) {
		memset(buffer, 20, ClusterSize);
		ret = 0;
	} else {
		ret = partition->readCluster(clusterNo, buffer);
	}
	return ret;
}

int PartitionWrapper::write(unsigned long clusterNo, const char *buffer) {
	int ret = 1;
	if (clusterNo > numOfClusters) {
		ret = 0; 
	} else {
		ret = partition->writeCluster(clusterNo, buffer);
	}
	return ret;
}

MemoryMapper::MemoryMapper(MemoryInterface *_memoryModule, CacheMemory *_indexCache, unsigned long _indexCluster) : memoryModule(_memoryModule), indexCache(_indexCache), indexCluster(_indexCluster) {

}

int MemoryMapper::read(unsigned long blockNo, char *buffer) {
	return memoryModule->read(LogToPh(blockNo), buffer);
}

int MemoryMapper::write(unsigned long blockNo, const char *buffer) {
	return memoryModule->write(LogToPh(blockNo), buffer);
}

void MemoryMapper::setIndexCluster(unsigned long _indexCluster) {
	indexCluster = _indexCluster;
}

unsigned long MemoryMapper::LogToPh(unsigned long logBlockNo) {
	unsigned long offset = 0;
	if (logBlockNo < 256) {
		offset = indexCluster * ClusterSize + logBlockNo * sizeof(unsigned long);
	} else {
		int entry = (logBlockNo - 256) / 512;
		unsigned long indexClusterNo = 0;
		char *indexClusterNoPointer = reinterpret_cast<char*>(&indexClusterNo);
		unsigned long indexClusterOffset = indexCluster * ClusterSize + (256 + entry) * sizeof(unsigned long);
		for (int i = 0; i < sizeof(unsigned long); i++) {
			indexCache->read(indexClusterOffset + i, indexClusterNoPointer + i);
		}
		offset = indexClusterNo * ClusterSize + (logBlockNo - 256) % 512 * sizeof(unsigned long);
	}

	unsigned long phBlockNo = 0;
	char *phBlockNoPointer = reinterpret_cast<char*>(&phBlockNo);
	for (int i = 0; i < sizeof(unsigned long); i++) {
		indexCache->read(offset + i, phBlockNoPointer + i);
	}
	return phBlockNo;
}

DataMemoryChecker::DataMemoryChecker(MemoryInterface *_memoryModule) : memoryModule(_memoryModule) {

}

int DataMemoryChecker::read(unsigned long blockNo, char *buffer) {
	if (blockNo == 0) {
		memset(buffer, 15, ClusterSize);//izbrisi ovo ti je trebalo za testiranje
		return 1;
	}
	return memoryModule->read(blockNo, buffer);
}

int DataMemoryChecker::write(unsigned long blockNo, const char *buffer) {
	if (blockNo == 0) {
		return 1;
	}
	return memoryModule->write(blockNo, buffer);
}

IndexMemoryChecker::IndexMemoryChecker(MemoryInterface *_memoryModule) : memoryModule(_memoryModule) {

}

int IndexMemoryChecker::read(unsigned long clusterNo, char *buffer) {
	if (clusterNo == 0) {
		memset(buffer, 0, ClusterSize);
	} else {
		memoryModule->read(clusterNo, buffer);
	}
	return 1;
}

int IndexMemoryChecker::write(unsigned long clusterNo, const char *buffer) {
	if (clusterNo != 0) {
		memoryModule->write(clusterNo, buffer);
	}
	return 1;
}