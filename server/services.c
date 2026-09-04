#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "services.h"

int write_all(int c, void *buf, int len)
{
    int sent = 0;

    while (sent < len)
    {
        int n = write(c, (char *)buf + sent, len - sent);

        if (n <= 0)
            return n;

        sent += n;
    }

    return sent;
}

int read_all(int c, void *buf, int len)
{
    int received = 0;

    while (received < len)
    {
        int n = read(c, (char *)buf + received, len - received);

        if (n <= 0)
            return n;

        received += n;
    }

    return received;
}

char *read_command(int c, char *buf)
{
    uint32_t network_len;
    uint32_t len;

    int n = read_all(c, &network_len, sizeof(network_len));

    if (n <= 0)
        return NULL;

    len = ntohl(network_len);

    if (len >= 100)
        return NULL;

    n = read_all(c, buf, len);

    if (n <= 0)
        return NULL;

    buf[len] = '\0';

    return buf;
}

void send_command(int c, char *buf)
{
    uint32_t len = strlen(buf);
    uint32_t network_len = htonl(len);

    write_all(c, &network_len, sizeof(network_len));
    write_all(c, buf, len);
}
