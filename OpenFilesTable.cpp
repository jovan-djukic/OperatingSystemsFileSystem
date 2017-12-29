#include "OpenFilesTable.h"
#include "Monitor.h"

FileInfo::FileInfo(const char *_fname, const char *_fext, unsigned long _size, unsigned long _indexCluster, EntryNum _num) : size(_size), indexCluster(_indexCluster), numOfUsers(0), entryNum(_num), isChanged(false) {
	memcpy(fname, _fname, 8);
	memcpy(fext, _fext, 3);
	rwMonitor = new ReaderPreferance();
	dMonitor = new ReaderPreferance();

	InitializeCriticalSection(&criticalSection);
}
FileInfo::~FileInfo() {
	delete rwMonitor;
	delete dMonitor;

	DeleteCriticalSection(&criticalSection);
}

const unsigned long FileInfo::getSize() const {
	return size;
}

void FileInfo::setSize(unsigned long _size) {
	size = _size;
}

const unsigned long FileInfo::getIndexCluster() const {
	return indexCluster;
}

void FileInfo::setIndexCluster(unsigned long _indexCluster) {
	indexCluster = _indexCluster;
}

const char* FileInfo::getFname() const {
	return fname;
}

const char* FileInfo::getFext() const {
	return fext;
}

EntryNum FileInfo::getEntryNum() const {
	return entryNum;
}

void FileInfo::setIsChanged() {
	isChanged = true;
}

bool FileInfo::getIsChanged() {
	return isChanged;
}

ReadersWritersMonitor* FileInfo::getRWMonitor() {
	return rwMonitor;
}

ReadersWritersMonitor* FileInfo::getDMonitor() {
	return dMonitor;
}

void FileInfo::incNumOfUsers() {
	EnterCriticalSection(&criticalSection);
	numOfUsers++;
	LeaveCriticalSection(&criticalSection);
}

void FileInfo::decNumOfUsers() {
	EnterCriticalSection(&criticalSection);
	numOfUsers--;
	LeaveCriticalSection(&criticalSection);
}

bool FileInfo::isNumOfUsersZero() {
	EnterCriticalSection(&criticalSection);
	bool ret = numOfUsers == 0;
	LeaveCriticalSection(&criticalSection);
	return ret;
}

OpenFilesTable::Node::Node(FileInfo *_info) : info(_info), next(nullptr) {

}

OpenFilesTable::TableEntry::TableEntry() : head(nullptr), tail(nullptr) {

}

OpenFilesTable::OpenFilesTable(int _capacity) : capacity(_capacity) {
	table = new TableEntry[capacity];
	for (int i = 0; i < capacity; i++) {
		table[i].head = table[i].tail = nullptr;
	}
}

OpenFilesTable::~OpenFilesTable() {
	delete[] table;
}

int OpenFilesTable::hash(const char *fname, const char *fext) {
	unsigned int entry = 5831;
	for (int i = 0; i < 8; i++) {
		entry += fname[i];
	}
	for (int i = 0; i < 3; i++) {
		entry += fext[i];
	}
	return entry;
}

void OpenFilesTable::insertFileInfo(FileInfo *info) {
	int entry = hash(info->getFname(), info->getFext()) % capacity;
	Node *newNode = new Node(info);
	if (table[entry].head == nullptr) {
		table[entry].head = newNode;
	} else {
		table[entry].tail->next = newNode;
	}
	table[entry].tail = newNode;
}

FileInfo* OpenFilesTable::getFileInfo(const char *fname, const char *fext) {
	FileInfo *info = nullptr;
	unsigned int entry = hash(fname, fext) % capacity;
	Node *head = table[entry].head;
	while (head != nullptr) {
		if (memcmp(fname, head->info->getFname(), 8) == 0 && memcmp(fext, head->info->getFext(), 3) == 0) {
			info = head->info;
			break;
		} else {
			head = head->next;
		}
	}
	return info;
}

FileInfo* OpenFilesTable::deleteFileInfo(const char *fname, const char *fext) {
	FileInfo *info = nullptr;
	unsigned int entry = hash(fname, fext) % capacity;
	Node *head = table[entry].head, *last = nullptr;
	while (head != nullptr) {
		if (memcmp(fname, head->info->getFname(), 8) == 0 && memcmp(fext, head->info->getFext(), 3) == 0) {
			info = head->info;
			break;
		}
		else {
			last = head;
			head = head->next;
		}
	}
	if (head != nullptr) {
		if (last == nullptr) {
			table[entry].head = table[entry].head->next;
			if (table[entry].head == nullptr) {
				table[entry].tail = nullptr;
			}
		} else {
			last->next = head->next;
			if (head == table[entry].tail) {
				table[entry].tail = last;
			}
		}
		delete head;
	}
	return info;
}

void OpenFilesTable::format() {
	for (int i = 0; i < capacity; i++) {
		Node *tmp = table[i].head;
		while (tmp != nullptr) {
			Node *old = tmp;
			tmp = tmp->next;
			delete old->info;
			delete old;
		}
		table[i].head = table[i].tail = nullptr;
	}
}

ThreadSafeOpenFilesTable::ThreadSafeOpenFilesTable(int _capacity) : OpenFilesTable(_capacity) {
	monitor = new ReaderPreferance();
}

ThreadSafeOpenFilesTable::~ThreadSafeOpenFilesTable() {
	delete monitor;
}

void ThreadSafeOpenFilesTable::insertFileInfo(FileInfo *info) {
	monitor->startWrite();
	OpenFilesTable::insertFileInfo(info);
	monitor->endWrite();
}

FileInfo* ThreadSafeOpenFilesTable::getFileInfo(const char *fname, const char *fext) {
	monitor->startRead();
	FileInfo *ret = OpenFilesTable::getFileInfo(fname, fext);
	monitor->endRead();
	return ret;
}

FileInfo* ThreadSafeOpenFilesTable::deleteFileInfo(const char *fname, const char *fext) {
	monitor->startWrite();
	FileInfo *ret = OpenFilesTable::deleteFileInfo(fname, fext);
	monitor->endWrite();
	return ret;
}

void ThreadSafeOpenFilesTable::format() {
	monitor->startWrite();
	OpenFilesTable::format();
	monitor->endWrite();
}

