#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include "client-services.h"

#define SERVER_PORT 8081

int main()
{
    int server_socket;

    struct sockaddr_in server_addr;

    X509 *ca_cert;
    X509 *server_cert;

    ca_cert = load_ca_certificate();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Connected to server\n");

    uint32_t cert_len;

    read(server_socket, &cert_len, sizeof(cert_len));

    unsigned char *cert_data = malloc(cert_len);

    read(server_socket, cert_data, cert_len);

    const unsigned char *p = cert_data;

    server_cert = d2i_X509(NULL, &p, cert_len);

    free(cert_data);

    printf("Server certificate received\n");

    int result = validate_server_certificate(server_cert, ca_cert);

    if (result != 1)
    {
        printf("Certificate invalid\n");
        printf("Connection terminated\n");

        X509_free(server_cert);
        X509_free(ca_cert);
        close(server_socket);

        return 1;
    }

    printf("Certificate valid\n");

    while (1)
    {
        char buf[1024];

        int n = read(server_socket, buf, sizeof(buf) - 1);

        if (n <= 0)
            break;

        buf[n] = '\0';

        printf("Server: %s\n", buf);

        printf("> ");

        fgets(buf, sizeof(buf), stdin);

        buf[strlen(buf) - 1] = '\0';

        write(server_socket, buf, strlen(buf));

        if (!strcmp(buf, "/quit"))
            break;
    }

    X509_free(server_cert);
    X509_free(ca_cert);

    close(server_socket);

    return 0;
}