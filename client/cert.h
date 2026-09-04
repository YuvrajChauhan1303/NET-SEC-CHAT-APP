#include <openssl/evp.h>
#include <openssl/x509.h>

X509 *download_ca_certificate();

int validate_server_certificate(X509 *server_cert, X509 *ca_cert);

int generate_challenge(unsigned char *challenge);

int verify_challenge(X509 *server_cert, unsigned char *challenge, int challenge_len, unsigned char *dh_share, int dh_share_len, unsigned char *signature, int signature_len);