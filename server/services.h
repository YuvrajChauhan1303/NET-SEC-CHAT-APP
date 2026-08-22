#define MAX_USERS 1000
#define MAX_USERNAME 20

extern char (*users)[MAX_USERNAME];
extern int *user_count;

void register_client(int c, char *username);
void itoa(int num, char *buf);
char *read_command(int c, char *buf);
void send_command(int c, char *buf);

int service_command(int c, char *buf, char *response);

void service_quit(int c, char *username);