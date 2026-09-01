#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/bn.h>

#include "services.h"
#include "dh.h"
#include "aes.h"

#define BUFFER_SIZE 1000
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

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

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0)
    {
        printf("Failed to resolve server address\n");
        return 1;
    }

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (s < 0)
    {
        printf("Failed to create socket\n");
        freeaddrinfo(res);
        return 1;
    }

    init_dh_params();

    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *client_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();

    if (ctx == NULL || client_sec == NULL || secret == NULL || share == NULL)
    {
        printf("Failed to allocate DH parameters\n");
        close(s);
        freeaddrinfo(res);
        return 1;
    }

    char x[513];

    char hexa[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    printf("\n\nclient key:\n%s\n\n", x);

    BN_hex2bn(&client_sec, x);

    printf("\n\nclient key (after conv):\n");
    BN_print_fp(stdout, client_sec);

    sq_mult(client_sec, share, ctx);

    printf("\n\nclient share:\n");
    BN_print_fp(stdout, share);
    printf("\n");

    if (connect(s, res->ai_addr, res->ai_addrlen) < 0)
    {
        printf("Failed to connect to server\n");
        close(s);
        freeaddrinfo(res);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    freeaddrinfo(res);

    char buf[BUFFER_SIZE];

    char *share_hex = BN_bn2hex(share);

    if (share_hex == NULL)
    {
        printf("Failed to convert client share\n");
        close(s);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    strcpy(buf, share_hex);
    OPENSSL_free(share_hex);

    if (write(s, buf, 513) != 513)
    {
        printf("Failed to send client share\n");
        close(s);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    int n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    printf("Server: %s\n", buf);

    BIGNUM *server_share = BN_new();

    if (server_share == NULL)
    {
        printf("Failed to allocate server share\n");
        close(s);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    BN_hex2bn(&server_share, buf);

    BIGNUM *KEY = BN_new();

    if (KEY == NULL)
    {
        printf("Failed to allocate shared key\n");
        close(s);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(server_share);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    secret_maker(server_share, client_sec, KEY, ctx);

    printf("\n\nkey:\n");
    BN_print_fp(stdout, KEY);
    printf("\n");

    unsigned char aes_key[AES_KEY_SIZE];

    if (derive_aes_key(KEY, aes_key) != 1)
    {
        printf("Failed to derive aes key\n");
        close(s);
        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(server_share);
        BN_free(KEY);
        BN_CTX_free(ctx);
        free_dh_params();
        return 1;
    }

    printf("\nAES KEY\n");
    print_hex("", aes_key, AES_KEY_SIZE);

    printf("\n");
    print_key_fingerprint(aes_key);
    printf("\n");

    while (1)
    {
        n = receive_command(s, aes_key, buf, sizeof(buf));

        if (n <= 0)
        {
            printf("Connection closed during registration\n");
            close(s);
            BN_free(client_sec);
            BN_free(secret);
            BN_free(share);
            BN_free(server_share);
            BN_free(KEY);
            BN_CTX_free(ctx);
            free_dh_params();
            return 1;
        }

        printf("Server: %s", buf);

        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            close(s);
            BN_free(client_sec);
            BN_free(secret);
            BN_free(share);
            BN_free(server_share);
            BN_free(KEY);
            BN_CTX_free(ctx);
            free_dh_params();
            return 1;
        }

        buf[strcspn(buf, "\n")] = '\0';

        send_command(s, buf, aes_key);

        n = receive_command(s, aes_key, buf, sizeof(buf));

        if (n <= 0)
        {
            printf("Connection closed during registration\n");
            close(s);
            BN_free(client_sec);
            BN_free(secret);
            BN_free(share);
            BN_free(server_share);
            BN_free(KEY);
            BN_CTX_free(ctx);
            free_dh_params();
            return 1;
        }

        printf("Server: %s\n", buf);

        if (strstr(buf, "User Registration Successful.") != NULL)
            break;
    }

    if (fgets(buf, sizeof(buf), stdin) == NULL)
    {
        close(s);
        return 1;
    }

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

    BN_free(client_sec);
    BN_free(secret);
    BN_free(share);
    BN_free(server_share);
    BN_free(KEY);
    BN_CTX_free(ctx);

    free_dh_params();

    return 0;
}