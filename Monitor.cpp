#include "Monitor.h"
#include <Windows.h>

ReadersWritersMonitor::ReadersWritersMonitor() : numOfReaders(0), isBusy(false), numOfReadersWaiting(0), numOfWritersWaiting(0) {
	InitializeConditionVariable(&readersCondition);
	InitializeConditionVariable(&writersCondition);
	InitializeCriticalSection(&monitorCriticalSection);
}

ReadersWritersMonitor::~ReadersWritersMonitor() {
	DeleteCriticalSection(&monitorCriticalSection);
}

void ReadersWritersMonitor::startRead() {
	EnterCriticalSection(&monitorCriticalSection);
		while (readerEntryWaitCondition()) {
			numOfReadersWaiting++;
			SleepConditionVariableCS(&readersCondition, &monitorCriticalSection, INFINITE);
			numOfReadersWaiting--;
		}
		numOfReaders++;
		WakeAllConditionVariable(&readersCondition);
	LeaveCriticalSection(&monitorCriticalSection);
}

void ReadersWritersMonitor::endRead() {
	EnterCriticalSection(&monitorCriticalSection);
		numOfReaders--;
		if (readerExitWakeConditon()) WakeAllConditionVariable(&writersCondition);
	LeaveCriticalSection(&monitorCriticalSection);
}

void ReadersWritersMonitor::startWrite() {
	EnterCriticalSection(&monitorCriticalSection);
		while (writerEntryWaitCondition()) {
			numOfWritersWaiting++;
			SleepConditionVariableCS(&writersCondition, &monitorCriticalSection, INFINITE);
			numOfWritersWaiting--;
		}
		isBusy = true;
	LeaveCriticalSection(&monitorCriticalSection);
}

void ReadersWritersMonitor::endWrite() {
	EnterCriticalSection(&monitorCriticalSection);
		isBusy = false;
		if (writerExitWakeCondition()) {
			WakeAllConditionVariable(&writersCondition);
		} else {
			WakeAllConditionVariable(&readersCondition);
		}
	LeaveCriticalSection(&monitorCriticalSection);
}

WriterPreferance::WriterPreferance() {

}

bool WriterPreferance::readerEntryWaitCondition() {
	return isBusy || numOfWritersWaiting != 0;
}

bool WriterPreferance::readerExitWakeConditon() {
	return numOfReaders == 0 || numOfWritersWaiting != 0;
}

bool WriterPreferance::writerEntryWaitCondition() {
	return isBusy || numOfReaders != 0;
}

bool WriterPreferance::writerExitWakeCondition() {
	return numOfWritersWaiting != 0;
}

UpgradeMonitor::UpgradeMonitor() {

}

bool UpgradeMonitor::readerEntryWaitCondition() {
	return isBusy || numOfWritersWaiting != 0;
}

bool UpgradeMonitor::readerExitWakeConditon() {
	return numOfWritersWaiting != 0 && numOfReaders == numOfWritersWaiting;
}

bool UpgradeMonitor::writerEntryWaitCondition() {
	return isBusy || numOfReaders != (numOfWritersWaiting + 1);
}

bool UpgradeMonitor::writerExitWakeCondition() {
	return numOfWritersWaiting != 0;
}

ReaderPreferance::ReaderPreferance() {

}

bool ReaderPreferance::readerEntryWaitCondition() {
	return isBusy;
}

bool ReaderPreferance::readerExitWakeConditon() {
	return numOfReaders == 0 && numOfReadersWaiting == 0;
}

bool ReaderPreferance::writerEntryWaitCondition() {
	return isBusy || numOfReaders != 0 || numOfReadersWaiting != 0;
}

bool ReaderPreferance::writerExitWakeCondition() {
	return numOfReadersWaiting == 0;
}

//OVO CE MOZDA DA TI BUDE IZVOR GRESAKA JER MNG NAKARADNO ODRADJENO, CEO OVAJ BAFERISANI PISAC JE NAKARADNO ODRADJEN
ModuleMonitor::ModuleMonitor(int _lineSize) : lineSize(_lineSize), numOfBlocksRead(0), numOfBlocksToBeWriten(0), requestPending(false), requestPut(false) {
	InitializeConditionVariable(&condition);
	InitializeCriticalSection(&monitorCriticalSection);
	InitializeCriticalSection(&moduleCriticalSection);
}

ModuleMonitor::~ModuleMonitor() {
	DeleteCriticalSection(&monitorCriticalSection);
	DeleteCriticalSection(&moduleCriticalSection);
}

void ModuleMonitor::startRead() {
	if (numOfBlocksRead == 0) {
		EnterCriticalSection(&monitorCriticalSection);
		if (requestPending) {
			SleepConditionVariableCS(&condition, &monitorCriticalSection, INFINITE);
		}
		LeaveCriticalSection(&monitorCriticalSection);
		EnterCriticalSection(&moduleCriticalSection);
	}
}

void ModuleMonitor::endRead() {
	numOfBlocksRead++;
	if (numOfBlocksRead == lineSize) {
		LeaveCriticalSection(&moduleCriticalSection);
		numOfBlocksRead = 0;
		if (requestPut) {
			EnterCriticalSection(&monitorCriticalSection);
			if (numOfBlocksToBeWriten != 0) {
				requestPending = true;
			}
			LeaveCriticalSection(&monitorCriticalSection);
			requestPut = false;
		}
	}
}

void ModuleMonitor::putBlock() {
	EnterCriticalSection(&monitorCriticalSection);
	if (requestPending) {
		SleepConditionVariableCS(&condition, &monitorCriticalSection, INFINITE);
	}
	numOfBlocksToBeWriten++;
	LeaveCriticalSection(&monitorCriticalSection);
	requestPut = true;
}

void ModuleMonitor::startWrite() {
	EnterCriticalSection(&moduleCriticalSection);
}

void ModuleMonitor::endWrite() {
	EnterCriticalSection(&monitorCriticalSection);
	numOfBlocksToBeWriten--;
	if (numOfBlocksToBeWriten == 0) {
		requestPending = false;
		WakeAllConditionVariable(&condition);
	}
	LeaveCriticalSection(&monitorCriticalSection);
	LeaveCriticalSection(&moduleCriticalSection);
}

Flag::Flag() : flag(false) {
	InitializeCriticalSection(&criticalSection);
}

Flag::~Flag() {
	DeleteCriticalSection(&criticalSection);
}

bool Flag::flagValue() {
	EnterCriticalSection(&criticalSection);
	bool ret = flag;
	LeaveCriticalSection(&criticalSection);
	return ret;
}

void Flag::setFlag(bool newValue) {
	EnterCriticalSection(&criticalSection);
	flag = newValue;
	LeaveCriticalSection(&criticalSection);
}