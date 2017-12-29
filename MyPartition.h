#ifndef _MY_PARTITION_H_
#define _MY_PARTITION_H_

#include "Monitor.h"
#include "MemoryModule.h"
#include "MemoryManagment.h"
#include "OpenFilesTable.h"
#include "DirectoryManagment.h"
#include "part.h"
#include "fs.h"
#include "KernelFile.h"

class KernelFile;

class MyPartition {
public:
	MyPartition(Partition*);
	~MyPartition();
	int readRootDir(EntryNum, Directory&);
	char doesExist(const char*, const char*);
	void format();
	KernelFile* openFile(const char*, const char*, char);
	void closeFile(FileInfo*, char);
	char deleteFile(const char*, const char*);
	Partition* getMyPartition();
private:
	ReadersWritersMonitor *partitionUseMonitor, *openDeleteMonitor;
	Flag *formatFlag;
	MemoryInterface *partitionCache, *partitionBufferedWriter, *partitionWrapper;
	PartitionMemoryManagmentUnit *partitionMemoryManagmentUnit;
	DirectoryManagmentUnit *directoyManagmentUnit;
	OpenFilesTable *openFilesTable;
	Partition *myPartition;
};

#endif