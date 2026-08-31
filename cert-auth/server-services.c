#include <stdio.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "server-services.h"

EVP_PKEY *generate_server_key()
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

X509_REQ *generate_server_csr(EVP_PKEY *server_key)
{
    X509_REQ *csr;
    X509_NAME *name;

    csr = X509_REQ_new();

    X509_REQ_set_version(csr, 0);

    name = X509_REQ_get_subject_name(csr);

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"IN", -1, -1, 0);

    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"NET-SEC-CHAT-APP", -1, -1, 0);

    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);

    X509_REQ_set_pubkey(csr, server_key);

    X509_REQ_sign(csr, server_key, EVP_sha256());

    return csr;
}

void save_server_key(EVP_PKEY *server_key)
{
    FILE *f;

    f = fopen("server-key/server.key", "wb");

    PEM_write_PrivateKey(f, server_key, NULL, NULL, 0, NULL, NULL);

    fclose(f);
}

void save_server_certificate(X509 *server_cert)
{
    FILE *f;

    f = fopen("server-key/server.crt", "wb");

    PEM_write_X509(f, server_cert);

    fclose(f);
}