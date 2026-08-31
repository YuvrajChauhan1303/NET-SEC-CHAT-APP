OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include
LDFLAGS = -L$(OPENSSL)/lib
LIBS = -lcrypto

all: ca server client

ca: cert-auth.c services.c services.h
	gcc cert-auth.c services.c -o ca $(CFLAGS) $(LDFLAGS) $(LIBS)

server: test-server.c server-services.c server-services.h
	gcc test-server.c server-services.c -o server $(CFLAGS) $(LDFLAGS) $(LIBS)

client: test-client.c client-services.c client-services.h
	gcc test-client.c client-services.c -o client $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f ca server client
	rm -f cert-key/cert-auth.key cert-key/cert-auth.crt
	rm -f server-key/server.key server-key/server.crt
	rm -f received.crt