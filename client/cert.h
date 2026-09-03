#include <openssl/evp.h>
#include <openssl/x509.h>

X509 *download_ca_certificate();
int validate_server_certificate(X509 *server_cert, X509 *ca_cert);
int generate_challenge(unsigned char *challenge);
int verify_challenge(X509 *server_cert, unsigned char *challenge, int challenge_len, unsigned char *signature, int signature_len);

EVP_PKEY *generate_client_keys();
X509_REQ *generate_client_csr(EVP_PKEY *client_key, const char *username);

int save_client_key(EVP_PKEY *client_key);
int save_client_csr(X509_REQ *csr);
int save_client_certificate(X509 *client_cert);
X509 *request_signed_certificate(X509_REQ *csr);

EVP_PKEY *load_client_key();
X509 *load_client_certificate();