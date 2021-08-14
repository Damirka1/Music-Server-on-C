#ifndef FILESTREAM_HEADER
#define FILESTREAM_HEADER

struct Stream
{
	char path[1024];
	FILE* stream;
	long long filesize;
	long long position;
	int readsize;
};

#endif
