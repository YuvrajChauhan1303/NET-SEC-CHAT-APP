#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>

#include "services.h"

struct User users[MAX_USERS];
int user_count = 0;

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

    listen(s, 10);

    printf("[SERVER] Server Initialized. Listening for requests.\n\n");

    while (1)
    {
        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET(s, &readfds);

        int max_fd = s;

       
        for (int i = 0; i < user_count; i++)
        {
            FD_SET(users[i].socket, &readfds);

            if (users[i].socket > max_fd)
                max_fd = users[i].socket;
        }

       
        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        
        if (FD_ISSET(s, &readfds))
        {
            c = accept(s, NULL, NULL);

            printf("[SERVER] New connection. Socket: %d\n", c);

            if (register_client(c))
            {
                printf("[SERVER] Registration complete.\n");
            }
        }

        
        for (int i = 0; i < user_count; i++)
        {
            int client_socket = users[i].socket;

            if (FD_ISSET(client_socket, &readfds))
            {
                int n = read(client_socket, buf, sizeof(buf) - 1);


                if (n <= 0)
                {
                    printf("[SERVER] %s disconnected.\n",
                           users[i].username);

                    service_quit(client_socket,
                                 users[i].username);

                    i--;

                    continue;
                }

                buf[n] = '\0';

                printf("[CLIENT %s] %s\n",
                       users[i].username,
                       buf);

               
                if (!strcmp(buf, "/who"))
                {
                    printf("[SERVER] %s requested /who\n",
                        users[i].username);

                    service_who(client_socket);
                }

                else if (!strncmp(buf, "/chat ", 6))
                {
                    service_chat(i, buf);
                }

                else if (!strcmp(buf, "/quit"))
                {
                    service_quit(client_socket,
                                users[i].username);

                    i--;

                    continue;
                }

                else
                {
                    service_message(i, buf);
                }
            }
        }
    }

    close(s);

    return 0;
}