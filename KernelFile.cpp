#include "KernelFile.h"
#include "fs.h"
#include "defs.h"
#include "part.h"
#include "MyPartition.h"

KernelFile::~KernelFile() {

}

SimpleKernelFile::SimpleKernelFile(BytesCnt _size, BytesCnt _position, unsigned long _indexCluster, MemoryInterface *_memoryDataModule, MemoryInterface *_memoryIndexModule) : size(_size), position(_position), indexCluster(_indexCluster) {
	indexCache = new ByteCacheMemory(FILE_INDEX_CACHE_NUM_OF_ENTRIES, FILE_INDEX_CACHE_SIZE_OF_ENTRY, new LRU(FILE_INDEX_CACHE_NUM_OF_ENTRIES), _memoryIndexModule);
	memoryMapper = new MemoryMapper(_memoryDataModule, indexCache, indexCluster);
	dataCache = new ByteCacheMemory(FILE_DATA_CACHE_NUM_OF_ENTRIES, FILE_DATA_CACHE_SIZE_OF_ENTRY, new LRU(FILE_DATA_CACHE_NUM_OF_ENTRIES), memoryMapper);
}

SimpleKernelFile::~SimpleKernelFile() {
	delete dataCache;
	delete indexCache;
	delete memoryMapper;
}

BytesCnt SimpleKernelFile::read(BytesCnt numOfBytes, char *buffer) {
	BytesCnt bytesRead = 0;
	for (unsigned long i = 0; i < numOfBytes && position < size; i++, position++, bytesRead++) {
		dataCache->read(position, buffer + i);
	}
	return bytesRead;
}

char SimpleKernelFile::seek(BytesCnt newPosition) {
	char ret = 0;
	if (newPosition < size) {
		position = newPosition;
		ret = 1;
	} 
	return ret;
}

BytesCnt SimpleKernelFile::filepos() {
	return position;
}

char SimpleKernelFile::eof() {
	if (position == size) {
		return 2;
	} else {
		return 0;
	}
}

BytesCnt SimpleKernelFile::getFileSize() {
	return size;
}

unsigned long SimpleKernelFile::getIndexCluster() const {
	return indexCluster;
}

//kod fajla otvorenog za citanje nema upisa tj nema vracanja u memoriju pa nema ovih odlozenih upisa i zato prosledjujemo isti memorijski modul
ReadableKernelFile::ReadableKernelFile(BytesCnt _size, unsigned long _indexCluster, MemoryInterface *_memoryModule) : SimpleKernelFile(_size, 0, _indexCluster, dataMemoryChecker = new DataMemoryChecker(_memoryModule), indexMemoryChecker = new IndexMemoryChecker(_memoryModule)) {

}

ReadableKernelFile::~ReadableKernelFile() {
	delete dataMemoryChecker;
	delete indexMemoryChecker;
}

char ReadableKernelFile::write(BytesCnt numOfBytes, char *buffer) {
	return 0;
}

char ReadableKernelFile::truncate() {
	return 0;
}

WritableKernelFile::WritableKernelFile(BytesCnt _size, BytesCnt _position, unsigned long _indexCluster, MemoryInterface *_memoryModule, MemoryManagmentUnit *_partitionMemoryManagmentUnit) : SimpleKernelFile(_size, _position, _indexCluster, bufferedDataWriter = new BufferedWriter(FILE_DATA_CACHE_SIZE_OF_ENTRY, dataMemoryChecker = new DataMemoryChecker(_memoryModule)), bufferedIndexWriter = new BufferedWriter(FILE_INDEX_CACHE_SIZE_OF_ENTRY, indexMemoryChecker = new IndexMemoryChecker(_memoryModule))) {
	fileMemoryManagmentUnit = new FileMemoryManagmentUnit(indexCache, _partitionMemoryManagmentUnit, (_size + ClusterSize - 1) / ClusterSize, _indexCluster);//mozda size nije u klasterima
}

char WritableKernelFile::write(BytesCnt numOfBytes, char *buffer) {
	for (unsigned long i = 0; i < numOfBytes; i++, position++) {
		//has space?
		if (position == size) {
			if (size % ClusterSize == 0) {
				int ret = fileMemoryManagmentUnit->allocate();
				if (ret == 0) {
					return 0;
				} else if (ret == 2) {
					indexCluster = fileMemoryManagmentUnit->getIndexCluster();
					memoryMapper->setIndexCluster(indexCluster);
				}
			}
			size++;
		}
		dataCache->write(position, buffer + i);
	}
	return 1;
}
//trucate radi do te pozicije racunajuci i da je ona slobodna
char WritableKernelFile::truncate() {
	if (size == 0) return 1;
	while ((size - 1) / ClusterSize > position / ClusterSize) {
		fileMemoryManagmentUnit->free((size - 1) / ClusterSize);
		size -= ClusterSize;
	}
	size = position;
	if (size % ClusterSize == 0) {
		fileMemoryManagmentUnit->free(0);
	}
	return 1;
}

WritableKernelFile::~WritableKernelFile() {
	indexCache->flush();
	dataCache->flush();
	delete bufferedIndexWriter;
	delete bufferedDataWriter;
	delete dataMemoryChecker;
	delete indexMemoryChecker;
	delete fileMemoryManagmentUnit;
}

KernelFileDecorator::KernelFileDecorator(KernelFile *_myFile, FileInfo *_info, MyPartition *_myPartition, char _mode) : myFile(_myFile),info(_info), myPartition(_myPartition), mode(_mode) {

}

char KernelFileDecorator::write(BytesCnt cnt, char *buffer) {
	return myFile->write(cnt, buffer);
}

BytesCnt KernelFileDecorator::read(BytesCnt cnt, char *buffer) {
	return myFile->read(cnt, buffer);
}

char KernelFileDecorator::seek(BytesCnt newPosition) {
	return myFile->seek(newPosition);
}

BytesCnt KernelFileDecorator::filepos() {
	return myFile->filepos();
}

char KernelFileDecorator::eof() {
	return myFile->eof();
}

BytesCnt KernelFileDecorator::getFileSize() {
	return myFile->getFileSize();
}

char KernelFileDecorator::truncate() {
	return myFile->truncate();
}

unsigned long KernelFileDecorator::getIndexCluster() const {
	return myFile->getIndexCluster();
}

KernelFileDecorator::~KernelFileDecorator() {
	if (mode != 'r' && mode != 'R') {
		info->setSize(myFile->getFileSize());
		info->setIndexCluster(myFile->getIndexCluster());
		info->setIsChanged();
	}
	delete myFile;
	myPartition->closeFile(info, mode);
}