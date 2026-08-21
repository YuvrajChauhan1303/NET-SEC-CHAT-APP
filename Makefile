all: client_bin server_bin

client_bin: client/client.c
	gcc client/client.c -o client/client

server_bin: server/server.c
	gcc server/server.c server/services.c -o server/server

clean:
	rm -f client/client server/server