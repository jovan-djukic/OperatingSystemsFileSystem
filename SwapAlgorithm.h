#ifndef _SWAP_ALGORITHM_H_
#define _SWAP_ALGORITHM_H_

#include "Thread.h"
#include "BoundedBuffer.h"
#include <Windows.h>

class SwapAlgorithm {
public:
	virtual void update(int) = 0;
	virtual int getSwapEntry() = 0;
	virtual ~SwapAlgorithm() = 0;
};

class LRU :public SwapAlgorithm {
public:
	LRU(int);
	void update(int) override;
	int getSwapEntry() override;
	~LRU();
private:
	struct Entry {
		int next, previous;
	};
	int capacity, victim, lastAccessed, firstAccessed;
	const int null = -1;
	Entry *entries; 
};

struct LRURequest {
	enum CallID {UPDATE, GET_SWAP_ENTRY};
	CallID callID;
	int entry;
	HANDLE semHandle;
	LRURequest(CallID, int);
	LRURequest(CallID);
	~LRURequest();
};

typedef BoundedBuffer<LRURequest*> LRURequestBuffer;

class LRURequestHandler : public RequestHandler {
public:
	LRURequestHandler(LRURequestBuffer*, LRU*);
	~LRURequestHandler();
protected:
	void handleRequest() override;
private:
	LRURequestBuffer *buffer;
	LRU *lru;
};

class ThreadSafeLRU : public SwapAlgorithm {
public:
	ThreadSafeLRU(int,int);
	void update(int) override;
	int getSwapEntry() override;
	~ThreadSafeLRU();
private:
	RequestHandler *handler;
	LRURequestBuffer *buffer;
	LRU *lru;
};

#endif