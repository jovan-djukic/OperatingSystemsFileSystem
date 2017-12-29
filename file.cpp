#include "file.h"
#include"fs.h"
#include "KernelFile.h"

File::File() {

}

char File::write(BytesCnt cnt, char *buffer) {
	return myImpl->write(cnt, buffer);
}

BytesCnt File::read(BytesCnt cnt, char *buffer) {
	return myImpl->read(cnt, buffer);
}

char File::seek(BytesCnt newPostion) {
	return myImpl->seek(newPostion);
}

BytesCnt File::filePos() {
	return myImpl->filepos();
}

char File::eof() {
	return myImpl->eof();
}

BytesCnt File::getFileSize() {
	return myImpl->getFileSize();
}

char File::truncate() {
	return myImpl->truncate();
}

File::~File() {
	delete myImpl;
}