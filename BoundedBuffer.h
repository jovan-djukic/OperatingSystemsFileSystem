#ifndef _BOUNDED_BUFFER_H_
#define _BOUNDED_BUFFER_H_

#include <Windows.h>

template <typename T>
class BoundedBuffer {
public:
	BoundedBuffer(int);
	void put(T);
	T get();
	~BoundedBuffer();
private:
	T *buffer;
	int head, tail, capacity;
	CRITICAL_SECTION headCriticalSection, tailCriticalSection;
	HANDLE itemAvailable, spaceAvailable;
};

template<typename T>
BoundedBuffer<T>::BoundedBuffer(int _capacity) : head(0), tail(0), capacity(_capacity) {
	buffer = new T[capacity];
	InitializeCriticalSection(&headCriticalSection);
	InitializeCriticalSection(&tailCriticalSection);
	itemAvailable = CreateSemaphore(NULL, 0, capacity, NULL);
	spaceAvailable = CreateSemaphore(NULL, capacity, capacity, NULL);
}

template<typename T>
void BoundedBuffer<T>::put(T data) {
	WaitForSingleObject(spaceAvailable, INFINITE);
		EnterCriticalSection(&headCriticalSection);
			buffer[head] = data;
			head = (head + 1) % capacity;
		LeaveCriticalSection(&headCriticalSection);
	ReleaseSemaphore(itemAvailable, 1, NULL);
}

template<typename T>
T BoundedBuffer<T>::get() {
	WaitForSingleObject(itemAvailable, INFINITE);
		EnterCriticalSection(&tailCriticalSection);
			T retData = buffer[tail];
			tail = (tail + 1) % capacity;
		LeaveCriticalSection(&tailCriticalSection);
	ReleaseSemaphore(spaceAvailable, 1, NULL);
	return retData;
}

template<typename T>
BoundedBuffer<T>::~BoundedBuffer() {
	DeleteCriticalSection(&headCriticalSection);
	DeleteCriticalSection(&tailCriticalSection);
	CloseHandle(itemAvailable);
	CloseHandle(spaceAvailable);
}

#endif