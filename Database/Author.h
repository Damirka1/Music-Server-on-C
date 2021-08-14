#ifndef AUTHOR_HEADER
#define AUTHOR_HEADER
#include "Album.h"
#include "Vector.h"

struct Author
{
	char Name[128];
	struct Vector* Albums;
};

#endif
