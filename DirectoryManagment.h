#ifndef _DIRECTORY_MANAGMENT_H_
#define _DIRECTORY_MANAGMENT_H_

#include "MemoryModule.h"
#include "MemoryManagment.h"
#include "Monitor.h"
#include "fs.h"

struct DEntryInfo {
	EntryNum num;
	Entry entry;
};

class DirectoryManagmentUnit {
public:
	DirectoryManagmentUnit(unsigned long, MemoryInterface*, MemoryManagmentUnit*);
	~DirectoryManagmentUnit();
	int readDirectory(EntryNum, Directory&);
	void updateFileInfo(DEntryInfo);
	DEntryInfo getFileInfo(const char*, const char*);
	EntryNum writeFileData(Entry);
	void deleteFileInfo(EntryNum);
	void format();
private:
	unsigned long getOffset(EntryNum);

	char *cmpbuffer;
	int numOfEntriesPerCluster;
	unsigned long indexCluster, size;
	MemoryInterface *memoryModule, *directoryMemoryChecker;
	MemoryManagmentUnit *partitionMemoryManagmentUnit;
	ReadersWritersMonitor *monitor;
};

#endif