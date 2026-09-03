#define MAX_USERS 1000
#define MAX_USERNAME 20

#include <openssl/evp.h>
#include <openssl/x509.h>

typedef struct
{
    char username[MAX_USERNAME];
    X509 *certificate;
} UserCertificate;

extern UserCertificate users[MAX_USERS];
extern int user_count;

EVP_PKEY *generate_ca_key();
X509 *generate_ca_certificate(EVP_PKEY *key);
void save_ca_key(EVP_PKEY *key);
void save_ca_certificate(X509 *cert);
X509 *sign_server_csr(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *csr);
void save_user_certificate(const char *username, X509 *cert);
int find_user_certificate(const char *username);
void delete_user_certificate(const char *username);