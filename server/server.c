#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include "users.h"
#include "chat.h"
#include "services.h"
#include "dh.h"
#include "aes.h"

int main()
{
    int s, c;
    struct sockaddr_in addr = {0};

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

            init_dh_params();

            if (register_client(c))
                printf("[SERVER] Registration complete.\n");
        }

        for (int i = 0; i < user_count; i++)
        {
            int client_socket = users[i].socket;

            if (!FD_ISSET(client_socket, &readfds))
                continue;

            unsigned char plaintext[4096];

            int plaintext_len = receive_command(client_socket, users[i].KEY, (char *)plaintext, sizeof(plaintext));

            if (plaintext_len <= 0)
            {
                service_quit(i);
                i--;
                continue;
            }

            printf("[CLIENT %s] %s\n", users[i].username, plaintext);

            if (!strcmp((char *)plaintext, "/who"))
            {
                printf("[SERVER] %s requested /who\n", users[i].username);
                service_who(client_socket);
            }
            else if (!strncmp((char *)plaintext, "/chat ", 6))
            {
                service_chat(i, (char *)plaintext);
            }
            else if (!strcmp((char *)plaintext, "/quit"))
            {
                service_quit(i);
                i--;
                continue;
            }
            else if (plaintext[0] == '@')
            {
                char username[MAX_USERNAME];

                get_username((char *)plaintext, username);

                service_chat_username(i, username);

                char *msg = (char *)plaintext + strlen(username) + 2;

                service_message(i, msg);
            }
            else
            {
                service_message(i, (char *)plaintext);
            }
        }
    }

    close(s);
    free_dh_params();

    return 0;
}