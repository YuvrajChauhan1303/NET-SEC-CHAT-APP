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
            // no other requests on THIS child
            close(s);

            char username[MAX_USERNAME];

            register_client(c, username);

            if (fork() == 0)
            {
                // read inputs from users.. the /chat, /quit, /who, etc... if no commands, send ,message to selected user...
                // means by default.. only server gets message.. does not relay them unless a user is selected.
                while (1)
                {

                    if (read_command(c, buf) == NULL)
                        break;

                    printf("Client: %s\n", buf);

                    if (!strcmp(buf, "/quit"))
                    {
                        service_quit(c, username);
                    }

                    if (!strcmp(buf, "/who"))
                    {
                        service_who(c);
                    }

                    char response[1000];

                    service_command(c, buf, response);
                }
            }
            else
            {
                // server can send msg here (temp)
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