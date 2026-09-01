#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

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