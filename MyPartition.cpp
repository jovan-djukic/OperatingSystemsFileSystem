#include "MyPartition.h"
#include "part.h"
#include "MemoryModule.h"
#include "defs.h"
#include "MemoryManagment.h"
#include "DirectoryManagment.h"
#include "OpenFilesTable.h"
#include "Monitor.h"

MyPartition::MyPartition(Partition *partition) : myPartition(partition){
	unsigned long numOfClusters = partition->getNumOfClusters();
	partitionWrapper = new PartitionWrapper(partition, numOfClusters);
	partitionBufferedWriter = new BufferedWriter(PARTITION_CACHE_SIZE_OF_ENTRY, partitionWrapper);
	partitionCache = new ThreadSafeBlockCache(PARTITION_CACHE_NUM_OF_ENTRIES, PARTITION_CACHE_SIZE_OF_ENTRY, PARTITION_CACHE_LRU_REQUEST_BUFFER_SIZE, partitionBufferedWriter);
	partitionMemoryManagmentUnit = new PartitionMemoryManagmentUnit(numOfClusters, partitionCache);
	directoyManagmentUnit = new DirectoryManagmentUnit((numOfClusters + ClusterSize - 1) / ClusterSize, partitionCache, partitionMemoryManagmentUnit);

	openFilesTable = new OpenFilesTable(OPEN_FILES_TABLE_SIZE);
	formatFlag = new Flag();
	partitionUseMonitor = new ReaderPreferance();
	openDeleteMonitor = new UpgradeMonitor();
}

MyPartition::~MyPartition() {	
	partitionUseMonitor->startWrite();

	delete directoyManagmentUnit;
	delete partitionMemoryManagmentUnit;
	delete partitionCache;
	delete partitionBufferedWriter;
	delete partitionWrapper;

	delete openFilesTable;
	delete partitionUseMonitor;
	delete openDeleteMonitor;
	delete formatFlag;
}

int MyPartition::readRootDir(EntryNum num, Directory& dir) {
	return directoyManagmentUnit->readDirectory(num, dir);
}

char MyPartition::doesExist(const char *fname, const char *fext) {
	partitionUseMonitor->startRead();
	openDeleteMonitor->startRead();
	
	char ret = 1;
	if (openFilesTable->getFileInfo(fname, fext) == nullptr) {
		DEntryInfo info = directoyManagmentUnit->getFileInfo(fname, fext);
		if (info.entry.name[0] == 0) {
			ret = 0;
		}
	}

	openDeleteMonitor->endRead();
	partitionUseMonitor->endRead();

	return ret;
}

void MyPartition::format() {
	formatFlag->setFlag(true);
	partitionUseMonitor->startWrite();

	openFilesTable->format();
	directoyManagmentUnit->format();
	partitionMemoryManagmentUnit->format();

	partitionUseMonitor->endWrite();
	formatFlag->setFlag(false);
}

