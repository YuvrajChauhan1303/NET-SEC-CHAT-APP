all: client server

client: client/client.c
	gcc client/client.c -o client/client

server: server/server.c
	gcc server/server.c -o server/server

clean:
	rm -f client/client server/server