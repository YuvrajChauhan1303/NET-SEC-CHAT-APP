#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    int s, c;
    struct sockaddr_in addr = {0};
    char buf[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    listen(s, 1);

    c = accept(s, NULL, NULL);

    while (1)
    {
        int n = read(c, buf, sizeof(buf));

        if (n > 0)
            printf("Client: %.*s\n", n, buf);

        scanf("%s", buf);
        send(c, buf, strlen(buf), 0);
    }

    close(c);
    close(s);
}