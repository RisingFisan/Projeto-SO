CC = gcc
CFLAGS = -O2 -Wall

all: argus server

argus: client.o
	$(CC) -o $@ $<

server: server.o
	$(CC) -o $@ $<

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf argus server *.o log* server_client_fifo client_server_fifo