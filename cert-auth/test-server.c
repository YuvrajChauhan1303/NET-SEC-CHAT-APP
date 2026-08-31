#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include "server-services.h"

#define CA_PORT 8080
#define SERVER_PORT 8081

int main()
{
    EVP_PKEY *server_key;
    X509_REQ *csr;
    X509 *server_cert;

    int ca_socket;
    int server_fd;
    int client_fd;

    struct sockaddr_in ca_addr;
    struct sockaddr_in server_addr;

    server_key = generate_server_key();

    save_server_key(server_key);

    csr = generate_server_csr(server_key);

    ca_socket = socket(AF_INET, SOCK_STREAM, 0);

    ca_addr.sin_family = AF_INET;
    ca_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ca_addr.sin_port = htons(CA_PORT);

    connect(ca_socket, (struct sockaddr *)&ca_addr, sizeof(ca_addr));

    int csr_len = i2d_X509_REQ(csr, NULL);

    unsigned char *csr_data = malloc(csr_len);

    unsigned char *p = csr_data;

    i2d_X509_REQ(csr, &p);

    uint32_t send_len = csr_len;

    write(ca_socket, &send_len, sizeof(send_len));

    write(ca_socket, csr_data, csr_len);

    free(csr_data);

    uint32_t cert_len;

    read(ca_socket, &cert_len, sizeof(cert_len));

    unsigned char *cert_data = malloc(cert_len);

    read(ca_socket, cert_data, cert_len);

    const unsigned char *q = cert_data;

    server_cert = d2i_X509(NULL, &q, cert_len);

    free(cert_data);

    close(ca_socket);

    save_server_certificate(server_cert);

    X509_REQ_free(csr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_fd, 5);

    printf("Server listening on port %d\n", SERVER_PORT);

    while (1)
    {
        client_fd = accept(server_fd, NULL, NULL);

        int send_cert_len = i2d_X509(server_cert, NULL);

        unsigned char *send_cert_data = malloc(send_cert_len);

        unsigned char *r = send_cert_data;

        i2d_X509(server_cert, &r);

        uint32_t len = send_cert_len;

        write(client_fd, &len, sizeof(len));

        write(client_fd, send_cert_data, send_cert_len);

        free(send_cert_data);

        close(client_fd);
    }

    X509_free(server_cert);
    EVP_PKEY_free(server_key);

    return 0;
}