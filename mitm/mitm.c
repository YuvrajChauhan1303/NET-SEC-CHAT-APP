#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "dh.h"
#include "../server/aes.h"

int read_full(int fd, void *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t n = read(fd, (unsigned char *)buf + total, len - total);

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

int write_full(int fd, const void *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t n = write(fd, (const unsigned char *)buf + total, len - total);

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

EVP_PKEY *generate_fake_key() {
    EVP_PKEY *key = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    if (ctx == NULL)
        return NULL;

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY_keygen(ctx, &key);

    EVP_PKEY_CTX_free(ctx);

    return key;
}

int fake_sign_challenge(EVP_PKEY *fake_key, unsigned char *challenge, int challenge_len, unsigned char *signature) {
    EVP_MD_CTX *ctx;
    size_t signature_len = 0;

    ctx = EVP_MD_CTX_new();

    if (ctx == NULL)
        return -1;

    if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, fake_key) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_DigestSignUpdate(ctx, challenge, challenge_len);
    EVP_DigestSignFinal(ctx, NULL, &signature_len);

    if (EVP_DigestSignFinal(ctx, signature, &signature_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);

    return (int)signature_len;
}

int main() {
    int s;
    int client_socket;
    int server_socket;

    struct sockaddr_in addr = {0};
    struct sockaddr_in server_addr = {0};

    srand(time(NULL));

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        printf("Socket failed\n");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("Bind failed\n");
        close(s);
        return 1;
    }

    if (listen(s, 10) < 0) {
        printf("Listen failed\n");
        close(s);
        return 1;
    }

    printf("MITM listening on port 8000\n");

    client_socket = accept(s, NULL, NULL);

    if (client_socket < 0) {
        printf("Client accept failed\n");
        close(s);
        return 1;
    }

    printf("Client connected\n");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        printf("Server socket failed\n");
        close(client_socket);
        close(s);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection to real server failed\n");
        close(client_socket);
        close(server_socket);
        close(s);
        return 1;
    }

    printf("Connected to real server\n");

    uint32_t cert_len;

    if (read_full(server_socket, &cert_len, sizeof(cert_len)) < 0)
        goto cleanup;

    unsigned char *cert_data = malloc(cert_len);

    if (cert_data == NULL)
        goto cleanup;

    if (read_full(server_socket, cert_data, cert_len) < 0) {
        free(cert_data);
        goto cleanup;
    }

    if (write_full(client_socket, &cert_len, sizeof(cert_len)) < 0 ||
        write_full(client_socket, cert_data, cert_len) < 0) {
        free(cert_data);
        goto cleanup;
    }

    free(cert_data);

    unsigned char challenge[32];

    if (read_full(client_socket, challenge, sizeof(challenge)) < 0)
        goto cleanup;

    printf("\n1. Sign challenge\n");
    printf("2. Relay challenge\n");
    printf("Choose attack: ");

    int choice;

    if (scanf("%d", &choice) != 1)
        goto cleanup;

    if (choice == 1) {
        EVP_PKEY *fake_key = generate_fake_key();

        if (fake_key == NULL)
            goto cleanup;

        unsigned char fake_signature[256];

        int fake_signature_len = fake_sign_challenge(
            fake_key,
            challenge,
            sizeof(challenge),
            fake_signature
        );

        if (fake_signature_len <= 0) {
            EVP_PKEY_free(fake_key);
            goto cleanup;
        }

        uint32_t signature_len = fake_signature_len;

        write_full(client_socket, &signature_len, sizeof(signature_len));
        write_full(client_socket, fake_signature, fake_signature_len);

        unsigned char buffer[1024];
        int n = read(client_socket, buffer, sizeof(buffer));

        if (n <= 0) {
            printf("\n[MALLORY] MITM attack failed\n");
            printf("[MALLORY] Client rejected the forged signature\n");
        } else {
            printf("\n[MALLORY] MITM attack failed\n");
        }

        EVP_PKEY_free(fake_key);
    } else if (choice == 2) {
        if (write_full(server_socket, challenge, sizeof(challenge)) < 0)
            goto cleanup;

        uint32_t signature_len;

        if (read_full(server_socket, &signature_len, sizeof(signature_len)) < 0)
            goto cleanup;

        if (signature_len == 0 || signature_len > 256)
            goto cleanup;

        unsigned char signature[256];

        if (read_full(server_socket, signature, signature_len) < 0)
            goto cleanup;

        uint32_t share_len;

        if (read_full(server_socket, &share_len, sizeof(share_len)) < 0)
            goto cleanup;

        if (share_len == 0 || share_len >= 514)
            goto cleanup;

        char real_server_share[514];

        if (read_full(server_socket, real_server_share, share_len) < 0)
            goto cleanup;

        real_server_share[share_len] = '\0';

        init_dh_params();

        BN_CTX *ctx = BN_CTX_new();
        BIGNUM *mallory_sec = BN_new();
        BIGNUM *mallory_share = BN_new();

        char private_key[513];
        char hexa[] = {
            '0','1','2','3','4','5','6','7',
            '8','9','A','B','C','D','E','F'
        };

        for (int i = 0; i < 512; i++)
            private_key[i] = hexa[rand() % 16];

        private_key[512] = '\0';

        BN_hex2bn(&mallory_sec, private_key);
        sq_mult(mallory_sec, mallory_share, ctx);

        char *mallory_share_hex = BN_bn2hex(mallory_share);

        if (mallory_share_hex == NULL) {
            BN_free(mallory_sec);
            BN_free(mallory_share);
            BN_CTX_free(ctx);
            free_dh_params();
            goto cleanup;
        }

        uint32_t mallory_share_len = strlen(mallory_share_hex);

        write_full(client_socket, &signature_len, sizeof(signature_len));
        write_full(client_socket, signature, signature_len);
        write_full(client_socket, &mallory_share_len, sizeof(mallory_share_len));
        write_full(client_socket, mallory_share_hex, mallory_share_len);

        unsigned char buffer[1024];
        int n = read(client_socket, buffer, sizeof(buffer));

        if (n <= 0) {
            printf("\n[MALLORY] MITM attack failed\n");
            printf("[MALLORY] Client rejected the substituted DH share\n");
        } else {
            printf("\n[MALLORY] MITM attack failed\n");
        }

        OPENSSL_free(mallory_share_hex);

        BN_free(mallory_sec);
        BN_free(mallory_share);
        BN_CTX_free(ctx);

        free_dh_params();
    } else {
        printf("\nInvalid choice\n");
    }

cleanup:
    close(client_socket);
    close(server_socket);
    close(s);

    return 0;
}