#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <Windows.h>

class ReadersWritersMonitor {
public:
	ReadersWritersMonitor();
	virtual void startRead();
	virtual void endRead();
	virtual void startWrite();
	virtual void endWrite();
	virtual ~ReadersWritersMonitor() = 0;
protected:
	virtual bool readerEntryWaitCondition() = 0;
	virtual bool readerExitWakeConditon() = 0;
	virtual bool writerEntryWaitCondition() = 0;
	virtual bool writerExitWakeCondition() = 0;

	bool isBusy;
	int numOfReaders, numOfReadersWaiting, numOfWritersWaiting;
	CONDITION_VARIABLE readersCondition, writersCondition;
	CRITICAL_SECTION monitorCriticalSection;
};

class WriterPreferance : public ReadersWritersMonitor {
public:
	WriterPreferance();
private:
	bool readerEntryWaitCondition() override;
	bool readerExitWakeConditon() override;
	bool writerEntryWaitCondition() override;
	bool writerExitWakeCondition() override;
};

class ReaderPreferance : public ReadersWritersMonitor {
public:
	ReaderPreferance();
private:
	bool readerEntryWaitCondition() override;
	bool readerExitWakeConditon() override;
	bool writerEntryWaitCondition() override;
	bool writerExitWakeCondition() override;
};

class UpgradeMonitor : public ReadersWritersMonitor {
public:
	UpgradeMonitor();
private:
	bool readerEntryWaitCondition() override;
	bool readerExitWakeConditon() override;
	bool writerEntryWaitCondition() override;
	bool writerExitWakeCondition() override;
};

class ModuleMonitor {
public:
	ModuleMonitor(int);
	void startRead();
	void endRead();
	void startWrite();
	void endWrite();
	void putBlock();
	~ModuleMonitor();
private:
	CRITICAL_SECTION monitorCriticalSection, moduleCriticalSection;
	CONDITION_VARIABLE condition;
	int lineSize, numOfBlocksRead, numOfBlocksToBeWriten;
	bool requestPut, requestPending;
};

class Flag {
public:
	Flag();
	~Flag();
	bool flagValue();
	void setFlag(bool);
private:
	CRITICAL_SECTION criticalSection;
	bool flag;
};

#endif