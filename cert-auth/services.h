#include <openssl/evp.h>
#include <openssl/x509.h>

#define MAX_USERS 1000
#define MAX_USERNAME 20

struct User
{
    char username[MAX_USERNAME];
    int socket;
    EVP_PKEY *public_key;
    X509 *certificate;
};

extern struct User users[MAX_USERS];
extern int user_count;

int register_client(char *username, int socket, EVP_PKEY *public_key, X509 *certificate);
int find_user(char *username);
void print_users(void);
void remove_client(int index);

EVP_PKEY *generate_ca_key();
X509 *generate_ca_certificate(EVP_PKEY *key);

void save_ca_key(EVP_PKEY *key);
void save_ca_certificate(X509 *cert);

int verify_csr(X509_REQ *csr);
X509 *sign_csr(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *csr);