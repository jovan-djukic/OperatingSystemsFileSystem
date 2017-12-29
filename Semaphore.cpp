#include "Semaphore.h"
#include <Windows.h>

Semaphore::Semaphore(int initValue, int maxValue) {
	semHandle = CreateSemaphore(NULL, initValue, maxValue, NULL);
}

void Semaphore::wait() {
	WaitForSingleObject(semHandle, INFINITE);
}

void Semaphore::signal() {
	ReleaseSemaphore(semHandle, 1, NULL);
}

void Semaphore::signal(int count) {
	ReleaseSemaphore(semHandle, count, NULL);
}

Semaphore::~Semaphore() {
	CloseHandle(semHandle);
}