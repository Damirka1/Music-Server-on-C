#ifndef SERVER_HEADER
#define SERVER_HEADER

int StartServer();

long long Send(int* new_fd, long long count);
long long Recieve(int* new_fd, long long count);

void QueueProccessor();
int ExecuteCommands(int* new_fd);

#endif
