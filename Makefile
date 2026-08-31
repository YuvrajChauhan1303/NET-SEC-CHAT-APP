OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include
LDFLAGS = -L$(OPENSSL)/lib
LIBS = -lcrypto

all: client_bin server_bin

client_bin: client/client.c client/dh.c client/crypto.c
	gcc $(CFLAGS) client/client.c client/dh.c client/crypto.c \
	-o client/client $(LDFLAGS) $(LIBS)

server_bin: server/server.c server/services.c server/users.c server/chat.c server/dh.c
	gcc $(CFLAGS) server/server.c server/services.c server/users.c server/chat.c server/dh.c \
	-o server/server $(LDFLAGS) $(LIBS)

clean:
	rm -f client/client server/server