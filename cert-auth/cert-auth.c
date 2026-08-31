#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include "services.h"

#define CA_PORT 8080

int main()
{
    int s, c;

    struct sockaddr_in addr = {0};

    char buf[1000];

    EVP_PKEY *ca_key;
    X509 *ca_cert;

    ca_key = generate_ca_key();
    ca_cert = generate_ca_certificate(ca_key);

    save_ca_key(ca_key);
    save_ca_certificate(ca_cert);

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(CA_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    listen(s, 10);

    printf("[CA] Cert-Auth Initialized. Listening for requests.\n\n");

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

            printf("[CA] New connection. Socket: %d\n", c);

            uint32_t csr_len;

            int n = read(c, &csr_len, sizeof(csr_len));

            if (n <= 0)
            {
                close(c);
                continue;
            }

            csr_len = ntohl(csr_len);

            unsigned char *csr_data = malloc(csr_len);

            read(c, csr_data, csr_len);

            const unsigned char *p = csr_data;

            X509_REQ *csr = d2i_X509_REQ(NULL, &p, csr_len);

            free(csr_data);

            if (verify_csr(csr) != 1)
            {
                printf("[CA] CSR verification failed.\n");

                X509_REQ_free(csr);
                close(c);

                continue;
            }

            printf("[CA] CSR verification successful.\n");

            X509_NAME *subject = X509_REQ_get_subject_name(csr);

            char username[MAX_USERNAME];

            X509_NAME_get_text_by_NID(subject, NID_commonName, username, MAX_USERNAME);

            if (find_user(username) != -1)
            {
                printf("[CA] Username already taken: %s\n", username);

                X509_REQ_free(csr);
                close(c);

                continue;
            }

            EVP_PKEY *client_key = X509_REQ_get_pubkey(csr);

            X509 *client_cert = sign_csr(ca_key, ca_cert, csr);

            register_client(username, c, client_key, client_cert);

            printf("[CA] Registered %s on socket %d\n", username, c);

            int cert_len = i2d_X509(client_cert, NULL);

            unsigned char *cert_data = malloc(cert_len);

            unsigned char *q = cert_data;

            i2d_X509(client_cert, &q);

            uint32_t send_len = htonl(cert_len);

            write(c, &send_len, sizeof(send_len));
            write(c, cert_data, cert_len);

            free(cert_data);

            X509_REQ_free(csr);

            printf("[CA] Certificate sent to %s\n", username);

            print_users();
        }

        for (int i = 0; i < user_count; i++)
        {
            int client_socket = users[i].socket;

            if (FD_ISSET(client_socket, &readfds))
            {
                int n = read(client_socket, buf, sizeof(buf) - 1);

                if (n <= 0)
                {
                    printf("[CA] %s disconnected.\n", users[i].username);

                    close(client_socket);
                    remove_client(i);

                    i--;

                    continue;
                }

                buf[n] = '\0';

                if (!strncmp(buf, "/give ", 6))
                {
                    char username[MAX_USERNAME];

                    strcpy(username, buf + 6);

                    username[strcspn(username, "\n")] = '\0';

                    int target = find_user(username);

                    if (target == -1)
                    {
                        char response[] = "User not found\n";

                        write(client_socket, response, strlen(response));

                        continue;
                    }

                    X509 *certificate = users[target].certificate;

                    int cert_len = i2d_X509(certificate, NULL);

                    unsigned char *cert_data = malloc(cert_len);

                    unsigned char *q = cert_data;

                    i2d_X509(certificate, &q);

                    uint32_t send_len = htonl(cert_len);

                    write(client_socket, &send_len, sizeof(send_len));
                    write(client_socket, cert_data, cert_len);

                    free(cert_data);

                    printf("[CA] Sent certificate of %s to %s\n", username, users[i].username);
                }

                else if (!strcmp(buf, "/quit"))
                {
                    printf("[CA] %s disconnected.\n", users[i].username);

                    close(client_socket);
                    remove_client(i);

                    i--;

                    continue;
                }
            }
        }
    }

    close(s);

    EVP_PKEY_free(ca_key);
    X509_free(ca_cert);

    return 0;
}