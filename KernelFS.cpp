#include "KernelFS.h"
#include "MyPartition.h"
#include "part.h"
#include <cstring>


KernelFS::KernelFS() {
	partitions = new MyPartition*[26];
	for (int i = 0; i < 26; i++) {
		partitions[i] = nullptr;
	}
	fsMonitor = new WriterPreferance();
}

KernelFS::~KernelFS() {
	delete[] partitions;
	delete fsMonitor;
}

char KernelFS::mount(Partition *part) {
	char ret = 0;
	
	fsMonitor->startWrite();
	for (int i = 0; i < 26; i++) {
		if (partitions[i] == nullptr) {
			partitions[i] = new MyPartition(part);
			ret = 'A' + i;
			break;
		}
	}
	fsMonitor->endWrite();
	return ret;
}

char KernelFS::unmount(char part) {
	char ret = 0;
	fsMonitor->startWrite();

	MyPartition *partition = partitions[part - 'A'];
	partitions[part - 'A'] = nullptr;
	
	fsMonitor->endWrite();

	if (partition != nullptr) {
		delete partition;
		ret = 1;
	}

	return ret;
}

char KernelFS::readRootDir(char part, EntryNum num, Directory& d) {
	char ret = 0;
	fsMonitor->startRead();

	if (partitions[part - 'A'] != nullptr) {
		ret = partitions[part - 'A']->readRootDir(num, d);
	}

	fsMonitor->endRead();
	return ret;
}

KernelFS::ParsedName KernelFS::parseName(char *fullFname) {
	ParsedName pn;
	pn.part = (*fullFname);
	int len = strlen(fullFname);
	memcpy(pn.fext, fullFname + len - 3, 3);
	memcpy(pn.fname, fullFname + 3, len - 7);
	for (int i = len - 7; i < 8; i++) {
		pn.fname[i] = ' ';
	}
	return pn;
}

char KernelFS::doesExist(char *fullFname) {
	int len = strlen(fullFname);
	if (len < 5 || len > 12) return 0;
	ParsedName pn = parseName(fullFname);
	
	char ret = 0;
	fsMonitor->startRead();
	if (partitions[pn.part - 'A'] != nullptr) {
		ret = partitions[pn.part - 'A']->deleteFile(pn.fname, pn.fext);
	}
	fsMonitor->endRead();
	return ret;
}

File* KernelFS::open(char *fullFname, char mode) {
	File *file = nullptr;
	ParsedName pn = parseName(fullFname);
	fsMonitor->startRead();
	if (partitions[pn.part - 'A'] != nullptr) {
		KernelFile *kfile = partitions[pn.part - 'A']->openFile(pn.fname, pn.fext, mode);
		if (kfile != nullptr) {
			file = new File();
			file->myImpl = kfile;
		}
	}
	fsMonitor->endRead();
	return file;
}

char KernelFS::deleteFile(char *fullFname) {
	char ret = 1;
	ParsedName pn = parseName(fullFname);
	fsMonitor->startRead();
	if (partitions[pn.part - 'A'] != nullptr) {
		ret = partitions[pn.part - 'A']->deleteFile(pn.fname, pn.fext);
	}
	fsMonitor->endRead();
	return ret;
}

char KernelFS::format(char part) {
	char ret = 1;
	fsMonitor->startRead();
	if (partitions[part - 'A'] != nullptr) {
		partitions[part - 'A']->format();
		ret = 0;
	}
	fsMonitor->endRead();
	return ret;
}