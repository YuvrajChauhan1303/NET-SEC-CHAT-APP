#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include "services.h"

int main()
{
    EVP_PKEY *ca_key;
    X509 *ca_cert;
    int s, c;
    struct sockaddr_in addr = {0};

    ca_key = generate_ca_key();
    ca_cert = generate_ca_certificate(ca_key);
    save_ca_key(ca_key);
    save_ca_certificate(ca_cert);

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 10);

    printf("[CA] CA Initialized. Listening for certificate requests.\n\n");

    while (1)
    {
        c = accept(s, NULL, NULL);

        printf("[CA] New connection. Socket: %d\n", c);

        uint32_t request;
        read(c, &request, sizeof(request));

        if (request == 1)
        {
            printf("[CA] CA certificate requested.\n");

            int cert_len = i2d_X509(ca_cert, NULL);
            unsigned char *cert_data = malloc(cert_len);
            unsigned char *p = cert_data;

            i2d_X509(ca_cert, &p);

            uint32_t send_len = cert_len;

            write(c, &send_len, sizeof(send_len));
            write(c, cert_data, cert_len);

            free(cert_data);

            printf("[CA] CA certificate sent.\n");
        }
        else if (request == 2)
        {
            printf("[CA] Server CSR received.\n");

            uint32_t csr_len;

            read(c, &csr_len, sizeof(csr_len));

            unsigned char *csr_data = malloc(csr_len);

            read(c, csr_data, csr_len);

            const unsigned char *p = csr_data;

            X509_REQ *csr = d2i_X509_REQ(NULL, &p, csr_len);

            free(csr_data);

            X509_NAME *subject = X509_REQ_get_subject_name(csr);

            char username[MAX_USERNAME];

            X509_NAME_get_text_by_NID(subject, NID_commonName, username, sizeof(username));

            X509 *server_cert = sign_server_csr(ca_key, ca_cert, csr);

            int index = find_user_certificate(username);

            if (index == -1)
            {
                strcpy(users[user_count].username, username);
                users[user_count].certificate = server_cert;
                user_count++;
            }
            else
            {
                X509_free(users[index].certificate);
                users[index].certificate = server_cert;
            }

            save_user_certificate(username, server_cert);

            int cert_len = i2d_X509(server_cert, NULL);
            unsigned char *cert_data = malloc(cert_len);
            unsigned char *q = cert_data;

            i2d_X509(server_cert, &q);

            uint32_t send_len = cert_len;

            write(c, &send_len, sizeof(send_len));
            write(c, cert_data, cert_len);

            free(cert_data);

            X509_REQ_free(csr);

            printf("[CA] Certificate for %s sent.\n", username);
        }
        else if (request == 3)
        {
            uint32_t username_len;

            read(c, &username_len, sizeof(username_len));

            char username[MAX_USERNAME];

            read(c, username, username_len);

            username[username_len] = '\0';

            int index = find_user_certificate(username);

            if (index == -1)
            {
                uint32_t cert_len = 0;

                write(c, &cert_len, sizeof(cert_len));
            }
            else
            {
                int cert_len = i2d_X509(users[index].certificate, NULL);
                unsigned char *cert_data = malloc(cert_len);
                unsigned char *p = cert_data;

                i2d_X509(users[index].certificate, &p);

                uint32_t send_len = cert_len;

                write(c, &send_len, sizeof(send_len));
                write(c, cert_data, cert_len);

                free(cert_data);
            }
        }
        else if (request == 4)
        {
            uint32_t username_len;

            read(c, &username_len, sizeof(username_len));

            char username[MAX_USERNAME];

            read(c, username, username_len);

            username[username_len] = '\0';

            delete_user_certificate(username);
        }

        close(c);
    }

    close(s);

    EVP_PKEY_free(ca_key);
    X509_free(ca_cert);

    return 0;
}