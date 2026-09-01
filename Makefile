OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include -Icrypto
LDFLAGS = -L$(OPENSSL)/lib
LIBS = -lcrypto

all: client_bin server_bin test_crypto_bin

client_bin: client/client.c client/dh.c crypto/crypto.c
	gcc $(CFLAGS) client/client.c client/dh.c crypto/crypto.c \
	-o client/client $(LDFLAGS) $(LIBS)

server_bin: server/server.c server/services.c server/users.c server/chat.c server/dh.c crypto/crypto.c
	gcc $(CFLAGS) server/server.c server/services.c server/users.c server/chat.c server/dh.c crypto/crypto.c \
	-o server/server $(LDFLAGS) $(LIBS)

test_crypto_bin: crypto/test_crypto.c crypto/crypto.c
	gcc $(CFLAGS) crypto/test_crypto.c crypto/crypto.c \
	-o crypto/test_crypto $(LDFLAGS) $(LIBS)

clean:
	rm -f client/client server/server crypto/test_crypto