KernelFile* MyPartition::openFile(const char *fname, const char *fext, char mode) {
	if (formatFlag->flagValue()) {
		return nullptr;
	}
	partitionUseMonitor->startRead();
	openDeleteMonitor->startRead();

	KernelFile *file = nullptr;
	FileInfo *info = openFilesTable->getFileInfo(fname, fext);
	
	if (info == nullptr) {
		openDeleteMonitor->startWrite();
		
		info = openFilesTable->getFileInfo(fname, fext);
		if (info == nullptr) {
			DEntryInfo dInfo = directoyManagmentUnit->getFileInfo(fname, fext);
			if (dInfo.entry.name[0] != 0) {
				info = new FileInfo(dInfo.entry.name, dInfo.entry.ext, dInfo.entry.size, dInfo.entry.indexCluster, dInfo.num);
				openFilesTable->insertFileInfo(info);
			} else if (mode == 'w' || mode == 'W') {
				DEntryInfo finfo;
				memcpy(finfo.entry.name, fname, 8);
				memcpy(finfo.entry.ext, fext, 3);
				finfo.entry.size = 0;
				finfo.entry.indexCluster = 0;
				finfo.num = directoyManagmentUnit->writeFileData(finfo.entry);
				if (finfo.num != ~0UL) {
					info = new FileInfo(fname, fext, finfo.entry.size, finfo.entry.indexCluster, finfo.num);
					openFilesTable->insertFileInfo(info);
				}
			}
		}
		openDeleteMonitor->endWrite();
	}
	if (info != nullptr) {
		info->getDMonitor()->startRead();
		info->incNumOfUsers();
	}
	openDeleteMonitor->endRead();

	if (info == nullptr) {
		partitionUseMonitor->endRead();
		return nullptr;
	}

	switch (mode) {
	case 'r':
	case 'R':{
		info->getRWMonitor()->startRead();
		file = new KernelFileDecorator(new ReadableKernelFile(info->getSize(), info->getIndexCluster(), partitionCache), info, this, mode);
		break;
	}
	case 'w':
	case 'W':{
		info->getRWMonitor()->startWrite();
		KernelFile *writeFile = new WritableKernelFile(info->getSize(), 0, info->getIndexCluster(), partitionCache, partitionMemoryManagmentUnit);
		writeFile->truncate();
		file = new KernelFileDecorator(writeFile, info, this, mode);
		break;
	}
	case 'a':
	case 'A': {
		info->getRWMonitor()->startWrite();
		file = new KernelFileDecorator(new WritableKernelFile(info->getSize(), info->getSize(), info->getIndexCluster(), partitionCache, partitionMemoryManagmentUnit), info, this, mode);
		break;
	}
	}

	return file;
}

void MyPartition::closeFile(FileInfo *info, char mode) {
	openDeleteMonitor->startRead();
	switch (mode) {
	case 'r':
	case 'R': {
		info->getRWMonitor()->endRead();
		break;
	}
	default: {
		info->getRWMonitor()->endWrite();
		break;
	}
	}

	info->decNumOfUsers();
	if (info->isNumOfUsersZero()) {
		char fname[8], fext[3];
		memcpy(fname, info->getFname(), 8);
		memcpy(fext, info->getFext(), 3);
		
		openDeleteMonitor->startWrite();
		
		FileInfo *finfo = openFilesTable->getFileInfo(fname, fext);
		if (finfo != nullptr && finfo->isNumOfUsersZero()) {
		
			finfo = openFilesTable->deleteFileInfo(fname, fext);
			if (finfo->getIsChanged()) {
				DEntryInfo entry;
				entry.num = finfo->getEntryNum();
				memcpy(entry.entry.name, finfo->getFname(), 8);
				memcpy(entry.entry.ext, finfo->getFext(), 3);
				entry.entry.indexCluster = finfo->getIndexCluster();
				entry.entry.size = finfo->getSize();
				directoyManagmentUnit->updateFileInfo(entry);
			}
			delete finfo;
			info = nullptr;
		}
		openDeleteMonitor->endWrite();
	}
	if (info != nullptr) {
		info->getDMonitor()->endRead();
	}
	partitionUseMonitor->endRead();
	openDeleteMonitor->endRead();
}

char MyPartition::deleteFile(const char *fname, const char *fext) {
	char ret = 0;
	FileInfo *finfo = nullptr;
	partitionUseMonitor->startRead();
	openDeleteMonitor->startRead();
	openDeleteMonitor->startWrite();

	DEntryInfo info = directoyManagmentUnit->getFileInfo(fname, fext);
	if (info.entry.name[0] != 0) {
		ret = 1;
		directoyManagmentUnit->deleteFileInfo(info.num);
		finfo = openFilesTable->getFileInfo(info.entry.name, info.entry.ext);
	}

	openDeleteMonitor->endWrite();
	openDeleteMonitor->endRead();

	if (finfo != nullptr) {
		finfo->getDMonitor()->startWrite();
		KernelFile *file = new WritableKernelFile(finfo->getSize(), 0, finfo->getIndexCluster(), partitionCache, partitionMemoryManagmentUnit);
		file->truncate();
		delete file;
		delete finfo;
	}

	partitionUseMonitor->endRead();

	return ret;
}

Partition* MyPartition::getMyPartition() {
	return	myPartition;
}
