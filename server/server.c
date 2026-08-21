#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/mman.h>

#include "services.h"

char (*users)[MAX_USERNAME];
int *user_count;

int main()
{
    int s, c;
    struct sockaddr_in addr = {0};
    char buf[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 1);

    users = mmap(NULL,
                 sizeof(char[MAX_USERS][MAX_USERNAME]),
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS,
                 -1,
                 0);

    user_count = mmap(NULL,
                      sizeof(int),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS,
                      -1,
                      0);

    *user_count = 0;

    printf("Server Initialized. Listening for requests.\n\n");

    while (1)
    {
        c = accept(s, NULL, NULL);

        if (fork() == 0)
        {
            close(s);

            register_client(c);

            if (fork() == 0)
            {

                while (1)
                {
                    if (read_command(c, buf) == NULL)
                        break;

                    printf("Client: %s\n", buf);
                }
            }
            else
            {

                while (1)
                {
                    fgets(buf, sizeof(buf), stdin);
                    buf[strlen(buf) - 1] = '\0';

                    send_command(c, buf);
                }
            }

            close(c);
            return 0;
        }

        close(c);
    }

    close(s);

    return 0;
}