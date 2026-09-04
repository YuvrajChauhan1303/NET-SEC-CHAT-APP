#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rand.h>

#include "cert.h"

X509 *download_ca_certificate()
{
    int ca;
    struct sockaddr_in ca_addr;

    ca = socket(AF_INET, SOCK_STREAM, 0);

    ca_addr.sin_family = AF_INET;
    ca_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ca_addr.sin_port = htons(8081);

    connect(ca, (struct sockaddr *)&ca_addr, sizeof(ca_addr));

    uint32_t request = 1;

    write(ca, &request, sizeof(request));

    uint32_t cert_len;

    read(ca, &cert_len, sizeof(cert_len));

    unsigned char *cert_data = malloc(cert_len);

    read(ca, cert_data, cert_len);

    const unsigned char *p = cert_data;

    X509 *ca_cert = d2i_X509(NULL, &p, cert_len);

    free(cert_data);

    close(ca);

    return ca_cert;
}

int validate_server_certificate(X509 *server_cert, X509 *ca_cert)
{
    EVP_PKEY *ca_key;
    int result;
    char cn[256];

    if (ca_cert == NULL || server_cert == NULL)
        return 0;

    ca_key = X509_get_pubkey(ca_cert);

    if (ca_key == NULL)
        return 0;

    result = X509_verify(server_cert, ca_key);

    EVP_PKEY_free(ca_key);

    if (result != 1)
        return 0;

    if (X509_cmp_current_time(X509_get0_notBefore(server_cert)) > 0)
        return 0;

    if (X509_cmp_current_time(X509_get0_notAfter(server_cert)) < 0)
        return 0;

    X509_NAME_get_text_by_NID(X509_get_subject_name(server_cert), NID_commonName, cn, sizeof(cn));

    if (strcmp(cn, "chat-app-server") != 0)
        return 0;

    return 1;
}

int generate_challenge(unsigned char *challenge)
{
    RAND_bytes(challenge, 32);

    return 32;
}
int verify_challenge(X509 *server_cert, unsigned char *challenge, int challenge_len, unsigned char *dh_share, int dh_share_len, unsigned char *signature, int signature_len)
{
    EVP_PKEY *server_key;
    EVP_MD_CTX *ctx;
    int result;

    server_key = X509_get_pubkey(server_cert);

    ctx = EVP_MD_CTX_new();

    EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, server_key);
    EVP_DigestVerifyUpdate(ctx, challenge, challenge_len);
    EVP_DigestVerifyUpdate(ctx, dh_share, dh_share_len);

    result = EVP_DigestVerifyFinal(ctx, signature, signature_len);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(server_key);

    return result;
}