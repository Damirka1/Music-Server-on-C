#include "DataBase.h"
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "Vector.h"
#include "Track.h"

struct Vector* Authors;
struct Vector* Tracks;
char RootPath[1024] = "Database/Musics";
char TrackFile[] = "Track.mp3";
long long Id = 0;

enum Dirs
{
	Author,
	Album,
	Music,
	File,
};
int _CheckDirs(int* it, char* Directory)
{
	int dir_count = 0;
	struct dirent* dent;
     	DIR* srcdir = opendir(Directory);

    	if (srcdir == NULL)
    	{
       		perror("opendir");
		return -1;
	}

	while((dent = readdir(srcdir)) != NULL)
    	{
        	struct stat st;

        	if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            		continue;

        	if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
        	{
            		perror(dent->d_name);
            		continue;
        	}

        	if (S_ISDIR(st.st_mode))
		{

			if(*it == Author)
			{
				// Load authors.
				//printf("Author: %s\n", dent->d_name);
				struct Author Ar;
				memset(Ar.Name, 0, 128);
				memcpy(Ar.Name, dent->d_name, 128);

				Ar.Albums = CreateVector(sizeof(struct Album));
				Push(Authors, &Ar);
			}
			else if (*it == Album)
			{
				// Load albums.
				//printf("Album: %s\n", dent->d_name);
				struct Album Am;
					
				memset(Am.Title, 0, 128);
				memcpy(Am.Title, dent->d_name, 128);
					
				Am.Musics = CreateVector(sizeof(struct Music));
				struct Author* Ar = Get(Authors, Authors->Size - 1);
				Am.Author = Ar->Name;

				Push(Ar->Albums, &Am);

			}
			else if(*it == Music)
			{
				// load music file
				printf("Music: %s\n", dent->d_name);
				struct Music Mc;
				memset(Mc.Title, 0, 128);
				memcpy(Mc.Title, dent->d_name, 128);

				struct Author* Ar = Get(Authors, Authors->Size - 1);
				struct Album* Am = Get(Ar->Albums, Ar->Albums->Size - 1);
				Mc.Album = Am;
				Push(Am->Musics, &Mc);

				struct Track Tr;
				memset(Tr.Path, 0, sizeof(Tr.Path));
				memcpy(Tr.Path, RootPath, strlen(RootPath));
				Tr.Path[strlen(Tr.Path)] = '/';
				memcpy(Tr.Path + strlen(Tr.Path), dent->d_name, strlen(dent->d_name));
				Tr.Path[strlen(Tr.Path)] = '/';
				memcpy(Tr.Path + strlen(Tr.Path), TrackFile, strlen(TrackFile));
				memcpy(Tr.Title, Mc.Title, sizeof(Mc.Title));
				memcpy(Tr.Author, Ar->Name, sizeof(Ar->Name));
				Tr.Id = Id++;
			
				Push(Tracks, &Tr);
			}


			short len = strlen(Directory);
			memcpy(Directory + len + 1, dent->d_name, strlen(dent->d_name));
			Directory[len] = '/';

			*it += 1;
			_CheckDirs(it, Directory);

			memset(Directory + len, 0, 1024 - len);
			*it -= 1;
		}
		else
		{
			if(*it == File)
			{
				// load track's info.

			}
			else if (*it == Music)
			{
				// load album's images.
			}
		}
    	}
    	closedir(srcdir);
    	return dir_count;
}

int Load()
{
	printf("Loading data base\n");
	Authors = CreateVector(sizeof(struct Author));
	Tracks = CreateVector(sizeof(struct Track));
	int it = 0;
	_CheckDirs(&it, RootPath);
	printf("Data base is loaded!\n");
	return 0;
}


void* AllData;
unsigned long DataSize;

