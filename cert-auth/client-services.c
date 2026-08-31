#include <stdio.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "client-services.h"

X509 *load_ca_certificate()
{
    FILE *f;

    f = fopen("cert-key/cert-auth.crt", "rb");

    X509 *cert = PEM_read_X509(f, NULL, NULL, NULL);

    fclose(f);

    return cert;
}

int validate_server_certificate(X509 *server_cert, X509 *ca_cert)
{
    EVP_PKEY *ca_key;
    int result;

    ca_key = X509_get_pubkey(ca_cert);

    result = X509_verify(server_cert, ca_key);

    EVP_PKEY_free(ca_key);

    if (result != 1)
        return 0;

    if (X509_cmp_current_time(X509_get0_notBefore(server_cert)) > 0)
        return 0;

    if (X509_cmp_current_time(X509_get0_notAfter(server_cert)) < 0)
        return 0;

    return 1;
}