#define MAX_USERS 1000
#define MAX_USERNAME 20

struct User
{
    char username[MAX_USERNAME];
    int socket;
    char chat_with[MAX_USERNAME];
    unsigned char KEY[65];
};

extern struct User users[MAX_USERS];
extern int user_count;

int register_client(int c);
int find_user(char *username);
void service_who(int c);
void service_quit(int c, char *username);