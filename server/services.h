#define MAX_USERS 1000
#define MAX_USERNAME 20

struct User
{
    char username[MAX_USERNAME];
    int socket;
    char chat_with[MAX_USERNAME];
};

extern struct User users[MAX_USERS];
extern int user_count;

int register_client(int c);

char *read_command(int c, char *buf);

void send_command(int c, char *buf);

void service_who(int c);

void get_chat_username(char *buf, char *username);

int find_user(char *username);

void set_chat(int user_index, int target_index);

void service_chat(int user_index, char *buf);

void service_message(int user_index, char *buf);

void service_quit(int c, char *username);

void get_username(char *buf, char *username);

void service_chat_username(int user_index, char *username);