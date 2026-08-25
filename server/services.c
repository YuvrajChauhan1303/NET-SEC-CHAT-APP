#include <string.h>
#include <unistd.h>

#include "services.h"

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