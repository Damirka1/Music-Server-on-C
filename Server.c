#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>

#include "Server.h"
#include "Database/DataBase.h"
#include "Database/FileStream.h"


struct
{
	struct pollfd c[50];
	int timeouts[50];
} Connections;

struct Vector* streams;
struct Vector* connections;

int sockfd;
int running = 0;
int buffer_command_size = 2048; // 2 kb for commands
int buffer_size = 262144; // 256 kb for data
char* buffer;
int listen_count = 50;

pthread_t qthread;

int StartServer()
{
	printf("Creating server\n");
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *servinfo;
	int new_fd;
	int rv;
	buffer = malloc(buffer_size);

	memset(buffer, 0, buffer_size);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // Set ipv4
	hints.ai_socktype = SOCK_STREAM; // Set TCP type
	hints.ai_flags = AI_PASSIVE; // Set ip of host

	if((rv = getaddrinfo(NULL, "25565", &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
	{
		perror("Can't create server socket");
		return -1;
	}

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	int opt = 1;

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		perror("setsockopt");
		return -1;
	}


	if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		close(sockfd);
		perror("bind error");
		return -1;
	}
	
	
	if(listen(sockfd, listen_count) == -1)
	{
		perror("listen error");
		return -1;
	}

	running = 1;

	Load();
	UploadInMemory();

	streams = CreateVector(sizeof(struct Stream));
	//connections = CreateVector(sizeof(struct pollfd));

	printf("Server started!\n");
	if(pthread_create(&qthread, NULL, &QueueProccessor, NULL) != 0)
	{
		perror("Can't create queue thread");
		close(sockfd);
		return -1;
	}

	while(running)
	{
		addr_size = sizeof(their_addr);
		if((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size)) == -1)
		{
			sleep(1);
			continue;
		}

		for(int i = 0; i < listen_count; i++)
		{
			if(Connections.c[i].fd == 0)
			{
				Connections.c[i].fd = new_fd;
				Connections.c[i].events = POLLIN;
				Connections.timeouts[i] = 0;
				printf("Added new connection to the queue\n");
				printf("New socket is %d, at %d\n", new_fd, i);
				break;
			}
		}

		//struct pollfd* pollfd = malloc(sizeof(pollfd));
		//pollfd->fd = new_fd;
		//pollfd->events = POLLIN;

		//Push(connections, pollfd);
	}
	
	pthread_join(qthread, NULL);
	
	close(sockfd);

	return 0;

}

void ClearBuffer()
{
	memset(buffer, 0, buffer_size);
}


int Recieve(int* new_fd, int count)
{
	ClearBuffer();
	int numbytes = 0;
	int attemps = 0;

	while(numbytes != count)
	{
		int r = recv(*new_fd, buffer + numbytes, count - numbytes, 0);
		if(r == -1)
		{
			//perror("Can't recieve data in cycle");
			return -1;
		}
		else if(r == 0)
		{
			if(attemps++ == 5)
				return -1;
			usleep(100);
			continue;
		}

		numbytes += r;
	}

	printf("Recieved %d\n", numbytes);

	return numbytes;
}

int Send(int* new_fd, int count)
{
	int attemps = 0;
	int numbytes = 0;

	while(numbytes < count)
	{
		int r = send(*new_fd, buffer + numbytes, count - numbytes, MSG_CONFIRM);
		if(r == -1)
		{
			perror("Can't send data in cycle");
			return -1;
		}
		else if(r == 0)
		{
			if(attemps++ == 5)
				return -1;
			usleep(100);
			continue;
		}

		numbytes += r;
	}

	printf("Data sended %d\n", numbytes);

	ClearBuffer();
	return numbytes;
}


