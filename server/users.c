#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/bn.h>
#include <time.h>
#include <openssl/sha.h>


#include "users.h"
#include "dh.h"
#include "services.h"

struct User users[MAX_USERS];
int user_count = 0;

int register_client(int c)
{
    srand(time(NULL) ^ getpid());

    char username[MAX_USERNAME];
    char buf[1000];
    char response[1000];

    
    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *server_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();

    char x[513];

    char hexa[] = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };

    for (int i = 0; i < 512; i++)
    {
        x[i] = hexa[rand() % 16];
    }

    x[512] = '\0';

    printf("\n\nserver key:\n%s\n\n", x);

    BN_hex2bn(&server_sec, x);

    printf("\n\nserver key (after conv):\n");
    BN_print_fp(stdout, server_sec);

    sq_mult(server_sec, share, ctx);

    printf("\n\nserver share:\n");
    BN_print_fp(stdout, share);
    printf("\n");

    // response = BN_bn2hex(share);

    strcpy(response, BN_bn2hex(share));

    write(c, response, strlen(response));

    int n = read(c, buf, 513);

    BIGNUM *client_share = BN_new();
    BN_hex2bn(&client_share, buf);

    BIGNUM *KEY = BN_new();
    secret_maker(client_share, server_sec, KEY, ctx); 
    
    // printf("\n\nkey:\n");
    // BN_print_fp(stdout, KEY);
    printf("\n");

    char hexkey[513];
    strcpy(hexkey, BN_bn2hex(KEY));

    unsigned char hashed_key[AES_KEY_SIZE];

    // SHA256((unsigned char*)hexkey, strlen(hexkey), hashed_key);
    if(derive_aes_key(KEY, hashed_key) != 1){
        printf("Failed to derive ase key");
        return 0;
    }
    printf("\n\n");

    printf("[SERVER] AES KEY: ");

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        printf("%02x", hashed_key[i]);
    }

    printf("\n\n");
    printf("\n---------------------------------------------------------------------------------\n\n");



    while (1)
    {
        printf("Enter name: ");
        sprintf(response, "\t");

        // write(c, response, strlen(response));
        send_command(c, response, hashed_key); 

        // int n = read(c, username, MAX_USERNAME - 1);

        // if (n <= 0)
        //     return 0;

        // username[n] = '\0';
        unsigned char encrypted[2048];
        unsigned char decrypted[2048];

        int n = read(c, encrypted, sizeof(encrypted));

        if (n <= 0)
            return 0;

        int plaintext_len = decrypt_message(
            encrypted,
            n,
            hashed_key,
            decrypted
        );

        if (plaintext_len < 0)
        {
            printf("[SERVER] Registration username decryption failed\n");
            return 0;
        }

        decrypted[plaintext_len] = '\0';

        strcpy(username, (char *)decrypted);

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
            sprintf(response,
                    "Username already taken. Please choose another name.\n");

            send_command(c, response, hashed_key);

            continue;
        }

        strcpy(users[user_count].username, username);

        users[user_count].socket = c;

        users[user_count].chat_with[0] = '\0';

        // strcpy(users[user_count].KEY, hashed_key);
        memcpy(users[user_count].KEY, hashed_key, AES_KEY_SIZE);
        sprintf(response,
                "User Registration Successful.\n"
                "User registered with name: %s"
                "\nTotal Registered Users: %d\n",
                users[user_count].username,
                user_count + 1);

        send_command(c, response, hashed_key);

        printf("[SERVER] Registered %s on socket %d\n",
               users[user_count].username,
               c);

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
    char response[1000];

    response[0] = '\0';

    for (int i = 0; i < user_count; i++)
    {
        sprintf(response + strlen(response),
                "%d.\t%s\n",
                i + 1,
                users[i].username);
    }

    write(c, response, strlen(response));
}

void service_quit(int c, char *username)
{
    int i;

    printf("[SERVER] Removing user: %s\n",
           username);

    for (i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username, username))
        {
            close(users[i].socket);

            for (; i < user_count - 1; i++)
            {
                users[i] = users[i + 1];
            }

            user_count--;

            printf("[SERVER] User removed. Total users: %d\n",
                   user_count);

            return;
        }
    }

    close(c);
}