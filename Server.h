#ifndef SERVER_HEADER
#define SERVER_HEADER

int StartServer();

int Send(int* new_fd, int count);
int Recieve(int* new_fd, int count);

void QueueProccessor();
int ExecuteCommands(int* new_fd);

#endif
