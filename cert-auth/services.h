#define MAX_USERS 1000
#define MAX_USERNAME 20

#include <openssl/evp.h>

struct User
{
    char username[MAX_USERNAME];
    EVP_PKEY *public_key;
};

extern struct User users[MAX_USERS];
extern int user_count;

int register_client(char *username, EVP_PKEY *public_key);
int find_user(char *username);
void print_users(void);