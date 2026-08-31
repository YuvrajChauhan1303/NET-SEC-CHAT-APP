#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "cert.h"

int rsa_generate_csr(EVP_PKEY *private_key, const char *username, X509_REQ **csr)
{
    X509_REQ *req;
    X509_NAME *name;

    req = X509_REQ_new();

    X509_REQ_set_version(req, 0);

    name = X509_REQ_get_subject_name(req);

    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)username, -1, -1, 0);

    X509_REQ_set_pubkey(req, private_key);

    X509_REQ_sign(req, private_key, EVP_sha256());

    *csr = req;

    return 0;
}

int rsa_save_csr(X509_REQ *csr, const char *path)
{
    FILE *file;

    file = fopen(path, "wb");

    if (file == NULL)
        return -1;

    PEM_write_X509_REQ(file, csr);

    fclose(file);

    return 0;
}

int rsa_load_csr(const char *path, X509_REQ **csr)
{
    FILE *file;

    file = fopen(path, "rb");

    if (file == NULL)
        return -1;

    *csr = PEM_read_X509_REQ(file, NULL, NULL, NULL);

    fclose(file);

    if (*csr == NULL)
        return -1;

    return 0;
}

int cert_save(X509 *certificate, const char *path)
{
    FILE *file;

    file = fopen(path, "wb");

    if (file == NULL)
        return -1;

    PEM_write_X509(file, certificate);

    fclose(file);

    return 0;
}

int cert_load(const char *path, X509 **certificate)
{
    FILE *file;

    file = fopen(path, "rb");

    if (file == NULL)
        return -1;

    *certificate = PEM_read_X509(file, NULL, NULL, NULL);

    fclose(file);

    if (*certificate == NULL)
        return -1;

    return 0;
}

int cert_verify(X509 *certificate, X509 *ca_certificate)
{
    EVP_PKEY *ca_key;
    int result;

    ca_key = X509_get_pubkey(ca_certificate);

    result = X509_verify(certificate, ca_key);

    EVP_PKEY_free(ca_key);

    return result;
}

EVP_PKEY *cert_get_public_key(X509 *certificate)
{
    return X509_get_pubkey(certificate);
}