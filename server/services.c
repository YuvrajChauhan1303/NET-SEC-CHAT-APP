#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "services.h"

void register_client(int c, char *username)
{

    char response[1000];

    sprintf(response, "Enter Name:\t");
    write(c, response, strlen(response));

    int n = read(c, username, MAX_USERNAME - 1);

    if (n <= 0)
        return;

    username[n] = '\0';

    strcpy(users[*user_count], username);

    printf("Registered %s with socket %d\n",
           users[*user_count],
           c);

    (*user_count)++;

    sprintf(response,
            "User Registration Successful.\n"
            "User registered with name: %s"
            "\nTotal Registered Users: %d\n",
            users[*user_count - 1],
            *user_count);

    write(c, response, strlen(response));
}

void itoa(int num, char *buf)
{
    sprintf(buf, "%d", num);
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

// if 1 -> forward message to selected user
// if 2 -> select a user, respond to main with selected user

int service_command(int c, char *buf, char *response)
{
    int chat_flag = 0;
    int select_flag = 0;

    char command[20];
    int i = 0;

    if (buf[0] == '/')
    {
        while (buf[i] != ' ' && buf[i] != '\0')
        {
            command[i] = buf[i];
            i++;
        }

        command[i] = '\0';

        if (!strcmp(command, "/chat"))
            chat_flag = 1;

        return 1;
    }

    else if (buf[0] == '@')
    {
        i = 1;

        char username[50];
        int j = 0;

        while (buf[i] != ' ' && buf[i] != '\0')
        {
            username[j] = buf[i];
            j++;
            i++;
        }

        username[j] = '\0';

        strcpy(response, username);

        select_flag = 1;

        return 2;
    }

    return 0;
}

void service_quit(int c, char *username)
{
    int i;

    printf("Removing user: %s\n", username);

    for (i = 0; i < *user_count; i++)
    {
        if (!strcmp(users[i], username))
        {
            for (; i < *user_count - 1; i++)
                strcpy(users[i], users[i + 1]);

            (*user_count)--;

            printf("User removed. Count: %d\n", *user_count);

            break;
        }
    }

    close(c);
}

void service_who(int c)
{
    char response[1000];

    response[0] = '\0';

    for (int i = 0; i < *user_count; i++)
    {
        sprintf(response + strlen(response),
                "%d.\t%s\n",
                i + 1,
                users[i]);
    }

    write(c, response, strlen(response));
}