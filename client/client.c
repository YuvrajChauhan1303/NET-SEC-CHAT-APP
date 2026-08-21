#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void send_command(int s, char *buf)
{
    write(s, buf, strlen(buf));
}

int main()
{
    int s;
    struct sockaddr_in addr = {0};
    char buf[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (struct sockaddr *)&addr, sizeof(addr));

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

            printf("Server: %s\n", buf);
        }
    }
    else
    {

        while (1)
        {
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';

            send_command(s, buf);
        }
    }

    close(s);

    return 0;
}