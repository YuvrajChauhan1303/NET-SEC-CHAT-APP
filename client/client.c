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
#include <openssl/err.h>

#include "services.h"
#include "dh.h"
#include "aes.h"
#include "cert.h"

int read_full(int fd, void *buf, size_t len)
{
    size_t total = 0;

    while (total < len)
    {
        int n = read(fd, (char *)buf + total, len - total);

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

int write_full(int fd, const void *buf, size_t len)
{
    size_t total = 0;

    while (total < len)
    {
        int n = write(fd, (const char *)buf + total, len - total);

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

int main(int argc, char *argv[])
{
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

    if (getaddrinfo(host, port, &hints, &res) != 0)
        return 1;

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (s < 0)
    {
        freeaddrinfo(res);
        return 1;
    }

    init_dh_params();

    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *client_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();

    char x[513];

    char hexa[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    BN_hex2bn(&client_sec, x);

    sq_mult(client_sec, share, ctx);

    if (connect(s, res->ai_addr, res->ai_addrlen) < 0)
    {
        freeaddrinfo(res);
        close(s);
        return 1;
    }

    freeaddrinfo(res);

    printf("[CLIENT] Connected to server.\n");

    uint32_t cert_len;

    if (read_full(s, &cert_len, sizeof(cert_len)) < 0)
    {
        printf("[CLIENT] Failed to receive certificate length.\n");
        close(s);
        return 1;
    }

    printf("[CLIENT] cert_len = %u\n", cert_len);

    if (cert_len == 0 || cert_len > 100000)
    {
        printf("[CLIENT] Invalid certificate length.\n");
        close(s);
        return 1;
    }

    unsigned char *cert_data = malloc(cert_len);

    if (cert_data == NULL)
    {
        close(s);
        return 1;
    }

    if (read_full(s, cert_data, cert_len) < 0)
    {
        printf("[CLIENT] Failed to receive certificate data.\n");
        free(cert_data);
        close(s);
        return 1;
    }

    printf("[CLIENT] received certificate data\n");

    const unsigned char *p = cert_data;

    X509 *server_cert = d2i_X509(NULL, &p, cert_len);

    if (server_cert == NULL)
    {
        printf("[CLIENT] Failed to parse server certificate.\n");
        ERR_print_errors_fp(stderr);

        free(cert_data);
        close(s);
        return 1;
    }

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

    if (write_full(s, challenge, sizeof(challenge)) < 0)
    {
        close(s);
        return 1;
    }

    uint32_t signature_len;

    if (read_full(s, &signature_len, sizeof(signature_len)) < 0)
    {
        printf("[CLIENT] Failed to receive signature length.\n");
        close(s);
        return 1;
    }

    if (signature_len == 0 || signature_len > 256)
    {
        printf("[CLIENT] Invalid signature length.\n");
        close(s);
        return 1;
    }

    unsigned char signature[256];

    if (read_full(s, signature, signature_len) < 0)
    {
        printf("[CLIENT] Failed to receive signature.\n");
        close(s);
        return 1;
    }

    uint32_t share_len;

    if (read_full(s, &share_len, sizeof(share_len)) < 0)
    {
        printf("[CLIENT] Failed to receive DH share length.\n");
        close(s);
        return 1;
    }

    if (share_len == 0 || share_len >= 1000)
    {
        printf("[CLIENT] Invalid DH share length.\n");
        close(s);
        return 1;
    }

    char buf[1000];

    if (read_full(s, buf, share_len) < 0)
    {
        printf("[CLIENT] Failed to receive DH share.\n");
        close(s);
        return 1;
    }

    buf[share_len] = '\0';

    printf("[CLIENT] Received server DH share.\n");

    if (!verify_challenge(server_cert,
                          challenge,
                          32,
                          (unsigned char *)buf,
                          share_len,
                          signature,
                          signature_len))
    {
        printf("[CLIENT] Server proof-of-possession failed.\n");
        printf("[CLIENT] Server DH share does not match the signed share.\n");
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

    printf("[CLIENT] Server proof-of-possession verified.\n");

    BIGNUM *server_share = BN_new();

    if (server_share == NULL ||
        BN_hex2bn(&server_share, buf) == 0)
    {
        printf("[CLIENT] Invalid server DH share.\n");

        BN_free(server_share);
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

    char *share_hex = BN_bn2hex(share);

    if (share_hex == NULL)
    {
        BN_free(server_share);
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

    if (write_full(s, share_hex, strlen(share_hex)) < 0)
    {
        OPENSSL_free(share_hex);
        BN_free(server_share);
        close(s);
        return 1;
    }

    OPENSSL_free(share_hex);

    BIGNUM *KEY = BN_new();

    if (KEY == NULL)
    {
        BN_free(server_share);
        close(s);
        return 1;
    }

    secret_maker(server_share, client_sec, KEY, ctx);

    unsigned char aes_key[AES_KEY_SIZE];

    if (derive_aes_key(KEY, aes_key) != 1)
    {
        printf("[CLIENT] AES key derivation failed.\n");

        BN_free(KEY);
        BN_free(server_share);
        close(s);
        return 1;
    }

    printf("\nAES KEY\n");
    print_hex("", aes_key, AES_KEY_SIZE);

    printf("\n");
    print_key_fingerprint(aes_key);

    printf("\n");

    while (1)
    {
        int n = receive_command(s, aes_key, buf, sizeof(buf));

        if (n <= 0)
        {
            printf("Connection closed during registration\n");
            close(s);
            return 1;
        }

        printf("Server: %s", buf);

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            return 1;

        buf[strcspn(buf, "\n")] = '\0';

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

    int n = receive_command(s, aes_key, buf, sizeof(buf));

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