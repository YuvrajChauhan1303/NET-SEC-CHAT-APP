#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

#include "services.h"
#include "dh.h"
#include "aes.h"
#include "cert.h"

#include "rsa.h"

int main(int argc, char *argv[])
{

    printf("Enter your username: ");

    char username[1000];
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    EVP_PKEY *client_key = generate_client_keys();
    X509_REQ *client_csr = generate_client_csr(client_key, username);

    save_client_key(client_key);
    save_client_csr(client_csr);

    X509 *client_cert = request_signed_certificate(client_csr);

    save_client_certificate(client_cert);

    X509_free(client_cert);
    X509_REQ_free(client_csr);
    EVP_PKEY_free(client_key);

    int s;
    struct addrinfo hints, *res;

    char *host = "127.0.0.1";
    char *port = "8080";

    if (argc >= 2)
        host = argv[1];

    if (argc >= 3)
        port = argv[2];

    X509 *ca_cert = download_ca_certificate();

    printf("[CLIENT] CA certificate downloaded.\n");

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host, port, &hints, &res);

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    init_dh_params();

    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *client_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();

    char x[513];

    char hexa[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    // printf("\n\nclient key:\n%s\n\n", x);

    BN_hex2bn(&client_sec, x);

    // printf("\n\nclient key (after conv):\n");
    // BN_print_fp(stdout, client_sec);

    sq_mult(client_sec, share, ctx);

    // printf("\n\nclient share:\n");
    // BN_print_fp(stdout, share);
    // printf("\n");

    connect(s, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    printf("[CLIENT] Connected to server.\n");

    uint32_t cert_len;

    read(s, &cert_len, sizeof(cert_len));

    unsigned char *cert_data = malloc(cert_len);

    read(s, cert_data, cert_len);

    const unsigned char *p = cert_data;

    X509 *server_cert = d2i_X509(NULL, &p, cert_len);

    free(cert_data);

    printf("[CLIENT] Server certificate received.\n");

    if (!validate_server_certificate(server_cert, ca_cert))
    {
        printf("[CLIENT] Server certificate validation failed.\n");
        printf("[CLIENT] Closing connection.\n");

        close(s);

        X509_free(ca_cert);
        X509_free(server_cert);

        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);

        free_dh_params();

        return 1;
    }

    printf("[CLIENT] Server certificate verified successfully.\n");

    unsigned char challenge[32];

    generate_challenge(challenge);
    write(s, challenge, 32);

    uint32_t signature_len;

    read(s, &signature_len, sizeof(signature_len));

    unsigned char signature[256];

    read(s, signature, signature_len);

    if (!verify_challenge(server_cert, challenge, 32, signature, signature_len))
    {
        printf("[CLIENT] Server proof-of-possession failed.\n");
        close(s);
        return 1;
    }

    printf("[CLIENT] Server proof-of-possession verified.\n");

    char buf[1000];

    char *share_hex = BN_bn2hex(share);

    strcpy(buf, share_hex);

    OPENSSL_free(share_hex);

    write(s, buf, 513);

    int n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    // printf("Server: %s\n", buf);

    BIGNUM *server_share = BN_new();

    BN_hex2bn(&server_share, buf);

    BIGNUM *KEY = BN_new();

    secret_maker(server_share, client_sec, KEY, ctx);

    // printf("\n\nkey:\n");
    // BN_print_fp(stdout, KEY);
    // printf("\n");

    unsigned char aes_key[AES_KEY_SIZE];

    derive_aes_key(KEY, aes_key);

    printf("\nAES KEY\n");
    print_hex("", aes_key, AES_KEY_SIZE);

    printf("\n");
    print_key_fingerprint(aes_key);

    printf("\n");

    while (1)
    {
        // n = receive_command(s, aes_key, buf, sizeof(buf));

        // if (n <= 0)
        // {
        //     printf("Connection closed during registration\n");
        //     close(s);
        //     return 1;
        // }

        // printf("Server: %s", buf);

        // if (fgets(buf, sizeof(buf), stdin) == NULL)
        //     return 1;

        // buf[strcspn(buf, "\n")] = '\0';

        strcpy(buf, username);

        send_command(s, buf, aes_key);

        n = receive_command(s, aes_key, buf, sizeof(buf));

        if (n <= 0)
        {
            printf("Connection closed during registration\n");
            close(s);
            return 1;
        }

        printf("Server: %s\n", buf);

        if (strstr(buf, "User Registration Successful.") != NULL)
            break;
    }

    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return 1;

    buf[strcspn(buf, "\n")] = '\0';

    send_command(s, buf, aes_key);

    n = receive_command(s, aes_key, buf, sizeof(buf));

    if (n <= 0)
        return 1;

    printf("Server: %s\n", buf);

    if (fork() == 0)
    {
        while (1)
        {
            n = receive_command(s, aes_key, buf, sizeof(buf));

            if (n <= 0)
                break;

            printf("Server:\n\n%s\n", buf);
        }

        close(s);
        return 0;
    }
    else
    {
        while (1)
        {
            if (fgets(buf, sizeof(buf), stdin) == NULL)
                break;

            buf[strcspn(buf, "\n")] = '\0';

            send_command(s, buf, aes_key);

            if (!strcmp(buf, "/quit"))
            {
                close(s);
                return 0;
            }
        }
    }

    close(s);

    X509_free(ca_cert);
    X509_free(server_cert);

    BN_free(client_sec);
    BN_free(secret);
    BN_free(share);
    BN_free(server_share);
    BN_free(KEY);
    BN_CTX_free(ctx);

    free_dh_params();

    return 0;
}