void UploadInMemory()
{
	if(DataSize)
		free(AllData);

	DataSize = 0;

	for(int i = 0; i < Authors->Size; i++)
	{
		struct Author* Ar = Get(Authors, i);
		//printf("Author: %s\n", Ar->Name);

		// Add size of author's name.
		DataSize += sizeof(struct Author) - sizeof(void*);

		DataSize += 4; // Add 4 bytes for 2 shorts for type and size

		for(int j = 0; j < Ar->Albums->Size; j++)
		{
			struct Album* Am = Get(Ar->Albums, j);
			// Add size of Album struct without pointers.
			DataSize += sizeof(struct Album) - (sizeof(void*) * 2);
			DataSize += 4;

			for(int k = 0; k < Am->Musics->Size; k++)
			{
				struct Music* Mc = Get(Am->Musics, k);
				DataSize += sizeof(struct Music) - sizeof(void*);
				DataSize += 4;
			}
		}
	}

	//printf("%d\n", DataSize);

	AllData = malloc(DataSize);
	unsigned long It = 0;

	for(int i = 0; i < Authors->Size; i++)
	{
		struct Author* Ar = Get(Authors, i);

		// Add size of author's name.
		int Size = sizeof(struct Author) - sizeof(void*);

		//printf("Author: %s, size: %d\n", Ar->Name, Size);
		short type = Author;
		short size = Size;
		
		memcpy(AllData + It, &type, 2);
		memcpy(AllData + It + 2, &size, 2);
		memcpy(AllData + It + 4, Ar->Name, Size);
	
		It += Size;
		It += 4; // add 2 short for type and size
		
		for(int j = 0; j < Ar->Albums->Size; j++)
		{
			struct Album* Am = Get(Ar->Albums, j);
		
			// Add size of Album struct without pointers.
			Size = sizeof(struct Album) - (sizeof(void*) * 2);

			//printf("Album: %s, %d\n", Am->Title, Size);

			short type = Album;
			short size = Size;

			memcpy(AllData + It, &type, 2);
			memcpy(AllData + It + 2, &size, 2);
			memcpy(AllData + It + 4, Am->Title, Size);

			It += Size;
			It += 4;
			
			for(int k = 0; k < Am->Musics->Size; k++)
			{
				struct Music* Mc = Get(Am->Musics, k);

				Size = sizeof(struct Music) - sizeof(void*);
				short type = File;
				short size = Size;

				//printf("Music: %s, size: %d\n", Mc->Title, Size);
				memcpy(AllData + It, &type, 2);
				memcpy(AllData + It + 2, &size, 2);
				memcpy(AllData + It + 4, Mc, Size);
						
				It += Size;
				It += 4;
			}
		}
	}
}


void Test(void* pData, unsigned long dSize)
{
	/*
	unsigned long it = 0;
	short type, size;

	printf("Datasize: %ld\n", dSize);
	int MusicIt = 0;

	while (it != dSize)
	{
		memcpy(&type, pData + it, sizeof(short));
		memcpy(&size, pData + it + 2, sizeof(short));
		
		//printf("type %d, size %d\n", type, size);
		
		MusicIt = 0;

		switch(type)
		{
			case Author:
			{
				struct Author* Ar = malloc(sizeof(struct Author));
				memcpy(Ar, pData + it + 4, size);

				printf("Author %s\n", Ar->Name);
				Ar->Albums = pData + it + 4 + size; 

				free(Ar);
				break;
			}
			case Album:
			{
				struct Album* Am = malloc(sizeof(struct Album));
				memcpy(Am, pData + it + 4, size);
				printf("Album %s\n", Am->Title);

				Am->Author = pData + it + 4 - size;
				Am->Musics = pData + it + 4 + size;

				free(Am);
				break;
			}
			case File:
			{
				struct Music* Mc = malloc(sizeof(struct Music));
				memcpy(Mc, pData + it + 4, size);
				printf("Music %s\n", Mc->Title);

				Mc->Album = pData + it + 4 - (size * MusicIt++);
				free(Mc);
				break;
			}
		}
		it += 4 + size;
	}
	
	printf("Iterator: %ld\n", it);
	*/


	for(int i = 0; i < Tracks->Size; i++)
	{
		struct Track* Tr = Get(Tracks, i);
		
		printf("Id: %i\n", Tr->Id);
		printf("Path: %s\n", Tr->Path);
		printf("Title: %s\n", Tr->Title);
		printf("Author: %s\n", Tr->Author);
	}

}

void* GetAll()
{
	return AllData;
}

unsigned long* GetSize()
{
	return &DataSize;
}


int Save()
{
	if(DataSize)
	{
		FILE* file = fopen("Database/Data", "wb");
		fwrite(AllData, DataSize, 1, file);
		fclose(file);
	}
}


struct Vector* GetTracks()
{
	return Tracks;
}

void ReleaseAllData()
{
	if(DataSize)
		free(AllData);

	Release(Tracks);

}
