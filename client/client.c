#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>

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

void send_command(int s, char *buf)
{
    uint32_t len = strlen(buf);
    uint32_t network_len = htonl(len);

    write_all(s, &network_len, sizeof(network_len));
    write_all(s, buf, len);
}

int receive_command(int s, char *buf, int size)
{
    uint32_t network_len;
    uint32_t len;

    int n = read_all(s, &network_len, sizeof(network_len));

    if (n <= 0)
        return n;

    len = ntohl(network_len);

    if (len >= size)
        return -1;

    n = read_all(s, buf, len);

    if (n <= 0)
        return n;

    buf[len] = '\0';

    return len;
}

int main(int argc, char *argv[])
{
    int s;
    struct addrinfo hints, *res;
    char *host = "server";
    char *port = "8080";

    if (argc >= 2)
        host = argv[1];

    if (argc >= 3)
        port = argv[2];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host, port, &hints, &res);

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    connect(s, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    char buf[1000];

    int n = receive_command(s, buf, sizeof(buf));

    if (n <= 0)
        return 1;

    printf("Server: %s\n", buf);

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    send_command(s, buf);

    n = receive_command(s, buf, sizeof(buf));

    if (n <= 0)
        return 1;

    printf("Server: %s", buf);

    if (fork() == 0)
    {
        while (1)
        {
            n = receive_command(s, buf, sizeof(buf));

            if (n <= 0)
                break;

            printf("Server:\n\n%s\n", buf);
        }
    }
    else
    {
        while (1)
        {
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';

            send_command(s, buf);

            if (!strcmp(buf, "/quit"))
            {
                close(s);

                return 0;
            }
        }
    }

    close(s);

    return 0;
}
