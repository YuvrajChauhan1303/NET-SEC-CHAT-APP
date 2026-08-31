#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include "services.h"

#define PORT 8081
#define BACKLOG 10

int main()
{
    int server_socket;
    int client_socket;
    int opt = 1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, BACKLOG);

    printf("Cert-auth server listening on port %d\n", PORT);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        char username[MAX_USERNAME];
        unsigned int key_len;
        unsigned char *key_data;
        const unsigned char *key_ptr;
        EVP_PKEY *public_key;

        recv(client_socket, username, MAX_USERNAME, 0);
        recv(client_socket, &key_len, sizeof(key_len), 0);

        key_len = ntohl(key_len);

        key_data = malloc(key_len);

        recv(client_socket, key_data, key_len, 0);

        key_ptr = key_data;
        public_key = d2i_PUBKEY(NULL, &key_ptr, key_len);

        if (register_client(username, public_key) == -1)
        {
            printf("Username already taken: %s\n", username);
            EVP_PKEY_free(public_key);
            free(key_data);
            close(client_socket);
            continue;
        }

        printf("Registered client: %s\n", username);

        print_users();

        close(client_socket);

        free(key_data);
    }

    close(server_socket);

    return 0;
}