#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "users.h"
#include "services.h"

struct User users[MAX_USERS];
int user_count = 0;

int register_client(int c)
{
    char username[MAX_USERNAME];
    char response[1000];

    while (1)
    {
        sprintf(response, "Enter Name:\t");
        send_command(c, response);

        if (read_command(c, username) == NULL)
            return 0;

        int name_taken = 0;

        for (int i = 0; i < user_count; i++)
        {
            if (!strcmp(users[i].username, username))
            {
                name_taken = 1;
                break;
            }
        }

        if (name_taken)
        {
            sprintf(response, "Username already taken. Please choose another name.\n");
            send_command(c, response);
            continue;
        }

        strcpy(users[user_count].username, username);
        users[user_count].socket = c;
        users[user_count].chat_with[0] = '\0';

        sprintf(response, "User Registration Successful.\nUser registered with name: %s\nTotal Registered Users: %d\n", users[user_count].username, user_count + 1);
        send_command(c, response);

        printf("[SERVER] Registered %s on socket %d\n", users[user_count].username, c);

        user_count++;

        return 1;
    }
}

int find_user(char *username)
{
    for (int i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username, username))
            return i;
    }

    return -1;
}

void service_who(int c)
{
    char response[1000];

    response[0] = '\0';

    for (int i = 0; i < user_count; i++)
    {
        sprintf(response + strlen(response), "%d.\t%s\n", i + 1, users[i].username);
    }

    send_command(c, response);
}

void service_quit(int c, char *username)
{
    int i;

    printf("[SERVER] Removing user: %s\n", username);

    for (i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username, username))
        {
            close(users[i].socket);

            for (; i < user_count - 1; i++)
            {
                users[i] = users[i + 1];
            }

            user_count--;

            printf("[SERVER] User removed. Total users: %d\n", user_count);

            return;
        }
    }

    close(c);
}
