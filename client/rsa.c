#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "rsa.h"

int rsa_generate_keypair(const char *private_key_path, const char *public_key_path)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    EVP_PKEY *key = NULL;

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY_keygen(ctx, &key);

    FILE *private_file = fopen(private_key_path, "wb");
    PEM_write_PrivateKey(private_file, key, NULL, NULL, 0, NULL, NULL);
    fclose(private_file);

    FILE *public_file = fopen(public_key_path, "wb");
    PEM_write_PUBKEY(public_file, key);
    fclose(public_file);

    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(ctx);

    return 0;
}

int rsa_load_private_key(const char *private_key_path, EVP_PKEY **key)
{
    FILE *file = fopen(private_key_path, "rb");
    *key = PEM_read_PrivateKey(file, NULL, NULL, NULL);
    fclose(file);
    return 0;
}

int rsa_load_public_key(const char *public_key_path, EVP_PKEY **key)
{
    FILE *file = fopen(public_key_path, "rb");
    *key = PEM_read_PUBKEY(file, NULL, NULL, NULL);
    fclose(file);
    return 0;
}

int rsa_encrypt(EVP_PKEY *public_key, const unsigned char *plaintext, size_t plaintext_len, unsigned char **ciphertext, size_t *ciphertext_len)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(public_key, NULL);
    size_t len = 0;

    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
    EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

    EVP_PKEY_encrypt(ctx, NULL, &len, plaintext, plaintext_len);

    *ciphertext = malloc(len);

    EVP_PKEY_encrypt(ctx, *ciphertext, &len, plaintext, plaintext_len);
    *ciphertext_len = len;

    EVP_PKEY_CTX_free(ctx);

    return 0;
}

int rsa_decrypt(EVP_PKEY *private_key, const unsigned char *ciphertext, size_t ciphertext_len, unsigned char **plaintext, size_t *plaintext_len)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(private_key, NULL);
    size_t len = 0;

    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
    EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

    EVP_PKEY_decrypt(ctx, NULL, &len, ciphertext, ciphertext_len);

    *plaintext = malloc(len);

    EVP_PKEY_decrypt(ctx, *plaintext, &len, ciphertext, ciphertext_len);
    *plaintext_len = len;

    EVP_PKEY_CTX_free(ctx);

    return 0;
}
