#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

    while (1)
    {
        scanf("%s", buf);
        send(s, buf, strlen(buf), 0);

        int n = read(s, buf, sizeof(buf));
        if (n > 0)
            printf("Server: %.*s\n", n, buf);
    }

    close(s);
}