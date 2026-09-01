#include <stdio.h>
#include <string.h>

#include "users.h"
#include "chat.h"
#include "services.h"

void get_chat_username(char *buf, char *username)
{
    int i = 6;
    int j = 0;

    while (buf[i] != ' ' && buf[i] != '\0')
    {
        username[j] = buf[i];

        i++;
        j++;
    }

    username[j] = '\0';
}

void set_chat(int user_index, int target_index)
{
    strcpy(users[user_index].chat_with,
           users[target_index].username);
}

void service_chat(int user_index, char *buf)
{
    char target[MAX_USERNAME];

    get_chat_username(buf, target);

    int target_index = find_user(target);

    if (target_index == -1)
    {
        char response[200];

        sprintf(response,
                "User %s not found.\n",
                target);

        send_command(users[user_index].socket,
                     response,
                     users[user_index].KEY);

        return;
    }

    set_chat(user_index, target_index);

    char response[200];

    sprintf(response,
            "Now chatting with %s\n",
            target);

    send_command(users[user_index].socket,
                 response,
                 users[user_index].KEY);

    printf("[SERVER] %s is now chatting with %s\n",
           users[user_index].username,
           target);
}

void service_message(int user_index, char *buf)
{
    if (users[user_index].chat_with[0] == '\0')
    {
        char response[] =
            "No chat selected. Use /chat <username>\n";

        send_command(users[user_index].socket,
                     response,
                     users[user_index].KEY);

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

            send_command(users[i].socket,
                         message,
                         users[i].KEY);

            printf("[SERVER] %s -> %s: %s\n",
                   users[user_index].username,
                   users[i].username,
                   buf);

            return;
        }
    }

    char response[] =
        "The selected user is no longer connected.\n";

    send_command(users[user_index].socket,
                 response,
                 users[user_index].KEY);

    users[user_index].chat_with[0] = '\0';
}

void get_username(char *buf, char *username)
{
    int i = 1;
    int j = 0;

    while (buf[i] != ' ' && buf[i] != '\0')
    {
        username[j] = buf[i];

        i++;
        j++;
    }

    username[j] = '\0';
}

void service_chat_username(int user_index, char *username)
{
    int target_index = find_user(username);

    if (target_index == -1)
    {
        char response[200];

        sprintf(response,
                "User %s not found.\n",
                username);

        send_command(users[user_index].socket,
                     response,
                     users[user_index].KEY);

        printf("[SERVER] User %s not found\n",
               username);

        return;
    }

    set_chat(user_index, target_index);

    printf("[SERVER] %s is now chatting with %s\n",
           users[user_index].username,
           username);
}