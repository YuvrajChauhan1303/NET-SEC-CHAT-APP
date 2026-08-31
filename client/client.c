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
#include <openssl/sha.h>

#include "dh.h"

void send_command(int s, char *buf)
{
    write(s, buf, strlen(buf));
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
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
    {
        x[i] = hexa[rand() % 16];
    }

    x[512] = '\0';

    // printf("\n\nclient key:\n%s\n\n", x);

    BN_hex2bn(&client_sec, x);

    // printf("\n\nclient key (after conv):\n");
    // BN_print_fp(stdout, client_sec);

    sq_mult(client_sec, share, ctx); // g^a mod p

    // printf("\n\nclient share:\n");
    // BN_print_fp(stdout, share);
    printf("\n");

    connect(s, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    char buf[1000];

    strcpy(buf, BN_bn2hex(share));

    write(s, buf, 513); // client shares its own share to server

    int n = read(s, buf, sizeof(buf) - 1); // client recvs the server's share

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    // printf("Server: %s\n", buf);

    BIGNUM *server_share = BN_new();
    BN_hex2bn(&server_share, buf);

    BIGNUM *KEY = BN_new();
    secret_maker(server_share, client_sec, KEY, ctx); // g^b, a => g ^ (a.b) => (g^b)^a

    // printf("\n\nkey:\n");
    // BN_print_fp(stdout, KEY);
    printf("\n");

    char hexkey[513];
    strcpy(hexkey, BN_bn2hex(KEY));

    unsigned char hashed_key[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)hexkey, strlen(hexkey), hashed_key);

    printf("\n\n");

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        printf("%02x", hashed_key[i]);
    }

    printf("\n\n");

    n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    printf("Server: %s", buf);

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    send_command(s, buf);

    n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    printf("Server: %s", buf);

    if (fork() == 0)
    {
        while (1)
        {
            n = read(s, buf, sizeof(buf) - 1);

            if (n <= 0)
                break;

            buf[n] = '\0';

            printf("Server:\n\n%s\n", buf);
        }
    }
    else
    {
        while (1)
        {
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';

            send_command(s, buf);

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