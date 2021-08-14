
build: Server.c
	gcc -Wall -pthread -O3 *.c Database/*.c -o Server