void QueueProccessor()
{
	printf("Thread started!\n");
	while(running)
	{
		int poll_count = poll(Connections.c, listen_count, 1);

		if (poll_count == -1)
		{
			perror("poll");
			continue;
		}

		for(int i = 0; i < listen_count; i++)
		{
			if(Connections.c[i].fd == 0)
				continue;

			struct pollfd* pfds = &Connections.c[i];
			if(pfds->revents & POLLIN)
			{
				if(Recieve(&pfds->fd, buffer_command_size) > 0)
				{
					Connections.timeouts[i] = 0;
					if(ExecuteCommands(&pfds->fd) == -1)
						break;
				}
			}
			else
			{
				if(Connections.timeouts[i]++ == 5000)
				{
					printf("Close connections %d, at %d\n", Connections.c[i].fd, i);
					close(Connections.c[i].fd);
					Connections.c[i].fd = 0;
					Connections.timeouts[i] = 0;
				}

			}
		}
	}
	pthread_exit(NULL);
}




int ExecuteCommands(int* new_fd)
{
	if(strlen(buffer) == 0)
		return;

	//struct pollfd poll_send;
	//poll_send.fd = *new_fd;
	//poll_send.events = POLLOUT;

	//printf("buffer test\n");
	printf("Buffer is %s\n", buffer);

	//return;

	if(strcmp(buffer, "exit") == 0)
	{
		printf("Shutdown server\n");
		running = 0;
		strcpy(buffer, "success\n");
		Send(new_fd, buffer_command_size);
		ClearBuffer();
		return -1;
	}
	else if(strcmp(buffer, "close") == 0)
	{
		printf("Close connection %d\n", *new_fd);
		strcpy(buffer, "success\n");
		Send(new_fd, buffer_command_size);
		close(*new_fd);
		*new_fd = 0;
		return 0;
	}
	else if(strcmp(buffer, "loadbase") == 0)
	{
		Load();
		UploadInMemory();
		strcpy(buffer, "success\n");
		Send(new_fd, buffer_command_size);
		return 0;
	}
	else if(strcmp(buffer, "check") == 0)
	{
		strcpy(buffer, "success\n");
		Send(new_fd, buffer_command_size);
		return 0;
	}
	else if(strcmp(buffer, "test") == 0)
	{
		unsigned long datasize = *GetSize();
		void* pData = GetAll();
		Test(pData, datasize);
		printf("test finished!\n");
		return 0;
	}
	else if(strcmp(buffer, "gettracks") == 0)
	{
		ClearBuffer();
		strcpy(buffer, "tracks ");
		struct Vector* tracks = GetTracks();
		long long datasize = tracks->DataSize * tracks->Size;
		memcpy(buffer + strlen(buffer), &datasize, sizeof(long long));
		Send(new_fd, buffer_command_size);

		long long it = 0;
		long long copysize = datasize;
		printf("copy size %d\n", copysize);
		if(copysize > buffer_size)
			copysize = buffer_size;

		while(it != datasize)
		{
			ClearBuffer();
			if(it + copysize > datasize)
			{
				copysize = datasize - it;
				if(copysize < 0)
					copysize = datasize;
			}
			memcpy(buffer, tracks->pData + it, copysize);
			it += copysize;
			Send(new_fd, copysize);
		}
		ClearBuffer();
		return 0;
	}


	// TODO: write new split function
	char* saveptr;
	char* token = strtok_r(buffer, " ", &saveptr);

	printf("token = %s\n", token);

	if(strcmp(token, "getfile") == 0)
	{
		char* path = buffer + strlen(token) + 1;
		FILE* file = fopen(path, "rb");
		printf("Open file %s\n", path);

		if(file == NULL)
		{
			printf("File not found!\n");
			strcpy(buffer, "File not found!\n");
			strcpy(buffer + strlen(buffer), path);
			Send(new_fd, buffer_command_size);
			return 0;
		}
		fseek(file, 0L, SEEK_END);
		long long filesize = ftell(file);
		printf("Size of file = %lld\n", filesize);
		fseek(file, 0L, SEEK_SET);

		strcpy(buffer, "file ");

		memcpy(buffer + strlen(buffer), &filesize, sizeof(long long));

		Send(new_fd, buffer_command_size);

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

			fread(buffer, readsize, 1, file);
			Send(new_fd, readsize);
			it += readsize;
		}
		fclose(file);
	}
	else if(strcmp(token, "getsizeof") == 0)
	{
		char* path = buffer + strlen(token) + 1;
		if(strlen(path) == 0)
			return 0;
		
		struct Stream* file = NULL;

		for(int i = 0; i < streams->Size; i++)
		{
			struct Stream* stream = Get(streams, i);
			if(strcmp(stream->path, path) == 0)
			{
				file = stream;
				printf("Stream already exist, use it\n");
				break;
			}
		}

		if(file == 0)
		{
			file = malloc(sizeof(struct Stream));

			strcpy(file->path, path);

			file->stream = fopen(file->path, "rb");

			if(file->stream == 0)
			{
				printf("Can't open stream file\n");
				strcpy(buffer, "Stream not found!\n");
				strcpy(buffer + strlen(buffer), path);

				free(file);
				Send(new_fd, buffer_command_size);
				return 0;
			}

			fseek(file->stream, 0L, SEEK_END);
			file->filesize = ftell(file->stream);
			Push(streams, file);
		}
		
		strcpy(buffer, "filesize ");

		memcpy(buffer + strlen(buffer), &file->filesize, sizeof(long long));
			
		Send(new_fd, buffer_command_size);


	}
	else if(strcmp(token, "getstream") == 0)
	{
		char* path = strtok_r(NULL, ";", &saveptr);

		if(strlen(path) == 0)
			return 0;

		printf("path = %s\n", path);

		struct Stream* file = NULL;

		for(int i = 0; i < streams->Size; i++)
		{
			struct Stream* stream = Get(streams, i);
			if(strcmp(stream->path, path) == 0)
			{
				file = stream;
				printf("Stream already exist, use it\n");
				break;
			}
		}

		if(file == 0)
		{
			file = malloc(sizeof(struct Stream));

			strcpy(file->path, path);

			file->stream = fopen(file->path, "rb");

			if(file->stream == 0)
			{
				strcpy(buffer, "Stream not found!\n");
				strcpy(buffer + strlen(buffer), path);

				free(file);
				Send(new_fd, buffer_command_size);
				return 0;
			}

			fseek(file->stream, 0L, SEEK_END);
			file->filesize = ftell(file->stream);
			Push(streams, file);
		}

		char* pos = strtok_r(NULL, ";", &saveptr);
		char* size = pos + strlen(pos) + 1;

		file->position = strtoll(pos, NULL, 10);
		long long chunksize = strtoll(size, NULL, 10);

		ClearBuffer();

		printf("pos %d, size %d\n", file->position, chunksize);

		fseek(file->stream, file->position, SEEK_SET);
		strcpy(buffer, "streamdata ");

		//printf("Prints info\n");
		//int it3 = 0;
		//while(it3 < buffer_command_size)
		//{
		//	putchar(((char*)buffer)[it3++]);
		//}
		//printf("\n");

		memcpy(buffer + strlen(buffer), &chunksize, sizeof(long long));

		//printf("Prints info 2\n");
		//int it2 = 0;
		//while(it2 < buffer_command_size)
		//{
		//	putchar(((char*)buffer)[it2++]);
		//}
		//printf("\n");

		Send(new_fd, buffer_command_size);

		//if(file->position > file->filesize)
		//{
		//	memset(buffer, -1, buffer_command_size);
		//	Send(new_fd, buffer_command_size);
		//	return;
		//}

		long long it = 0;
		long long readsize = chunksize;

		if(readsize > buffer_size)
			readsize = buffer_size;

		while(it != chunksize)
		{
			if(it + readsize > file->filesize)
			{
				readsize = file->filesize - it;
				if(readsize < 0)
					readsize = file->filesize;
			}

			fread(buffer, readsize, 1, file->stream);
			Send(new_fd, readsize);
			it += readsize;
		}
	}
}
