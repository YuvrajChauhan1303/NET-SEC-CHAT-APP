#include <stdio.h>
#include <string.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "services.h"

struct User users[MAX_USERS];
int user_count = 0;

int register_client(char *username, int socket, EVP_PKEY *public_key, X509 *certificate)
{
    if (find_user(username) != -1)
        return -1;

    strcpy(users[user_count].username, username);
    users[user_count].socket = socket;
    users[user_count].public_key = public_key;
    users[user_count].certificate = certificate;

    user_count++;

    return 0;
}

int find_user(char *username)
{
    for (int i = 0; i < user_count; i++)
    {
        if (strcmp(users[i].username, username) == 0)
            return i;
    }

    return -1;
}

void print_users(void)
{
    for (int i = 0; i < user_count; i++)
    {
        printf("Username: %s\n", users[i].username);
        printf("Public Key:\n");
        PEM_write_PUBKEY(stdout, users[i].public_key);
        printf("Certificate:\n");
        PEM_write_X509(stdout, users[i].certificate);
        printf("\n");
    }
}

void remove_client(int index)
{
    EVP_PKEY_free(users[index].public_key);
    X509_free(users[index].certificate);

    users[index] = users[user_count - 1];

    user_count--;
}

EVP_PKEY *generate_ca_key()
{
    EVP_PKEY *key = NULL;
    EVP_PKEY_CTX *ctx;

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY_keygen(ctx, &key);
    EVP_PKEY_CTX_free(ctx);

    return key;
}

X509 *generate_ca_certificate(EVP_PKEY *key)
{
    X509 *cert;
    X509_NAME *name;

    cert = X509_new();

    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);
    X509_set_pubkey(cert, key);

    name = X509_get_subject_name(cert);

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"IN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"NET-SEC-CHAT-APP", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"NET-SEC-CHAT-APP Root CA", -1, -1, 0);

    X509_set_issuer_name(cert, name);
    X509_sign(cert, key, EVP_sha256());

    return cert;
}

void save_ca_key(EVP_PKEY *key)
{
    FILE *f;

    f = fopen("cert-key/cert-auth.key", "wb");
    PEM_write_PrivateKey_ex(f, key, NULL, NULL, 0, NULL, NULL, NULL, NULL);
    fclose(f);
}

void save_ca_certificate(X509 *cert)
{
    FILE *f;

    f = fopen("cert-key/cert-auth.crt", "wb");
    PEM_write_X509(f, cert);
    fclose(f);
}

int verify_csr(X509_REQ *csr)
{
    EVP_PKEY *key;
    int result;

    key = X509_REQ_get_pubkey(csr);
    result = X509_REQ_verify(csr, key);

    EVP_PKEY_free(key);

    return result;
}

X509 *sign_csr(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *csr)
{
    X509 *cert;
    EVP_PKEY *client_key;

    cert = X509_new();

    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), user_count + 2);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);

    X509_set_subject_name(cert, X509_REQ_get_subject_name(csr));
    X509_set_issuer_name(cert, X509_get_subject_name(ca_cert));

    client_key = X509_REQ_get_pubkey(csr);

    X509_set_pubkey(cert, client_key);

    EVP_PKEY_free(client_key);

    X509_sign(cert, ca_key, EVP_sha256());

    return cert;
}