CC = g++
CFLAGS = -std=c++11

all: client_main.cpp server_main.cpp server.o client.o
	$(CC) $(CFLAGS) client_main.cpp client.o -o client
	$(CC) $(CFLAGS) server_main.cpp server.o -o server

server.o: server.cpp server.h epoll.h
	$(CC) $(CFLAGS) -c server.cpp

client.o: client.cpp client.h epoll.h
	$(CC) $(CFLAGS) -c client.cpp

clean:
	rm -f *.o server client
