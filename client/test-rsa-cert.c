#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include "rsa.h"

#define CERT_AUTH_PORT 8081
#define CERT_AUTH_IP "127.0.0.1"
#define MAX_USERNAME 20

int main()
{
    const char *username = "alice";
    const char *private_key_path = "alice-private.pem";
    const char *public_key_path = "alice-public.pem";

    int sock;
    struct sockaddr_in server_addr;
    EVP_PKEY *public_key = NULL;
    unsigned char *der_key = NULL;
    unsigned char *der_ptr;
    int der_len;

    rsa_generate_keypair(private_key_path, public_key_path);

    printf("RSA key pair generated\n");

    rsa_load_public_key(public_key_path, &public_key);

    printf("Public key loaded\n");

    der_len = i2d_PUBKEY(public_key, NULL);
    der_key = malloc(der_len);

    der_ptr = der_key;
    i2d_PUBKEY(public_key, &der_ptr);

    printf("Public key converted to DER\n");
    printf("DER key length: %d bytes\n", der_len);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CERT_AUTH_PORT);
    inet_pton(AF_INET, CERT_AUTH_IP, &server_addr.sin_addr);

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Connected to cert-auth\n");

    char username_buffer[MAX_USERNAME];
    memset(username_buffer, 0, sizeof(username_buffer));
    strcpy(username_buffer, username);

    send(sock, username_buffer, MAX_USERNAME, 0);

    unsigned int network_key_len = htonl(der_len);

    send(sock, &network_key_len, sizeof(network_key_len), 0);

    send(sock, der_key, der_len, 0);

    printf("Registered %s with cert-auth\n", username);

    close(sock);
    free(der_key);
    EVP_PKEY_free(public_key);

    return 0;
}