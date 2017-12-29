#include "TagMemory.h"

TagMemory::TagMemory(int _capacity) : capacity(_capacity) {
	tags = new unsigned long[capacity];
}

TagMemory::~TagMemory() {
	delete[] tags;
	tags = nullptr;
}

int TagMemory::compare(unsigned long tag, int sizeOfEntrie) const {
	for (int i = 0; i < capacity; i++) {
		if (tags[i] == ((tag / sizeOfEntrie) * sizeOfEntrie)) {
			return i * sizeOfEntrie + tag - tags[i];
		}
	}
	return -1; 
}

void TagMemory::write(int entrie, unsigned long tag) {
	tags[entrie] = tag;
}

unsigned long TagMemory::get(int entrie) const {
	return tags[entrie];
}