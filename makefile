
build: Server.c
	gcc -Wall -pthread -O3 *.c Database/*.c -o Server

run: 
	./Server 2>&1 > log.file &
