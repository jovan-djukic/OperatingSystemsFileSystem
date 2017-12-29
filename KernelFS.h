#ifndef _KERNEL_FS_H_
#define _KERNEL_FS_H_

#include "Monitor.h"
#include "MyPartition.h"
#include "part.h"
#include "fs.h"
#include "file.h"

class KernelFS {
public:
	//static KernelFS* getInstance();
	//static void deleteInstance();
	KernelFS();
	char mount(Partition*);
	char unmount(char);
	char readRootDir(char, EntryNum, Directory&);
	char doesExist(char*);
	File* open(char*, char);
	char format(char);
	char deleteFile(char*);
	~KernelFS();
private:
	struct ParsedName {
		char part, fname[8], fext[3];
	};
	ParsedName parseName(char*);

	//KernelFS();
	//static KernelFS *instance;

	MyPartition **partitions;
	ReadersWritersMonitor *fsMonitor;
};

#endif