#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "services.h"

void register_client(int c)
{
    write(c, "Enter Name:", 11);

    char buf[1000];

    int n = read(c, buf, sizeof(buf) - 1);

    if (n <= 0)
        return;

    buf[n] = '\0';

    strcpy(users[*user_count], buf);

    printf("Registered %s with socket %d\n",
           users[*user_count],
           c);

    (*user_count)++;

    char response[1000];

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