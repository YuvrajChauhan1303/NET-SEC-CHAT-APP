#include <openssl/evp.h>
#include <openssl/x509.h>

int rsa_generate_csr(EVP_PKEY *private_key, const char *username, X509_REQ **csr);
int rsa_save_csr(X509_REQ *csr, const char *path);
int rsa_load_csr(const char *path, X509_REQ **csr);

int cert_save(X509 *certificate, const char *path);
int cert_load(const char *path, X509 **certificate);

int cert_verify(X509 *certificate, X509 *ca_certificate);
EVP_PKEY *cert_get_public_key(X509 *certificate);