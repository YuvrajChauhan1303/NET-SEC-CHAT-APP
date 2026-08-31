OPENSSL = /opt/homebrew/opt/openssl@3

CFLAGS = -I$(OPENSSL)/include
LDFLAGS = -L$(OPENSSL)/lib
LIBS = -lcrypto

ca: cert-auth.c services.c services.h
	gcc cert-auth.c services.c -o ca $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o *.out ca