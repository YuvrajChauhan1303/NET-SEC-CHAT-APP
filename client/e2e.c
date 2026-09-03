#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include "e2e.h"
#include "dh.h"
#include "aes.h"

void e2e_generate_pair(BIGNUM **secret, BIGNUM **share, BN_CTX *ctx)
{
    unsigned char random[64];

    RAND_bytes(random, sizeof(random));

    *secret = BN_bin2bn(random, sizeof(random), NULL);
    *share = BN_new();

    sq_mult(*secret, *share, ctx);
}

void e2e_save_secret(const char *peer, BIGNUM *secret)
{
    char path[100];
    char *secret_hex;
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_secret_%s.txt", peer);

    secret_hex = BN_bn2hex(secret);

    f = fopen(path, "w");

    fprintf(f, "%s\n", secret_hex);

    fclose(f);

    OPENSSL_free(secret_hex);
}

BIGNUM *e2e_load_secret(const char *peer)
{
    char path[100];
    char secret_hex[E2E_SHARE_SIZE];
    FILE *f;
    BIGNUM *secret;

    snprintf(path, sizeof(path), "client-key/e2e_secret_%s.txt", peer);

    f = fopen(path, "r");

    if (f == NULL)
        return NULL;

    fscanf(f, "%512s", secret_hex);

    fclose(f);

    secret = BN_new();

    BN_hex2bn(&secret, secret_hex);

    return secret;
}

void e2e_save_pending(const char *peer, const char *share)
{
    char path[100];
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_pending_%s.txt", peer);

    f = fopen(path, "w");

    fprintf(f, "%s\n", share);

    fclose(f);
}

int e2e_load_pending(const char *peer, char *share)
{
    char path[100];
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_pending_%s.txt", peer);

    f = fopen(path, "r");

    if (f == NULL)
        return 0;

    fscanf(f, "%512s", share);

    fclose(f);

    return 1;
}

void e2e_remove_pending(const char *peer)
{
    char path[100];

    snprintf(path, sizeof(path), "client-key/e2e_pending_%s.txt", peer);

    remove(path);
}

int e2e_has_pending(const char *peer)
{
    char path[100];
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_pending_%s.txt", peer);

    f = fopen(path, "r");

    if (f == NULL)
        return 0;

    fclose(f);

    return 1;
}

void e2e_save_key(const char *peer, unsigned char *key)
{
    char path[100];
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_key_%s.txt", peer);

    f = fopen(path, "w");

    for (int i = 0; i < AES_KEY_SIZE; i++)
        fprintf(f, "%02x", key[i]);

    fprintf(f, "\n");

    fclose(f);
}

int e2e_load_key(const char *peer, unsigned char *key)
{
    char path[100];
    char hex[65];
    FILE *f;

    snprintf(path, sizeof(path), "client-key/e2e_key_%s.txt", peer);

    f = fopen(path, "r");

    if (f == NULL)
        return 0;

    fscanf(f, "%64s", hex);

    fclose(f);

    for (int i = 0; i < AES_KEY_SIZE; i++)
        sscanf(hex + i * 2, "%2hhx", &key[i]);

    return 1;
}

int e2e_create_init(const char *peer, char *packet, int packet_size, BN_CTX *ctx)
{
    BIGNUM *secret;
    BIGNUM *share;
    char *share_hex;

    e2e_generate_pair(&secret, &share, ctx);

    e2e_save_secret(peer, secret);

    share_hex = BN_bn2hex(share);

    snprintf(packet, packet_size, "@%s %s%s", peer, E2E_INIT_TAG, share_hex);

    OPENSSL_free(share_hex);

    BN_free(secret);
    BN_free(share);

    return 1;
}

int e2e_process_init(const char *peer, const char *share)
{
    e2e_save_pending(peer, share);

    printf("E2E INIT from %s processed successfully. Type /e2e %s to respond.\n", peer, peer);

    return 1;
}

