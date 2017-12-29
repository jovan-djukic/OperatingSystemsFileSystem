#ifndef _MEMORY_MANAGMENT_H_
#define _MEMORY_MANAGMENT_H_

#include <Windows.h>
#include "MemoryModule.h"

class MemoryManagmentUnit {
public:
	virtual unsigned long allocate() = 0;
	virtual void free(unsigned long) = 0;
	virtual ~MemoryManagmentUnit() = 0;
};

class PartitionMemoryManagmentUnit : public MemoryManagmentUnit {
public:
	PartitionMemoryManagmentUnit(unsigned long, MemoryInterface*);
	unsigned long allocate() override;
	void free(unsigned long) override;
	void format();
	~PartitionMemoryManagmentUnit();
private:
	unsigned long findFreeCluster();
	void read(unsigned long);
	void flush();

	CRITICAL_SECTION criticalSection;
	bool isChanged;
	unsigned long numOfClustersInPartition;
	int numOfClusters, inMemory;
	char *bitVectorBuffer;
	MemoryInterface *memoryModule;
};

class FileMemoryManagmentUnit : public MemoryManagmentUnit {
public:
	FileMemoryManagmentUnit(CacheMemory*, MemoryManagmentUnit*,unsigned long, unsigned long);
	unsigned long allocate() override;
	void free(unsigned long) override;
	unsigned long getIndexCluster() const;
	~FileMemoryManagmentUnit();
private:
	unsigned long allocateNewIndexCluster();
	void freeIndexCluster();
	void write(int,int,unsigned long);
	unsigned long read(int, int);

	MemoryManagmentUnit *partitionMemoryManagmentUnit;
	CacheMemory *indexCache;
	unsigned long numOfDataClusters, indexCluster;
	int numOfIndexClusters;

	static const unsigned long MAX_FILE_CLUSTERS;
};

#endif