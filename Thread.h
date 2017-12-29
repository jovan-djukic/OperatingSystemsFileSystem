#ifndef _THREAD_H_
#define _THREAD_H_

#include <Windows.h>

class Thread {
public:
	Thread();
	void start();
	virtual void run() {}
	void waitToComplete();
	virtual ~Thread();
private:
	static DWORD WINAPI starterFunction(LPVOID);

	bool isStarted;
	HANDLE threadHandle, startSemHandle;
	DWORD myID;
};

class RequestHandler : public Thread {
public:
	RequestHandler();
	void putRequest();
	void stop();
	virtual ~RequestHandler();
protected:
	virtual void handleRequest() = 0;
	void run() override;
private:
	CRITICAL_SECTION criticalSection;
	HANDLE requestSemHandle;
	int numOfRequests;
	bool isStopped;
};

#endif