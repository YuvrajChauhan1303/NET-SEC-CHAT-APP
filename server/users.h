#define MAX_USERS 1000

#define AES_KEY_SIZE 32

#define MAX_USERNAME 20

#include "aes.h"

struct User
{
    char username[MAX_USERNAME];
    int socket;
    char chat_with[MAX_USERNAME];
    unsigned char KEY[AES_KEY_SIZE];
};

extern struct User users[MAX_USERS];

extern int user_count;

int register_client(int c);

int find_user(char *username);

void service_who(int c);

void service_quit(int user_index);