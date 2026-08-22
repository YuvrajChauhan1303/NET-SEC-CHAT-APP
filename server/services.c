#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "services.h"

int register_client(int c)
{
    char username[MAX_USERNAME];
    char response[1000];

    while (1)
    {
        sprintf(response, "Enter Name:\t");

        write(c, response, strlen(response));

        int n = read(c, username, MAX_USERNAME - 1);

        if (n <= 0)
            return 0;

        username[n] = '\0';

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
            sprintf(response,
                    "Username already taken. Please choose another name.\n");

            write(c, response, strlen(response));

            continue;
        }

        strcpy(users[user_count].username, username);

        users[user_count].socket = c;

        users[user_count].chat_with[0] = '\0';

        sprintf(response,
                "User Registration Successful.\n"
                "User registered with name: %s"
                "\nTotal Registered Users: %d\n",
                users[user_count].username,
                user_count + 1);

        write(c, response, strlen(response));

        printf("[SERVER] Registered %s on socket %d\n",
               users[user_count].username,
               c);

        user_count++;

        return 1;
    }
}

char *read_command(int c, char *buf)
{
    int n = read(c, buf, 99);

    if (n <= 0)
        return NULL;

    buf[n] = '\0';

    return buf;
}

void send_command(int c, char *buf)
{
    write(c, buf, strlen(buf));
}

void service_who(int c)
{
    char response[1000];

    response[0] = '\0';

    for (int i = 0; i < user_count; i++)
    {
        sprintf(response + strlen(response),
                "%d.\t%s\n",
                i + 1,
                users[i].username);
    }

    write(c, response, strlen(response));
}

void service_chat(int user_index, char *buf)
{
    char target[MAX_USERNAME];

    sscanf(buf, "/chat %s", target);

    for (int i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username, target))
        {
            strcpy(users[user_index].chat_with,
                   target);

            char response[200];

            sprintf(response,
                    "Now chatting with %s\n",
                    target);

            write(users[user_index].socket,
                  response,
                  strlen(response));

            printf("[SERVER] %s is now chatting with %s\n",
                   users[user_index].username,
                   target);

            return;
        }
    }

    char response[200];

    sprintf(response,
            "User %s not found.\n",
            target);

    write(users[user_index].socket,
          response,
          strlen(response));
}

void service_message(int user_index, char *buf)
{
    if (users[user_index].chat_with[0] == '\0')
    {
        char response[] =
            "No chat selected. Use /chat <username>\n";

        write(users[user_index].socket,
              response,
              strlen(response));

        return;
    }

    for (int i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username,
                    users[user_index].chat_with))
        {
            char message[1200];

            sprintf(message,
                    "%s: %s",
                    users[user_index].username,
                    buf);

            write(users[i].socket,
                  message,
                  strlen(message));

            printf("[SERVER] %s -> %s: %s\n",
                   users[user_index].username,
                   users[i].username,
                   buf);

            return;
        }
    }
    char response[] =
        "The selected user is no longer connected.\n";

    write(users[user_index].socket,
          response,
          strlen(response));

    users[user_index].chat_with[0] = '\0';
}

void service_quit(int c, char *username)
{
    int i;

    printf("[SERVER] Removing user: %s\n",
           username);

    for (i = 0; i < user_count; i++)
    {
        if (!strcmp(users[i].username,
                    username))
        {
            close(users[i].socket);

            for (; i < user_count - 1; i++)
            {
                users[i] = users[i + 1];
            }

            user_count--;

            printf("[SERVER] User removed. Total users: %d\n",
                   user_count);

            return;
        }
    }

    close(c);
}