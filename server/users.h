#define MAX_USERS 1000

#define AES_KEY_SIZE 32

#define MAX_USERNAME 20

#include "aes.h"
#include <openssl/evp.h>

struct User
{
    char username[MAX_USERNAME];
    int socket;
    char chat_with[MAX_USERNAME];
    unsigned char KEY[AES_KEY_SIZE];
};

extern struct User users[MAX_USERS];

extern int user_count;

int register_client(int c, EVP_PKEY *server_key, unsigned char *challenge);

int find_user(char *username);

void service_who(int c);

void service_quit(int user_index);
int read_full(int fd, void *buf, size_t len);
int write_full(int fd, const void *buf, size_t len);