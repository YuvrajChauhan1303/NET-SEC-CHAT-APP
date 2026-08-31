#include <stdio.h>
#include <string.h>
#include <openssl/pem.h>

#include "services.h"

struct User users[MAX_USERS];
int user_count = 0;

int register_client(char *username, EVP_PKEY *public_key)
{
    if (find_user(username) != -1)
        return -1;

    strcpy(users[user_count].username, username);
    users[user_count].public_key = public_key;
    user_count++;

    return 0;
}

int find_user(char *username)
{
    for (int i = 0; i < user_count; i++)
    {
        if (strcmp(users[i].username, username) == 0)
            return i;
    }

    return -1;
}

void print_users(void)
{
    for (int i = 0; i < user_count; i++)
    {
        printf("Username: %s\n", users[i].username);
        printf("Public Key:\n");
        PEM_write_PUBKEY(stdout, users[i].public_key);
        printf("\n");
    }
}