#include <openssl/evp.h>
#include <openssl/x509.h>

EVP_PKEY *generate_ca_key();
X509 *generate_ca_certificate(EVP_PKEY *key);

void save_ca_key(EVP_PKEY *key);
void save_ca_certificate(X509 *cert);

X509 *sign_server_csr(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *csr);