#include <unistd.h>
#include "services.h"

void write_all(int s, void *buf, int len)
{
    int sent = 0;

    while (sent < len)
    {
        int n = write(s, (char *)buf + sent, len - sent);

        if (n <= 0)
            return;

        sent += n;
    }
}

int read_all(int s, void *buf, int len)
{
    int received = 0;

    while (received < len)
    {
        int n = read(s, (char *)buf + received, len - received);

        if (n <= 0)
            return n;

        received += n;
    }

    return received;
}