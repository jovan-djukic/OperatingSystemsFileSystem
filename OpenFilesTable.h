#ifndef _OPEN_FILES_TABLE_H_
#define _OPEN_FILES_TABLE_H_

#include "fs.h"
#include "Monitor.h"
#include <Windows.h>

class FileInfo {
public:
	FileInfo(const char*, const char*, unsigned long, unsigned long, EntryNum);
	const unsigned long getSize() const;
	void setSize(unsigned long);
	const unsigned long getIndexCluster() const;
	void setIndexCluster(unsigned long);
	const char* getFname() const;
	const char* getFext() const;
	void setIsChanged();
	bool getIsChanged();
	EntryNum getEntryNum() const;
	ReadersWritersMonitor* getRWMonitor();
	ReadersWritersMonitor* getDMonitor();
	void incNumOfUsers();
	void decNumOfUsers();
	bool isNumOfUsersZero();
	~FileInfo();
private:
	CRITICAL_SECTION criticalSection;
	int numOfUsers;

	bool isChanged;
	char fname[8], fext[3];
	unsigned long indexCluster, size;
	EntryNum entryNum;
	ReadersWritersMonitor *rwMonitor, *dMonitor;
};

class OpenFilesTable {
public:
	OpenFilesTable(int);
	virtual ~OpenFilesTable();
	virtual void insertFileInfo(FileInfo*);
	virtual FileInfo* getFileInfo(const char*, const char*);
	virtual FileInfo* deleteFileInfo(const char*, const char*);
	virtual void format();
private:
	int hash(const char*, const char*);

	struct Node {
		FileInfo *info;
		Node *next;
		Node(FileInfo *);
	};
	struct TableEntry {
		Node *head, *tail;
		TableEntry();
	};

	TableEntry *table;
	int capacity;
};

class ThreadSafeOpenFilesTable : public OpenFilesTable {
public:
	ThreadSafeOpenFilesTable(int);
	~ThreadSafeOpenFilesTable();
	void insertFileInfo(FileInfo*) override;
	FileInfo* getFileInfo(const char*, const char*) override;
	FileInfo* deleteFileInfo(const char*, const char*) override;
	void format() override;
private:
	ReadersWritersMonitor *monitor;
};

#endif