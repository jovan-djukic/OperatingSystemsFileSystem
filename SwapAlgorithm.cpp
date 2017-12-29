#include "SwapAlgorithm.h"
#include <Windows.h>

SwapAlgorithm::~SwapAlgorithm() {

}

LRU::LRU(int _capacity) : capacity(_capacity), victim(0), firstAccessed(null), lastAccessed(null) {
	entries = new Entry[capacity];
	for (int i = 0; i < capacity; i++) {
		entries[i].next = entries[i].previous = null;
	}
}

LRU::~LRU() {
	delete[] entries;
}

int LRU::getSwapEntry() {
	if (victim < capacity) {
		return victim++;
	} else {
		return lastAccessed;
	}
}

void LRU::update(int entrie) {
	if (entrie != firstAccessed) {
		if (entrie != lastAccessed) {
			if (entries[entrie].next != null) {
				entries[entries[entrie].next].previous = entries[entrie].previous;
			}
			if (entries[entrie].previous != null) {
				entries[entries[entrie].previous].next = entries[entrie].next;
			}
		} else {
			lastAccessed = entries[entrie].next;
			entries[lastAccessed].previous = null;
		}

		entries[entrie].previous = firstAccessed;
		entries[firstAccessed].next = entrie;
		firstAccessed = entrie;
		entries[firstAccessed].next = null;
	}
}

LRURequest::LRURequest(CallID _callID, int _entry) : callID(_callID), entry(_entry), semHandle(NULL){
}

LRURequest::LRURequest(CallID _callID) : callID(_callID), entry(-1) {
	semHandle = CreateSemaphore(NULL, 0, 1, NULL);
}

LRURequest::~LRURequest() {
	if (semHandle != NULL) {
		CloseHandle(semHandle);
	}
}

LRURequestHandler::LRURequestHandler(LRURequestBuffer *_buffer, LRU *_lru) : buffer(_buffer), lru(_lru) {

}

LRURequestHandler::~LRURequestHandler() {
	waitToComplete();
}

void LRURequestHandler::handleRequest() {
	LRURequest *request = buffer->get();
	if (request->callID == LRURequest::UPDATE) {
		lru->update(request->entry);
		delete request;
		//request handler is responsible for deallocatio
	}
	else if (request->callID == LRURequest::GET_SWAP_ENTRY) {
		request->entry = lru->getSwapEntry();
		ReleaseSemaphore(request->semHandle, 1, NULL);
		//user threads are responsible for deallocation beacuse they will be blocked 
	}
}

ThreadSafeLRU::ThreadSafeLRU(int _lruCapacity, int _bufferCapacity) {
	lru = new LRU(_lruCapacity);
	buffer = new LRURequestBuffer(_bufferCapacity);
	//creating and starting handler thread
	handler = new LRURequestHandler(buffer, lru);
	handler->start();
}

ThreadSafeLRU::~ThreadSafeLRU() {
	handler->stop();
	delete handler;
	delete buffer;
	delete lru;
}

void ThreadSafeLRU::update(int entry) {
	LRURequest *request = new LRURequest(LRURequest::UPDATE, entry);
	buffer->put(request);
	handler->putRequest();
}

int ThreadSafeLRU::getSwapEntry() {
	LRURequest *request = new LRURequest(LRURequest::GET_SWAP_ENTRY);
	buffer->put(request);
	handler->putRequest();
	WaitForSingleObject(request->semHandle, INFINITE);
	int retEntry = request->entry;
	delete request;
	return retEntry;
}