#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include "services.h"

UserCertificate users[MAX_USERS];
int user_count = 0;

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
    PEM_write_PrivateKey(f, key, NULL, NULL, 0, NULL, NULL);
    fclose(f);
}

void save_ca_certificate(X509 *cert)
{
    FILE *f;

    f = fopen("cert-key/cert-auth.crt", "wb");
    PEM_write_X509(f, cert);
    fclose(f);
}

X509 *sign_server_csr(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *csr)
{
    X509 *cert;
    EVP_PKEY *server_key;

    cert = X509_new();

    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 2);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);

    X509_set_subject_name(cert, X509_REQ_get_subject_name(csr));
    X509_set_issuer_name(cert, X509_get_subject_name(ca_cert));

    server_key = X509_REQ_get_pubkey(csr);

    X509_set_pubkey(cert, server_key);

    EVP_PKEY_free(server_key);

    X509_sign(cert, ca_key, EVP_sha256());

    return cert;
}

void save_user_certificate(const char *username, X509 *cert)
{
    char path[MAX_USERNAME + 100];
    FILE *f;

    snprintf(path, sizeof(path), "cert-key/users/%s.crt", username);

    f = fopen(path, "wb");
    PEM_write_X509(f, cert);
    fclose(f);
}

int find_user_certificate(const char *username)
{
    int i;

    for (i = 0; i < user_count; i++)
    {
        if (strcmp(users[i].username, username) == 0)
            return i;
    }

    return -1;
}

void delete_user_certificate(const char *username)
{
    int index;
    int i;
    char path[MAX_USERNAME + 100];

    index = find_user_certificate(username);

    if (index == -1)
        return;

    X509_free(users[index].certificate);

    snprintf(path, sizeof(path), "cert-key/users/%s.crt", username);
    unlink(path);

    for (i = index; i < user_count - 1; i++)
        users[i] = users[i + 1];

    user_count--;

    printf("[CA] Certificate deleted for user: %s\n", username);
}