CC = gcc
CFLAGS = -Wall -g

all: client server
	+$(MAKE) -C SDStore-transf

client: client.o

client.o: client.c

server: server.o queue.o

server.o: server.c queue.c

clean:
	+$(MAKE) -C SDStore-transf clean
	rm -f server_client_fifo client.o server.o