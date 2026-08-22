#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

void send_command(int s, char *buf)
{
    write(s, buf, strlen(buf));
}

int main(int argc, char *argv[])
{
    int s;
    struct addrinfo hints, *res;

    char *host = "127.0.0.1";
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

    int n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    printf("Server: %s\n", buf);

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    send_command(s, buf);

    n = read(s, buf, sizeof(buf) - 1);

    if (n <= 0)
        return 1;

    buf[n] = '\0';

    printf("Server: %s", buf);

    if (fork() == 0)
    {
        while (1)
        {
            n = read(s, buf, sizeof(buf) - 1);

            if (n <= 0)
                break;

            buf[n] = '\0';

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