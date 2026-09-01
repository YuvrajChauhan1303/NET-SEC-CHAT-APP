#include <openssl/evp.h>
#include <openssl/x509.h>

X509 *download_ca_certificate();

int validate_server_certificate(X509 *server_cert, X509 *ca_cert);