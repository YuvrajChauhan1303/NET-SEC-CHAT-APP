#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/bn.h>
#include "dh.h"
#include "aes.h"
#include "services.h"

#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

void dh_client(int client_socket, unsigned char *client_aes_key)
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *client_sec = BN_new();
    BIGNUM *client_share = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *client_public = BN_new();

    char buf[513];
    char x[513];
    char hexa[] = "0123456789ABCDEF";

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    printf("Client side private-key%s:  \n", x);

    BN_hex2bn(&client_sec, x);

    sq_mult(client_sec, client_share, ctx);

    printf("MITM Fake server share: ");

    BN_print_fp(stdout, client_share);

    printf("\n");

    int n = read_all(client_socket, buf, 513);

    if (n <= 0)
        return;

    buf[512] = '\0';

    BN_hex2bn(&client_public, buf);

    char share_buf[513];
    char *share = BN_bn2hex(client_share);

    memset(share_buf, '0', 512);
    share_buf[512] = '\0';

    int share_len = strlen(share);

    memcpy(share_buf + 512 - share_len, share, share_len);

    write_all(client_socket, share_buf, 513);

    OPENSSL_free(share);

    secret_maker(client_public, client_sec, secret, ctx);

    printf("MITM Client-side shared secret:\n");

    BN_print_fp(stdout, secret);

    printf("\n");

    derive_aes_key(secret, client_aes_key);

    printf("MITM Client AES key: ");

    print_hex("", client_aes_key, AES_KEY_SIZE);

    BN_free(client_sec);
    BN_free(client_share);
    BN_free(secret);
    BN_free(client_public);
    BN_CTX_free(ctx);
}

void dh_server(int server_socket, unsigned char *server_aes_key)
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *server_sec = BN_new();
    BIGNUM *server_share = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *server_public = BN_new();

    char buf[513];
    char x[513];
    char hexa[] = "0123456789ABCDEF";

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    printf("Server side private-key%s:  \n", x);

    BN_hex2bn(&server_sec, x);

    sq_mult(server_sec, server_share, ctx);

    printf("MITM Fake client share: ");

    BN_print_fp(stdout, server_share);

    printf("\n");

    char share_buf[513];
    char *share = BN_bn2hex(server_share);

    memset(share_buf, '0', 512);
    share_buf[512] = '\0';

    int share_len = strlen(share);

    memcpy(share_buf + 512 - share_len, share, share_len);

    write_all(server_socket, share_buf, 513);

    OPENSSL_free(share);

    int n = read_all(server_socket, buf, 513);

    if (n <= 0)
        return;

    buf[512] = '\0';

    BN_hex2bn(&server_public, buf);

    printf("Server share: ");

    BN_print_fp(stdout, server_public);

    printf("\n");

    secret_maker(server_public, server_sec, secret, ctx);

    printf("[MITM] Server-side shared secret:\n");

    BN_print_fp(stdout, secret);

    printf("\n");

    derive_aes_key(secret, server_aes_key);

    printf("[MITM] Server AES key: ");

    print_hex("", server_aes_key, AES_KEY_SIZE);

    BN_free(server_sec);
    BN_free(server_share);
    BN_free(secret);
    BN_free(server_public);
    BN_CTX_free(ctx);
}

int main()
{
    int s, client_socket;
    int server_socket;

    struct sockaddr_in addr = {0};
    struct sockaddr_in server_addr = {0};

    unsigned char client_aes_key[AES_KEY_SIZE];
    unsigned char server_aes_key[AES_KEY_SIZE];

    srand(time(NULL));

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        printf("Socket failed");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    listen(s, 10);

    printf("MITM Listening for client on port 8000\n\n");

    client_socket = accept(s, NULL, NULL);

    printf("MIT Client connected. Socket: %d\n", client_socket);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("server", "8080", &hints, &res) != 0)
    {
        printf("Failed to resolve server\n");
        close(client_socket);
        close(server_socket);
        close(s);
        return 1;
    }

    memcpy(&server_addr, res->ai_addr, sizeof(server_addr));

    freeaddrinfo(res);

    connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("MITM : Connected to the real server, Socket : %d\n", server_socket);

    init_dh_params();

    printf("Starting DH with client\n");

    dh_client(client_socket, client_aes_key);

    printf("Starting DH with server\n");

    dh_server(server_socket, server_aes_key);

    printf("MITM: DH Completed\n\n");

    while (1)
    {
        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(client_socket, &readfds);
        FD_SET(server_socket, &readfds);

        int max_fd = client_socket;

        if (server_socket > max_fd)
            max_fd = server_socket;

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(client_socket, &readfds))
        {
            uint32_t network_len;
            unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];
            unsigned char plaintext[BUFFER_SIZE];
            unsigned char encrypted_again[ENCRYPTED_BUFFER_SIZE];

            int n = read_all(client_socket, &network_len, sizeof(network_len));

            if (n <= 0)
                break;

            int encrypted_len = ntohl(network_len);

            if (encrypted_len <= 0 || encrypted_len > ENCRYPTED_BUFFER_SIZE)
                break;

            n = read_all(client_socket, encrypted, encrypted_len);

            if (n <= 0)
                break;

            printf("\n");

            int plaintext_len = decrypt_message(encrypted, encrypted_len, client_aes_key, plaintext);

            if (plaintext_len < 0)
            {
                printf("Decryption failed\n\n");
                continue;
            }

            plaintext[plaintext_len] = '\0';

            printf("Plaintext from client: %s\n", plaintext);

            encrypted_len = encrypt_message(plaintext, plaintext_len, server_aes_key, encrypted_again);

            network_len = htonl(encrypted_len);

            write_all(server_socket, &network_len, sizeof(network_len));
            write_all(server_socket, encrypted_again, encrypted_len);

            fflush(stdout);
        }

        if (FD_ISSET(server_socket, &readfds))
        {
            uint32_t network_len;
            unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];
            unsigned char plaintext[BUFFER_SIZE];
            unsigned char encrypted_again[ENCRYPTED_BUFFER_SIZE];

            int n = read_all(server_socket, &network_len, sizeof(network_len));

            if (n <= 0)
                break;

            int encrypted_len = ntohl(network_len);

            if (encrypted_len <= 0 || encrypted_len > ENCRYPTED_BUFFER_SIZE)
                break;

            n = read_all(server_socket, encrypted, encrypted_len);

            if (n <= 0)
                break;

            printf("\n");

            int plaintext_len = decrypt_message(encrypted, encrypted_len, server_aes_key, plaintext);

            if (plaintext_len < 0)
            {
                printf("Decryption failed\n");
                continue;
            }

            plaintext[plaintext_len] = '\0';

            printf("Plaintext from Server: %s\n", plaintext);

            encrypted_len = encrypt_message(plaintext, plaintext_len, client_aes_key, encrypted_again);

            network_len = htonl(encrypted_len);

            write_all(client_socket, &network_len, sizeof(network_len));
            write_all(client_socket, encrypted_again, encrypted_len);

            fflush(stdout);
        }
    }

    close(client_socket);
    close(server_socket);
    close(s);

    free_dh_params();

    return 0;
}