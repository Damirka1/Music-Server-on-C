#ifndef ALBUM_HEADER
#define ALBUM_HEADER
#include "Music.h"
#include "Vector.h"

struct Album
{
	char Title[128];
	char Length[16];
	char BigImage[16];
	char LittleImage[16];
	char* Author;
	struct Vector* Musics;
};

#endif
