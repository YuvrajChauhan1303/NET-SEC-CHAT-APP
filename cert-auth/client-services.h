#include <openssl/x509.h>

X509 *load_ca_certificate();

int validate_server_certificate(X509 *server_cert, X509 *ca_cert);