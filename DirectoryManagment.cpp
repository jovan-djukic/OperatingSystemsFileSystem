#include "DirectoryManagment.h"
#include "part.h"
#include "MemoryModule.h"
#include "MemoryManagment.h"
#include "KernelFile.h"
#include "Monitor.h"
#include "fs.h"

DirectoryManagmentUnit::DirectoryManagmentUnit(unsigned long _indexCluster, MemoryInterface *_memoryModule, MemoryManagmentUnit *_partitionMemoryManagmentUnit) : indexCluster(_indexCluster), memoryModule(_memoryModule), partitionMemoryManagmentUnit(_partitionMemoryManagmentUnit), size(0), numOfEntriesPerCluster(ClusterSize / sizeof(Entry)) {
	directoryMemoryChecker = new IndexMemoryChecker(memoryModule);
	
	size = ClusterSize / (sizeof(unsigned long) * 2) * ClusterSize + ClusterSize / (sizeof(unsigned long) * 2) * ClusterSize / sizeof(unsigned long) * ClusterSize;

	cmpbuffer = new char[sizeof(Entry)];
	memset(cmpbuffer, 0, sizeof(Entry));

	monitor = new UpgradeMonitor();
}

DirectoryManagmentUnit::~DirectoryManagmentUnit() {
	monitor->startRead();
	monitor->startWrite();
	delete monitor;
	delete[] cmpbuffer;
	delete directoryMemoryChecker;
}

unsigned long DirectoryManagmentUnit::getOffset(EntryNum num) {
	unsigned long clusterNo = num / numOfEntriesPerCluster;
	return clusterNo * ClusterSize + (num % numOfEntriesPerCluster) * sizeof(Entry);
}

int DirectoryManagmentUnit::readDirectory(EntryNum num, Directory& d) {
	monitor->startRead();
	
	int ret = 0;
	EntryNum currEntry = 0;
	KernelFile *file = new ReadableKernelFile(size, indexCluster, directoryMemoryChecker);
	
	while (!file->eof()) {
		Entry entry;
		file->read(sizeof(Entry), reinterpret_cast<char*>(&entry));
		if (memcmp(&entry, cmpbuffer, sizeof(Entry)) == 0) {
			monitor->startWrite();
			size = file->filepos() - sizeof(entry);
			monitor->endWrite();
			break;
		}
		currEntry++;
		if (currEntry % numOfEntriesPerCluster == 0) {
			if (file->seek(file->filepos() + ClusterSize - numOfEntriesPerCluster * sizeof(Entry)) == 0) {
				break;
			}
		}
		if (entry.name[0] == 0) continue;
		if (currEntry >= num) {
			d[ret++] = entry;
			if (ret == ENTRYCNT) {
				break;
			}
		}
	}
	
	delete file;
	monitor->endRead();
	return ret;
}

void DirectoryManagmentUnit::updateFileInfo(DEntryInfo info) {
	monitor->startRead();
	monitor->startWrite();

	KernelFile *file = new WritableKernelFile(size, getOffset(info.num), indexCluster, directoryMemoryChecker, partitionMemoryManagmentUnit);
	file->write(sizeof(Entry), reinterpret_cast<char*>(&info.entry));
	delete file;

	monitor->endWrite();
	monitor->endRead();
}

DEntryInfo DirectoryManagmentUnit::getFileInfo(const char *fname, const char *fext) {
	Entry entry;
	bool found = false;
	monitor->startRead();
	
	KernelFile *file = new ReadableKernelFile(size, indexCluster, directoryMemoryChecker);
	EntryNum num = 0, currEntry = 0;

	while (!file->eof() && !found) {
		file->read(sizeof(Entry), reinterpret_cast<char*>(&entry));
		if (memcmp(&entry, cmpbuffer, sizeof(Entry)) == 0) {
			monitor->startWrite();
			size = file->filepos() - sizeof(entry);
			monitor->endWrite();
			break;
		}
		currEntry++;
		if (currEntry % numOfEntriesPerCluster == 0) {
			if (file->seek(file->filepos() + ClusterSize - numOfEntriesPerCluster * sizeof(Entry)) == 0) {
				break;
			}
		}
		if (entry.name[0] == 0) continue;
		if (memcmp(entry.name, fname, 8) == 0 && memcmp(entry.ext, fext, 3) == 0) {
			found = true;
			break;
		}
		num++;
	}

	delete file;
	if (!found) {
		entry.name[0] = 0;
	}
	monitor->endRead();
	DEntryInfo info;
	info.num = num;
	info.entry = entry;
	return info;
}

