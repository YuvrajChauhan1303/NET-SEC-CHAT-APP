#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "rsa.h"
#include "cert.h"

#define CA_IP "127.0.0.1"
#define CA_PORT 8080

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <username>\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];

    char private_key_path[100];
    char public_key_path[100];
    char csr_path[100];
    char cert_path[100];

    sprintf(private_key_path, "%s-private.pem", username);
    sprintf(public_key_path, "%s-public.pem", username);
    sprintf(csr_path, "%s.csr", username);
    sprintf(cert_path, "%s.crt", username);

    EVP_PKEY *private_key = NULL;
    X509_REQ *csr = NULL;
    X509 *certificate = NULL;
    X509 *ca_certificate = NULL;

    int sock;
    struct sockaddr_in addr;

    uint32_t csr_len;
    uint32_t cert_len;

    unsigned char *csr_data;
    unsigned char *cert_data;

    int csr_der_len;

    rsa_generate_keypair(private_key_path, public_key_path);

    printf("RSA key pair generated for %s\n", username);

    rsa_load_private_key(private_key_path, &private_key);

    printf("Private key loaded\n");

    rsa_generate_csr(private_key, username, &csr);

    printf("CSR generated\n");

    rsa_save_csr(csr, csr_path);

    printf("CSR saved to %s\n", csr_path);

    csr_der_len = i2d_X509_REQ(csr, NULL);

    csr_data = malloc(csr_der_len);

    unsigned char *p = csr_data;

    i2d_X509_REQ(csr, &p);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(CA_PORT);
    inet_pton(AF_INET, CA_IP, &addr.sin_addr);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    printf("Connected to cert-auth\n");

    csr_len = htonl(csr_der_len);

    write(sock, &csr_len, sizeof(csr_len));
    write(sock, csr_data, csr_der_len);

    printf("CSR sent to cert-auth\n");

    read(sock, &cert_len, sizeof(cert_len));

    cert_len = ntohl(cert_len);

    cert_data = malloc(cert_len);

    read(sock, cert_data, cert_len);

    const unsigned char *q = cert_data;

    certificate = d2i_X509(NULL, &q, cert_len);

    printf("Certificate received\n");

    cert_save(certificate, cert_path);

    printf("Certificate saved to %s\n", cert_path);

    cert_load("../cert-auth/cert-key/cert-auth.crt", &ca_certificate);

    printf("Verifying certificate...\n");

    if (cert_verify(certificate, ca_certificate) != 1)
    {
        printf("Certificate verification FAILED\n");

        X509_free(certificate);
        X509_free(ca_certificate);
        X509_REQ_free(csr);
        EVP_PKEY_free(private_key);
        free(csr_data);
        free(cert_data);
        close(sock);

        return 1;
    }

    printf("Certificate verification SUCCESSFUL\n");

    printf("\nCertificate:\n");
    PEM_write_X509(stdout, certificate);

    while (1)
    {
        char command[100];

        printf("\nCA> ");
        fgets(command, sizeof(command), stdin);

        command[strcspn(command, "\n")] = '\0';

        write(sock, command, strlen(command));

        if (!strcmp(command, "/quit"))
            break;

        if (!strncmp(command, "/give ", 6))
        {
            uint32_t response_len;

            read(sock, &response_len, sizeof(response_len));

            response_len = ntohl(response_len);

            unsigned char *response_data = malloc(response_len);

            read(sock, response_data, response_len);

            const unsigned char *r = response_data;

            X509 *response_cert = d2i_X509(NULL, &r, response_len);

            printf("\nVerifying received certificate...\n");

            if (cert_verify(response_cert, ca_certificate) != 1)
            {
                printf("Certificate verification FAILED\n");

                X509_free(response_cert);
                free(response_data);

                continue;
            }

            printf("Certificate verification SUCCESSFUL\n");

            printf("\nReceived certificate:\n");

            PEM_write_X509(stdout, response_cert);

            X509_free(response_cert);

            free(response_data);
        }
    }

    close(sock);

    free(csr_data);
    free(cert_data);

    X509_free(certificate);
    X509_free(ca_certificate);
    X509_REQ_free(csr);
    EVP_PKEY_free(private_key);

    return 0;
}