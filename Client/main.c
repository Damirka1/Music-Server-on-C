#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "../Database/Author.h"
#include "../Database/Track.h"


int Connect(int* sockfd, struct addrinfo* servinfo)
{
	if((*sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
	{
		close(*sockfd);
		perror("Can't create server socket");
		return -1;
	}

	if(connect(*sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		close(*sockfd);
		perror("Can't connect to server");
		return -1;
	}

	fcntl(*sockfd, F_SETFL, O_NONBLOCK);

	return 0;
}

struct Vector* LoadBuffer(int* sockfd, void* buffer, int* buffer_size, char* fsize)
{
	long long filesize = 0;
	memcpy(&filesize, fsize + 1, sizeof(long long));
	printf("Size of file = %d\n", filesize);

	struct Vector* vec = CreateVector(filesize);

	vec->pData = malloc(filesize);
	vec->DataSize = filesize;

	printf("Create vector\n");

	long long it = 0;
	long long readsize = filesize;

	if(readsize > *buffer_size)
		readsize = *buffer_size;

	while(it != filesize)
	{
		Recieve(sockfd, readsize, buffer, buffer_size);
		if(it + readsize > filesize)
		{
			readsize = filesize - it;
			if(readsize < 0)
				readsize = filesize;
				
		}
		memcpy(vec->pData + it, buffer, readsize);
		it += readsize;
	}

	vec->Size = 1;
	return vec;

}

ClearBuffer(void* buffer, int* buffer_size)
{
	memset(buffer, 0, *buffer_size);
}


int Recieve(int* sockfd, int count, void* buffer, int* buffer_size)
{
	ClearBuffer(buffer, buffer_size);

	int numbytes = recv(*sockfd, buffer, count, 0);
	int attemps = 0;
	if(numbytes <= 0)
	{
		perror("Can't recieve data");
		return -1;
	}

	while(numbytes != count)
	{
		int r = recv(*sockfd, buffer + numbytes, *buffer_size - numbytes, 0);
		if(r == -1)
		{
			perror("Can't recieve data in cycle");
			return -1;
		}
		else if(r == 0)
		{
			if(attemps++ == 1000)
				return -1;
			sleep(1);
			continue;
		}
		numbytes += r;
	}
	printf("end recv %d\n", numbytes);
	return numbytes;
}

int Send(int* sockfd, int count, void* buffer, int* buffer_size)
{
	if(count > *buffer_size)
		count = *buffer_size;

	int numbytes = send(*sockfd, buffer, count, 0);
	int attemps = 0;
	if(numbytes <= 0)
	{
		perror("Can't send data");
		return -1;
	}
	printf("count = %d\n", numbytes);

	while(numbytes != count)
	{
		int r = send(*sockfd, buffer + numbytes, *buffer_size - numbytes, 0);
		if(r == -1)
		{
			perror("Can't send data in cycle");
			return -1;
		}
		else if(r == 0)
		{
			if(attemps++ == 3)
				return -1;
			sleep(1);
			continue;
		}
		numbytes += r;
	}
	printf("Data sended, count = %d\n", numbytes);
	ClearBuffer(buffer, buffer_size);
	return numbytes;
}

int sockfd = 0;
int buffer_command_size = 2048; // 2kb for commands
int buffer_size = 131072; // 128 kb for data
void* buffer;

void ExecuteCommands()
{
	ClearBuffer(buffer, &buffer_size);
	Recieve(&sockfd, buffer_command_size, buffer, &buffer_size);

	if(strlen(buffer) == 0)
		return;
	
	printf("Response from server is %s\n", buffer);

	char* saveptr;
	char* token = strtok_r(buffer, " ", &saveptr);

	if(strcmp(buffer, "file") == 0)
	{
		printf("Downloading file\n");
		char* fsize = buffer + strlen(token) + 1;
		long long filesize = 0;
		memcpy(&filesize, fsize, sizeof(long long));
		printf("Size of file = %lld\n", filesize);

		//void* f = malloc(filesize);

		FILE* file = fopen("get", "wb");

		long long it = 0;
		long long readsize = filesize;

		if(readsize > buffer_size)
			readsize = buffer_size;
		
		while(it != filesize)
		{
			if(it + readsize > filesize)
			{
				readsize = filesize - it;
				if(readsize < 0)
					readsize = filesize;
			}

			Recieve(&sockfd, readsize, buffer, &buffer_size);
			//memcpy(f + it, buffer, buffer_size);
			fwrite(buffer, readsize, 1, file);
			it += readsize;
		}

		//fwrite(f, filesize, 1, file);
		
		fclose(file);
		printf("File downloaded!\n");
	}
	else if(strcmp(token, "streamdata") == 0)
	{
		char* fsize = buffer + strlen(token);
		struct Vector* vec = LoadBuffer(&sockfd, buffer, &buffer_size, fsize);
		printf("size %d, stream size %d\n", vec->Size, vec->DataSize);
		
		int it = 0;
		while(it < vec->DataSize)
			putchar(((char*)vec->pData)[it++]);
		printf("\n");
		Release(vec);
	}
}


int main(int argc, char** argv)
{
	struct addrinfo hints, *servinfo;
	int rv;
	char* ip = "192.168.1.30";
	char* port = "25565";

	buffer = malloc(buffer_size);

	memset(buffer, 0, buffer_size);
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP

	if((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinof: %s\n", gai_strerror(rv));
		return -1;
	}

	

	Connect(&sockfd, servinfo);
	
	gets(buffer);

	while(strcmp(buffer, "logout") != 0)
	{
		Send(&sockfd, buffer_command_size, buffer, &buffer_size);

		if(strcmp(buffer, "exit") == 0)
			return 0;
		
		ExecuteCommands();
		gets(buffer);
	}



}


