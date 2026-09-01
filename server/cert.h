#include <openssl/evp.h>
#include <openssl/x509.h>

EVP_PKEY *generate_server_key();

X509_REQ *generate_server_csr(EVP_PKEY *server_key);

void save_server_key(EVP_PKEY *server_key);

void save_server_certificate(X509 *server_cert);