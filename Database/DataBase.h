#ifndef DATABASE_HEADER
#define DATABASE_HEADER
#include "Author.h"

int Save();

int Load();

struct Music* FindMusic(char* message);

struct Album* FindAlbum(char* message);

struct Author* findAuthor(char* message);

void UploadInMemory();

void* GetAll();
unsigned long* GetSize();

struct Vector* GetTracks();

void ReleaseAllData();

void Test(void* pData, unsigned long dSize);

#endif
