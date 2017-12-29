#include "Thread.h"
#include <Windows.h>
#include <limits>

#include <iostream>

DWORD WINAPI Thread::starterFunction(LPVOID threadObject) {
	Thread *thread = static_cast<Thread*>(threadObject);
	WaitForSingleObject(thread->startSemHandle, INFINITE);
	if (thread->isStarted) {
		thread->run();
	}
	return 0;
}

Thread::Thread() : isStarted(false) {
	startSemHandle = CreateSemaphore(NULL, 0, 1, NULL);
	threadHandle = CreateThread(NULL, 0, starterFunction, this, 0, &myID);
}

void Thread::start() {
	isStarted = true;
	ReleaseSemaphore(startSemHandle, 1, NULL);
}

void Thread::waitToComplete() {
	if (!isStarted) {
		ReleaseSemaphore(startSemHandle, 1, NULL);
	}
	WaitForSingleObject(threadHandle, INFINITE);
}

Thread::~Thread() {
	waitToComplete();
	CloseHandle(startSemHandle);
	CloseHandle(threadHandle);
}

RequestHandler::RequestHandler() : numOfRequests(0), isStopped(false) {
	requestSemHandle = CreateSemaphore(NULL, 0, _CRT_INT_MAX, NULL);
	InitializeCriticalSection(&criticalSection);
}

RequestHandler::~RequestHandler() {
	waitToComplete();
	CloseHandle(requestSemHandle);
	DeleteCriticalSection(&criticalSection);
}

void RequestHandler::putRequest() {
	EnterCriticalSection(&criticalSection);
	numOfRequests++;
	LeaveCriticalSection(&criticalSection);
	ReleaseSemaphore(requestSemHandle, 1, NULL);
}

void RequestHandler::stop() {
	EnterCriticalSection(&criticalSection);
	isStopped = true;
	LeaveCriticalSection(&criticalSection);
	ReleaseSemaphore(requestSemHandle, 1, NULL);
}

void RequestHandler::run() {
	while (true) {
		bool flag = false;
		WaitForSingleObject(requestSemHandle, INFINITE);
		EnterCriticalSection(&criticalSection);
		if (isStopped && numOfRequests == 0) {
			flag = true;
			//std::cout << "HANDLER STOPPED" << std::endl;
		}
		if (numOfRequests > 0) {
			numOfRequests--;
		}
		LeaveCriticalSection(&criticalSection);
		if (flag) {
			break;
		}
		//template method
		handleRequest();
	}
}