EntryNum DirectoryManagmentUnit::writeFileData(Entry entry) {
	monitor->startRead();
	monitor->startWrite();

	unsigned long currEntry = 0;

	KernelFile *file = new WritableKernelFile(size, 0, indexCluster, directoryMemoryChecker, partitionMemoryManagmentUnit);
	while (!file->eof()) {
		Entry entry;
		file->read(sizeof(Entry), reinterpret_cast<char*>(&entry));
		if (memcmp(&entry, cmpbuffer, sizeof(Entry)) == 0) {
			file->truncate();
			file->seek(file->filepos() - sizeof(Entry));
			break;
		}
		if (entry.name[0] == 0) {
			file->seek(file->filepos() - sizeof(Entry));
			break;
		}
		currEntry++;
		if (currEntry % numOfEntriesPerCluster == 0) {
			if (file->seek(file->filepos() + ClusterSize - numOfEntriesPerCluster * sizeof(Entry)) == 0) {
				break;
			}
		}
	}
	EntryNum num = ~0UL;
	
	if (file->eof()) {
		if (file->getFileSize() % ClusterSize == numOfEntriesPerCluster * sizeof(Entry)) {
			if (file->write(ClusterSize - numOfEntriesPerCluster * sizeof(Entry), cmpbuffer) == 0) {
				return num;
			}
		}
		if (file->getFileSize() % ClusterSize == 0) {
			char buffer[ClusterSize];
			memset(buffer, 0, ClusterSize);
			if (file->write(ClusterSize, buffer) == 0) {
				return num;
			}
			file->seek(file->filepos() - ClusterSize);
		}
	}
	
	if (file->write(sizeof(Entry), reinterpret_cast<char*>(&entry)) != 0) {
		num = currEntry;
		size = file->getFileSize();
	} 
	
	delete file;
	monitor->endWrite();
	monitor->endRead();

	return num;
}

void DirectoryManagmentUnit::deleteFileInfo(EntryNum num) {
	monitor->startRead();
	monitor->startWrite();
	KernelFile *file = new WritableKernelFile(size, getOffset(num), indexCluster, directoryMemoryChecker, partitionMemoryManagmentUnit);
	Entry entry;
	entry.name[0] = 0;
	file->write(sizeof(Entry), reinterpret_cast<char*>(&entry));
	bool okToTruncate = true;
	if (!file->eof()) {
		if (file->seek(getOffset(num + 1)) != 0) {
			BytesCnt bytesRead = file->read(sizeof(Entry), reinterpret_cast<char*>(&entry));
			if (bytesRead == sizeof(Entry) && memcmp(&entry, cmpbuffer, sizeof(Entry)) != 0)  {
				okToTruncate = false;
			}
		}
	}

	if (okToTruncate) {
		while (file->seek(getOffset(num - 1))) {
			file->read(sizeof(Entry), reinterpret_cast<char*>(&entry));
			if (entry.name[0] != 0) {
				break;
			}
			num--;
		}

		file->seek(getOffset(num));
		file->truncate();
		size = file->getFileSize();
	}

	monitor->endWrite();
	monitor->endRead();
}

void DirectoryManagmentUnit::format() {
	char buffer[ClusterSize];
	memset(buffer, 0, ClusterSize);
	memoryModule->write(indexCluster, buffer);
	size = 0;
}