int e2e_create_ack(const char *peer, char *packet, int packet_size, BN_CTX *ctx)
{
    char pending_share[E2E_SHARE_SIZE];
    BIGNUM *peer_share;
    BIGNUM *secret;
    BIGNUM *share;
    BIGNUM *shared_secret;
    unsigned char aes_key[AES_KEY_SIZE];
    char *share_hex;

    if (!e2e_load_pending(peer, pending_share))
        return 0;

    peer_share = BN_new();

    BN_hex2bn(&peer_share, pending_share);

    e2e_generate_pair(&secret, &share, ctx);

    e2e_save_secret(peer, secret);

    shared_secret = BN_new();

    secret_maker(peer_share, secret, shared_secret, ctx);

    derive_aes_key(shared_secret, aes_key);

    printf("E2E key fingerprint for %s: ", peer);
    print_key_fingerprint(aes_key);

    e2e_save_key(peer, aes_key);

    share_hex = BN_bn2hex(share);

    snprintf(packet, packet_size, "@%s %s%s", peer, E2E_ACK_TAG, share_hex);

    e2e_remove_pending(peer);

    OPENSSL_free(share_hex);

    BN_free(peer_share);
    BN_free(secret);
    BN_free(share);
    BN_free(shared_secret);

    return 1;
}

int e2e_process_ack(const char *peer, const char *share, BN_CTX *ctx)
{
    BIGNUM *peer_share;
    BIGNUM *secret;
    BIGNUM *shared_secret;
    unsigned char aes_key[AES_KEY_SIZE];

    peer_share = BN_new();

    BN_hex2bn(&peer_share, share);

    secret = e2e_load_secret(peer);

    if (secret == NULL)
    {
        BN_free(peer_share);
        return 0;
    }

    shared_secret = BN_new();

    secret_maker(peer_share, secret, shared_secret, ctx);

    derive_aes_key(shared_secret, aes_key);

    printf("E2E key fingerprint for %s: ", peer);
    print_key_fingerprint(aes_key);

    e2e_save_key(peer, aes_key);

    BN_free(peer_share);
    BN_free(secret);
    BN_free(shared_secret);

    printf("E2E key established with %s.\n", peer);

    return 1;
}

int e2e_encrypt(const char *peer, const char *message, char *packet, int packet_size)
{
    unsigned char key[AES_KEY_SIZE];
    unsigned char encrypted[4096];
    int encrypted_len;
    int offset;

    if (!e2e_load_key(peer, key))
        return 0;

    encrypted_len = encrypt_message((unsigned char *)message, strlen(message), key, encrypted);

    if (encrypted_len < 0)
        return 0;

    offset = snprintf(packet, packet_size, "@%s %s", peer, E2E_MSG_TAG);

    for (int i = 0; i < encrypted_len; i++)
        offset += snprintf(packet + offset, packet_size - offset, "%02x", encrypted[i]);

    return offset;
}

int e2e_decrypt(const char *peer, const char *data, char *message, int message_size)
{
    unsigned char key[AES_KEY_SIZE];
    unsigned char encrypted[4096];
    int encrypted_len;
    int plaintext_len;
    int data_len;

    if (!e2e_load_key(peer, key))
        return 0;

    data_len = strlen(data);

    if (data_len % 2 != 0)
        return 0;

    encrypted_len = data_len / 2;

    if (encrypted_len > sizeof(encrypted))
        return 0;

    for (int i = 0; i < encrypted_len; i++)
        sscanf(data + i * 2, "%2hhx", &encrypted[i]);

    plaintext_len = decrypt_message(encrypted, encrypted_len, key, (unsigned char *)message);

    if (plaintext_len < 0 || plaintext_len >= message_size)
        return 0;

    message[plaintext_len] = '\0';

    return plaintext_len;
}

int e2e_is_init(const char *message)
{
    return !strncmp(message, E2E_INIT_TAG, strlen(E2E_INIT_TAG));
}

int e2e_is_ack(const char *message)
{
    return !strncmp(message, E2E_ACK_TAG, strlen(E2E_ACK_TAG));
}

int e2e_is_msg(const char *message)
{
    return !strncmp(message, E2E_MSG_TAG, strlen(E2E_MSG_TAG));
}