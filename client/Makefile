OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include

LDFLAGS = -L$(OPENSSL)/lib

LIBS = -lcrypto

rsa: rsa.c rsa.h test-rsa.c
	gcc rsa.c test-rsa.c -o rsa $(CFLAGS) $(LDFLAGS) $(LIBS)

cert-rsa: rsa.c rsa.h cert.c cert.h test-rsa-cert.c
	gcc rsa.c cert.c test-rsa-cert.c -o cert-rsa $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f rsa cert-rsa test_crypto client
	rm -f *.pem *.crt *.csr