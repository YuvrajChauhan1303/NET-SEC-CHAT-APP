#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/bn.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "users.h"
#include "dh.h"
#include "services.h"
#include "aes.h"

#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

struct User users[MAX_USERS];
int user_count = 0;

int register_client(int c)
{
    char username[MAX_USERNAME];
    char buf[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *server_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();
    BIGNUM *client_share = NULL;
    BIGNUM *KEY = NULL;

    char x[513];
    char hexa[] = "0123456789ABCDEF";

    if (ctx == NULL || server_sec == NULL || secret == NULL || share == NULL)
    {
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    BN_hex2bn(&server_sec, x);

    printf("\n[SERVER] Server secret generated.\n");

    sq_mult(server_sec, share, ctx);

    char *share_hex = BN_bn2hex(share);

    if (share_hex == NULL)
    {
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    size_t share_len = strlen(share_hex);

    if (write(c, share_hex, share_len) != (ssize_t)share_len)
    {
        OPENSSL_free(share_hex);
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    OPENSSL_free(share_hex);

    int n = read(c, buf, sizeof(buf) - 1);

    if (n <= 0)
    {
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    buf[n] = '\0';

    client_share = BN_new();

    if (client_share == NULL || BN_hex2bn(&client_share, buf) == 0)
    {
        BN_free(client_share);
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    KEY = BN_new();

    if (KEY == NULL)
    {
        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(client_share);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    secret_maker(client_share, server_sec, KEY, ctx);

    unsigned char hashed_key[AES_KEY_SIZE];

    if (derive_aes_key(KEY, hashed_key) != 1)
    {
        printf("[SERVER] AES key derivation failed.\n");

        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(client_share);
        BN_free(KEY);
        BN_CTX_free(ctx);
        close(c);
        return 0;
    }

    printf("[SERVER] AES KEY: ");

    for (int i = 0; i < AES_KEY_SIZE; i++)
        printf("%02x", hashed_key[i]);

    printf("\n");

    while (1)
    {
        strcpy(response, "Enter Name:\t");

        send_command(c, response, hashed_key);

        int plaintext_len = receive_command(c, hashed_key, username, sizeof(username));

        if (plaintext_len <= 0)
        {
            printf("[SERVER] Client disconnected during registration.\n");

            BN_free(server_sec);
            BN_free(secret);
            BN_free(share);
            BN_free(client_share);
            BN_free(KEY);
            BN_CTX_free(ctx);
            close(c);
            return 0;
        }

        if (plaintext_len >= MAX_USERNAME)
        {
            strcpy(response, "Username too long. Please choose another name.\n");
            send_command(c, response, hashed_key);
            continue;
        }

        username[plaintext_len] = '\0';

        int name_taken = 0;

        for (int i = 0; i < user_count; i++)
        {
            if (!strcmp(users[i].username, username))
            {
                name_taken = 1;
                break;
            }
        }

        if (name_taken)
        {
            strcpy(response, "Username already taken. Please choose another name.\n");
            send_command(c, response, hashed_key);
            continue;
        }

        if (user_count >= MAX_USERS)
        {
            strcpy(response, "Server is full.\n");
            send_command(c, response, hashed_key);

            BN_free(server_sec);
            BN_free(secret);
            BN_free(share);
            BN_free(client_share);
            BN_free(KEY);
            BN_CTX_free(ctx);
            close(c);
            return 0;
        }

        strcpy(users[user_count].username, username);
        users[user_count].socket = c;
        users[user_count].chat_with[0] = '\0';

        memcpy(users[user_count].KEY, hashed_key, AES_KEY_SIZE);

        snprintf(response, sizeof(response), "User Registration Successful.\nUser registered with name: %s\nTotal Registered Users: %d\n", users[user_count].username, user_count + 1);

        send_command(c, response, hashed_key);

        printf("[SERVER] Registered %s on socket %d\n", users[user_count].username, c);

        user_count++;

        BN_free(server_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(client_share);
        BN_free(KEY);
        BN_CTX_free(ctx);

        return 1;
    }
}

int find_user(char *username)
{
    for (int i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username, username))
            return i;
    }

    return -1;
}

void service_who(int c)
{
    char response[BUFFER_SIZE];

    response[0] = '\0';

    for (int i = 0; i < user_count; i++)
    {
        snprintf(response + strlen(response), sizeof(response) - strlen(response), "%d.\t%s\n", i + 1, users[i].username);
    }

    for (int i = 0; i < user_count; i++)
    {
        if (users[i].socket == c)
        {
            send_command(c, response, users[i].KEY);
            return;
        }
    }
}

void service_quit(int user_index)
{
    if (user_index < 0 || user_index >= user_count)
        return;

    printf("[SERVER] Removing user: %s\n", users[user_index].username);

    notify_ca_user_quit(users[user_index].username);

    close(users[user_index].socket);

    for (int i = user_index; i < user_count - 1; i++)
        users[i] = users[i + 1];

    user_count--;

    printf("[SERVER] User removed. Total users: %d\n", user_count);
}

void notify_ca_user_quit(const char *username)
{
    int ca;
    struct sockaddr_in ca_addr;
    uint32_t request = 4;
    uint32_t username_len = strlen(username);

    ca = socket(AF_INET, SOCK_STREAM, 0);

    ca_addr.sin_family = AF_INET;
    ca_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ca_addr.sin_port = htons(8081);

    connect(ca, (struct sockaddr *)&ca_addr, sizeof(ca_addr));

    write(ca, &request, sizeof(request));
    write(ca, &username_len, sizeof(username_len));
    write(ca, username, username_len);

    close(ca);
}