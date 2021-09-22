#ifndef MUSIC_HEADER
#define MUSIC_HEADER

struct Music
{
	char Title[128];
	char Genre[128];
	char Length[16];
	struct Album* Album;
};

#endif
