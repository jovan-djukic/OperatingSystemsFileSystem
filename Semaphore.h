#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <Windows.h>

class Semaphore {
public:
	Semaphore(int, int);
	void wait();
	void signal();
	void signal(int);
	~Semaphore();
private:
	HANDLE semHandle;
};

#endif