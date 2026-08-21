#define MAX_USERS 1000
#define MAX_USERNAME 20

extern char (*users)[MAX_USERNAME];
extern int *user_count;

void register_client(int c);
void itoa(int num, char *buf);
char *read_command(int c, char *buf);
void send_command(int c, char *buf);