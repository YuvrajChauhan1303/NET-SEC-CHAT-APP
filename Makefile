OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include

LDFLAGS = -L$(OPENSSL)/lib

LIBS = -lcrypto

all: client_bin server_bin cert_auth

client_bin: client/client.c client/dh.c client/aes.c client/services.c client/cert.c
	gcc $(CFLAGS) client/client.c client/dh.c client/aes.c client/services.c client/cert.c -o client/client $(LDFLAGS) $(LIBS)

server_bin: server/server.c server/services.c server/users.c server/chat.c server/dh.c server/aes.c server/cert.c
	gcc $(CFLAGS) server/server.c server/services.c server/users.c server/chat.c server/dh.c server/aes.c server/cert.c -o server/server $(LDFLAGS) $(LIBS)

cert_auth: cert-auth/cert-auth.c cert-auth/services.c
	gcc $(CFLAGS) cert-auth/cert-auth.c cert-auth/services.c -o cert-auth/ca $(LDFLAGS) $(LIBS)

clean:
	rm -f client/client server/server cert-auth/ca
	rm -f server/server-key/server.key server/server-key/server.crt
	rm -f cert-auth/ca-key/ca.key cert-auth/ca-key/ca.crt