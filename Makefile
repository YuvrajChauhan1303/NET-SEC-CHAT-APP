all: client_bin server_bin

client_bin: client/client.c
	gcc client/client.c client/dh.c  -o client/client -lcrypto

server_bin: server/server.c server/services.c server/users.c server/chat.c
	gcc server/server.c server/services.c server/users.c server/chat.c server/dh.c -o server/server -lcrypto

clean:
	rm -f client/client server/server