CC = gcc
CFLAGS = -Wall -g

all: client server
	+$(MAKE) -C SDStore-transf

client: client.o

client.o: client.c

server: server.o

server.o: server.c

clean:
	+$(MAKE) -C SDStore-transf clean
	rm -f server_client_fifo client.o server.o