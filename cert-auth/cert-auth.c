#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include "services.h"

#define CA_PORT 8080

int main()
{
    EVP_PKEY *ca_key;
    X509 *ca_cert;

    int s;
    int client;

    struct sockaddr_in addr;

    ca_key = generate_ca_key();

    ca_cert = generate_ca_certificate(ca_key);

    save_ca_key(ca_key);

    save_ca_certificate(ca_cert);

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(CA_PORT);

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    listen(s, 5);

    while (1)
    {
        client = accept(s, NULL, NULL);

        uint32_t csr_len;

        read(client, &csr_len, sizeof(csr_len));

        unsigned char *csr_data = malloc(csr_len);

        read(client, csr_data, csr_len);

        const unsigned char *p = csr_data;

        X509_REQ *csr = d2i_X509_REQ(NULL, &p, csr_len);

        free(csr_data);

        X509 *server_cert = sign_server_csr(ca_key, ca_cert, csr);

        int cert_len = i2d_X509(server_cert, NULL);

        unsigned char *cert_data = malloc(cert_len);

        unsigned char *q = cert_data;

        i2d_X509(server_cert, &q);

        uint32_t send_len = cert_len;

        write(client, &send_len, sizeof(send_len));

        write(client, cert_data, cert_len);

        free(cert_data);

        X509_free(server_cert);
        X509_REQ_free(csr);

        close(client);
    }

    return 0;
}