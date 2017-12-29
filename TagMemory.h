#ifndef _TAG_MEMORY_H_
#define _TAG_MEMORY_H_

class TagMemory {
public:
	TagMemory(int);
	int compare(unsigned long,int) const;
	void write(int, unsigned long);
	unsigned long get(int) const;
	~TagMemory();
private:
	unsigned long *tags;
	int capacity;
};

#endif