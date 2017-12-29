#include "MemoryManagment.h"
#include "Queue.h"
#include "MemoryModule.h"
#include "part.h"


MemoryManagmentUnit::~MemoryManagmentUnit() {

}

PartitionMemoryManagmentUnit::PartitionMemoryManagmentUnit(unsigned long _numOfClustersInPartition, MemoryInterface *_memoryModule) : memoryModule(_memoryModule), numOfClustersInPartition(_numOfClustersInPartition), isChanged(false) {
	numOfClusters = (numOfClustersInPartition + ClusterSize - 1) / ClusterSize;
	inMemory = -1;
	bitVectorBuffer = nullptr;
	InitializeCriticalSection(&criticalSection);
}

void PartitionMemoryManagmentUnit::flush() {
	EnterCriticalSection(&criticalSection);
	if (isChanged && bitVectorBuffer != nullptr) {
		memoryModule->write(inMemory, bitVectorBuffer);
	}
	LeaveCriticalSection(&criticalSection);
}

PartitionMemoryManagmentUnit::~PartitionMemoryManagmentUnit() {
	flush();
	if (bitVectorBuffer != nullptr) {
		delete[] bitVectorBuffer;
	}
	DeleteCriticalSection(&criticalSection);
}

void PartitionMemoryManagmentUnit::read(unsigned long clusterNo) {
	if (inMemory != clusterNo) {
		if (bitVectorBuffer == nullptr) {
			bitVectorBuffer = new char[ClusterSize];
		}
		if (isChanged && bitVectorBuffer != nullptr) {
			memoryModule->write(inMemory, bitVectorBuffer);
			isChanged = false;
		}
		inMemory = clusterNo;
		memoryModule->read(inMemory, bitVectorBuffer);
	}
}

unsigned long PartitionMemoryManagmentUnit::findFreeCluster() {
	unsigned long clusterNo = 0;
	if (inMemory == -1) {
		read(0);
	}
	for (int i = 0; i < numOfClusters; i++) {
		int boundary = 0;
		if (inMemory < numOfClusters || numOfClustersInPartition % ClusterSize == 0) {
			boundary = ClusterSize; 
		} else {
			boundary = numOfClustersInPartition % ClusterSize;
		}
		for (int j = 0; j < boundary; j++) {
			if (inMemory == 0 && j < numOfClusters) continue;

			if (utilities::getBitFromBitvector(bitVectorBuffer, j) == 0) {
				clusterNo = inMemory * ClusterSize + j;
				break;
			}
		}
		if (clusterNo != 0) {
			break;
		}
		read((inMemory + 1) % numOfClusters);
	}
	return clusterNo;
}

unsigned long PartitionMemoryManagmentUnit::allocate() {
	EnterCriticalSection(&criticalSection);
	unsigned long clusterNo = 0;
	clusterNo = findFreeCluster();
	if (clusterNo != 0) {
		utilities::setBitInBitvector(bitVectorBuffer, clusterNo % ClusterSize, 1);
		isChanged = true;
	}
	LeaveCriticalSection(&criticalSection);
	return clusterNo;
}

void PartitionMemoryManagmentUnit::free(unsigned long clusterNo) {
	EnterCriticalSection(&criticalSection);
	read(clusterNo / ClusterSize);
	utilities::setBitInBitvector(bitVectorBuffer, clusterNo % ClusterSize, 0);
	LeaveCriticalSection(&criticalSection);
}

void PartitionMemoryManagmentUnit::format() {
	if (bitVectorBuffer == nullptr) {
		bitVectorBuffer = new char[ClusterSize];
	}
	memset(bitVectorBuffer, 0, ClusterSize);
	for (int i = 1; i < numOfClusters; i++) {
		memoryModule->write(i, bitVectorBuffer);
	}
	for (int i = 0; i < numOfClusters; i++) {
		utilities::setBitInBitvector(bitVectorBuffer, i, 1);
	}
	memoryModule->write(0, bitVectorBuffer);
	inMemory = -1;
	isChanged = false;
}

const unsigned long FileMemoryManagmentUnit::MAX_FILE_CLUSTERS = ClusterSize / 8 + ClusterSize / 8 * ClusterSize / 4;

FileMemoryManagmentUnit::FileMemoryManagmentUnit(CacheMemory *_indexCache, MemoryManagmentUnit *_partitionMemoryManagment, unsigned long _numOfDataClusters, unsigned long _indexCluster) : indexCache(_indexCache), partitionMemoryManagmentUnit(_partitionMemoryManagment), numOfDataClusters(_numOfDataClusters), indexCluster(_indexCluster) {
	numOfIndexClusters = numOfDataClusters == 0 ? 0 : numOfDataClusters <= 256 ? 1 : 2 + (numOfDataClusters - 256 - 1) / 512;//ako se fajl kreira prazan onda _indexCluster = 0
}

FileMemoryManagmentUnit::~FileMemoryManagmentUnit() {

}

