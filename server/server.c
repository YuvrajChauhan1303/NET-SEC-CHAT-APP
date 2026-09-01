#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include "users.h"
#include "chat.h"
#include "services.h"
#include "dh.h"
#include "aes.h"
#include "cert.h"

int main()
{
    EVP_PKEY *server_key;
    X509_REQ *csr;
    X509 *server_cert;

    int s, c;
    struct sockaddr_in addr = {0};

    server_key = generate_server_key();
    save_server_key(server_key);

    csr = generate_server_csr(server_key);

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ca = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in ca_addr;
    ca_addr.sin_family = AF_INET;
    ca_addr.sin_addr.s_addr = INADDR_ANY;
    ca_addr.sin_port = htons(8081);

    connect(ca, (struct sockaddr *)&ca_addr, sizeof(ca_addr));

    int csr_len = i2d_X509_REQ(csr, NULL);

    unsigned char *csr_data = malloc(csr_len);

    unsigned char *p = csr_data;

    i2d_X509_REQ(csr, &p);

    uint32_t request = 2;

    write(ca, &request, sizeof(request));
    write(ca, &csr_len, sizeof(csr_len));
    write(ca, csr_data, csr_len);

    free(csr_data);

    uint32_t cert_len;

    read(ca, &cert_len, sizeof(cert_len));

    unsigned char *cert_data = malloc(cert_len);

    read(ca, cert_data, cert_len);

    const unsigned char *q = cert_data;

    server_cert = d2i_X509(NULL, &q, cert_len);

    free(cert_data);

    close(ca);

    save_server_certificate(server_cert);

    X509_REQ_free(csr);

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 10);

    printf("[SERVER] Server Initialized. Listening for requests.\n\n");

    while (1)
    {
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        int max_fd = s;

        for (int i = 0; i < user_count; i++)
        {
            FD_SET(users[i].socket, &readfds);

            if (users[i].socket > max_fd)
                max_fd = users[i].socket;
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(s, &readfds))
        {
            c = accept(s, NULL, NULL);

            printf("[SERVER] New connection. Socket: %d\n", c);

            int send_cert_len = i2d_X509(server_cert, NULL);

            unsigned char *send_cert_data = malloc(send_cert_len);

            unsigned char *r = send_cert_data;

            i2d_X509(server_cert, &r);

            uint32_t len = send_cert_len;

            write(c, &len, sizeof(len));

            write(c, send_cert_data, send_cert_len);

            free(send_cert_data);

            init_dh_params();

            if (register_client(c))
                printf("[SERVER] Registration complete.\n");
        }

        for (int i = 0; i < user_count; i++)
        {
            int client_socket = users[i].socket;

            if (!FD_ISSET(client_socket, &readfds))
                continue;

            unsigned char plaintext[4096];

            int plaintext_len = receive_command(client_socket, users[i].KEY, (char *)plaintext, sizeof(plaintext));

            if (plaintext_len <= 0)
            {
                service_quit(i);
                i--;
                continue;
            }

            printf("[CLIENT %s] %s\n", users[i].username, plaintext);

            if (!strcmp((char *)plaintext, "/who"))
            {
                printf("[SERVER] %s requested /who\n", users[i].username);
                service_who(client_socket);
            }
            else if (!strncmp((char *)plaintext, "/chat ", 6))
            {
                service_chat(i, (char *)plaintext);
            }
            else if (!strcmp((char *)plaintext, "/quit"))
            {
                service_quit(i);
                i--;
                continue;
            }
            else if (plaintext[0] == '@')
            {
                char username[MAX_USERNAME];

                get_username((char *)plaintext, username);

                service_chat_username(i, username);

                char *msg = (char *)plaintext + strlen(username) + 2;

                service_message(i, msg);
            }
            else
            {
                service_message(i, (char *)plaintext);
            }
        }
    }

    close(s);
    free_dh_params();

    X509_free(server_cert);
    EVP_PKEY_free(server_key);

    return 0;
}