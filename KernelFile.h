#ifndef _KERNEL_FILE_H_
#define _KERNEL_FILE_H_
 
#include "fs.h"
#include "defs.h"
#include "MemoryManagment.h"
#include "MemoryModule.h"
#include "Monitor.h"
#include "DirectoryManagment.h"
#include "OpenFilesTable.h"
#include "MyPartition.h"

class MyPartition;

class KernelFile {
public:
	virtual char write(BytesCnt, char*) = 0;
	virtual BytesCnt read(BytesCnt, char*) = 0;
	virtual char seek(BytesCnt) = 0;
	virtual BytesCnt filepos() = 0;
	virtual char eof() = 0;
	virtual BytesCnt getFileSize() = 0;
	virtual char truncate() = 0;
	virtual unsigned long getIndexCluster() const = 0;
	virtual ~KernelFile() = 0;
};

class SimpleKernelFile : public KernelFile {
public:
	SimpleKernelFile(BytesCnt, BytesCnt, unsigned long, MemoryInterface*, MemoryInterface*);
	BytesCnt read(BytesCnt, char*) override;
	char seek(BytesCnt) override;
	BytesCnt filepos() override; 
	char eof() override;
	BytesCnt getFileSize() override;
	unsigned long getIndexCluster() const override;
	virtual ~SimpleKernelFile();
protected:
	BytesCnt position, size;
	unsigned long indexCluster;

	CacheMemory *indexCache, *dataCache;
	MemoryMapper *memoryMapper;
};
//kod fajla otvorenog za citanje nema upisa tj nema vracanja u memoriju pa nema ovih odlozenih upisa i zato prosledjujemo isti memorijski modul
class ReadableKernelFile : public SimpleKernelFile {
public:
	ReadableKernelFile(BytesCnt, unsigned long, MemoryInterface*);
	char write(BytesCnt, char*) override;
	char truncate() override;
	~ReadableKernelFile();
private:
	DataMemoryChecker *dataMemoryChecker;
	IndexMemoryChecker *indexMemoryChecker;
};
//ovde imamo odvojen odlozen upis za indekse i za podatke to moze jer ce samo ove niti pristupati podacima i to razlicitim, a medjusobno iskljucenje je obezbedjeno na nivou odlozenog upisa
class WritableKernelFile : public SimpleKernelFile {
public:
	WritableKernelFile(BytesCnt,BytesCnt,unsigned long,MemoryInterface*,MemoryManagmentUnit*);
	char write(BytesCnt, char*) override;
	char truncate() override;
	~WritableKernelFile();
private:
	FileMemoryManagmentUnit *fileMemoryManagmentUnit;
	BufferedWriter *bufferedDataWriter, *bufferedIndexWriter;
	DataMemoryChecker *dataMemoryChecker;
	IndexMemoryChecker *indexMemoryChecker;
};

class KernelFileDecorator : public KernelFile {
public:
	KernelFileDecorator(KernelFile*, FileInfo*, MyPartition*const, char);
	~KernelFileDecorator();
	char write(BytesCnt, char*) override;
	BytesCnt read(BytesCnt, char*) override;
	char seek(BytesCnt) override;
	BytesCnt filepos() override;
	char eof() override;
	BytesCnt getFileSize() override;
	char truncate() override;
	unsigned long getIndexCluster() const override;
private:
	MyPartition * const myPartition;
	char mode;
	KernelFile *myFile;
	FileInfo *info;
};

#endif