void FileMemoryManagmentUnit::write(int clusterNo, int entry, unsigned long value) {
	unsigned long indexClusterNo = 0;
	if (clusterNo == 0) {
		indexClusterNo = indexCluster;
	} else {
		char *indexClusterNoPointer = reinterpret_cast<char*>(&indexClusterNo);
		for (int i = 0; i < sizeof(unsigned long); i++) {
			indexCache->read(indexCluster * ClusterSize + (256 + clusterNo - 1) * sizeof(unsigned long) + i, indexClusterNoPointer + i);
		}
	}
	unsigned long offset = indexClusterNo * ClusterSize + entry * sizeof(unsigned long);
	char *valuePointer = reinterpret_cast<char*>(&value);
	for (int i = 0; i < sizeof(unsigned long); i++) {
		indexCache->write(offset + i, valuePointer + i);
	}
} 

unsigned long FileMemoryManagmentUnit::read(int clusterNo, int entry) {
	unsigned long indexClusterNo = 0;
	if (clusterNo == 0) {
		indexClusterNo = indexCluster;
	}
	else {
		char *indexClusterNoPointer = reinterpret_cast<char*>(&indexClusterNo);
		for (int i = 0; i < sizeof(unsigned long); i++) {
			indexCache->read(indexCluster * ClusterSize + (256 + clusterNo - 2) * sizeof(unsigned long) + i, indexClusterNoPointer + i);
		}
	}
	unsigned long offset = indexClusterNo * ClusterSize + entry * sizeof(unsigned long);
	unsigned long value = 0;
	char *valuePointer = reinterpret_cast<char*>(&value);
	for (int i = 0; i < sizeof(unsigned long); i++) {
		indexCache->read(offset + i, valuePointer + i);
	}
	return value;
}

unsigned long FileMemoryManagmentUnit::allocateNewIndexCluster() {
	unsigned long clusterNo = partitionMemoryManagmentUnit->allocate();
	int ret = 0;
	if (clusterNo != 0) {
		if (numOfIndexClusters == 0) {//this first leve index cluster
			indexCluster = clusterNo;
			ret = 2;
		} else {//this is second level index cluster and we need ot write that in index cluster
			write(0, 256 + numOfIndexClusters - 1, clusterNo);
			ret = 1;
		}
		char fillChar = 0;
		for (int i = 0; i < ClusterSize; i++) {
			indexCache->write(clusterNo * ClusterSize + i, &fillChar);
		}
		numOfIndexClusters++;
	}
	return ret;
}

void FileMemoryManagmentUnit::freeIndexCluster() {//frees last index cluster
	unsigned long clusterNo = 0;
	if (numOfIndexClusters > 1) {
		clusterNo = read(0, 256 + numOfIndexClusters - 2);
		write(0, 256 + numOfIndexClusters - 2, 0);
	} else {
		clusterNo = indexCluster;
		indexCluster = 0;
	}
	numOfIndexClusters--;
	partitionMemoryManagmentUnit->free(clusterNo);
}

unsigned long FileMemoryManagmentUnit::allocate() {
	if (numOfDataClusters > MAX_FILE_CLUSTERS) {// file is at max size
		return 0;
	}
	
	int ret = 0;

	unsigned long newNumberOfDataClusters = numOfDataClusters + 1;
	int currentNumberOfIndexCluster = newNumberOfDataClusters <= 256 ? 1 : 2 + (newNumberOfDataClusters - 256 - 1) / 512;
	if (numOfIndexClusters < currentNumberOfIndexCluster) {
		if (allocateNewIndexCluster() == 0) {
			freeIndexCluster();
			return 0;
		}
		ret = 2;
	}

	unsigned long clusterNo = partitionMemoryManagmentUnit->allocate();
	if (clusterNo == 0) {
		if (ret == 2) {
			freeIndexCluster();
			return 0;
		}
	} else if (ret == 0) {
		ret = 1;
	}
	//numOfDataClusters++;

	int indexClusterNo = numOfDataClusters < 256 ? 0 : (numOfDataClusters - 256) / 512 + 1;
	int entry = numOfDataClusters < 256 ? numOfDataClusters : (numOfDataClusters - 256) % 512;
	write(indexClusterNo, entry, clusterNo);
	numOfDataClusters++;
	return ret;
}

void FileMemoryManagmentUnit::free(unsigned long logClusterNo) {
	if (numOfDataClusters == 0) {
		return;
	}
	int indexClusterNo = logClusterNo < 256 ? 0 : (logClusterNo - 256) / 512 + 1;
	int entry = logClusterNo < 256 ? logClusterNo : (logClusterNo - 256) % 512;
	unsigned long phClusterNo = read(indexClusterNo, entry);
	if (phClusterNo == 0) return;//directory
	write(indexClusterNo, entry, 0);
	partitionMemoryManagmentUnit->free(phClusterNo);
	numOfDataClusters--;
	
	int currentNumOfIndexClusters = numOfDataClusters == 0 ? 0 : numOfDataClusters <= 256 ? 1 : 2 + (numOfDataClusters - 256 - 1) / 512;
	if (currentNumOfIndexClusters < numOfIndexClusters) {
		freeIndexCluster();
	}
}

unsigned long FileMemoryManagmentUnit::getIndexCluster() const {
	return indexCluster;
}

