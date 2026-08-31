#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "rsa.h"

int main()
{
    const char *private_key_path = "test-private.pem";
    const char *public_key_path = "test-public.pem";
    const unsigned char message[] = "Hello Bob, this is Alice.";
    size_t message_len = strlen((const char *)message);
    unsigned char *ciphertext = NULL;
    unsigned char *plaintext = NULL;
    size_t ciphertext_len = 0;
    size_t plaintext_len = 0;
    EVP_PKEY *private_key = NULL;
    EVP_PKEY *public_key = NULL;

    printf("Generating RSA key pair...\n");
    rsa_generate_keypair(private_key_path, public_key_path);

    printf("RSA key pair generated\n");

    rsa_load_private_key(private_key_path, &private_key);
    rsa_load_public_key(public_key_path, &public_key);

    printf("Keys loaded successfully\n");
    printf("Original message: %s\n", message);

    rsa_encrypt(public_key, message, message_len, &ciphertext, &ciphertext_len);

    printf("Encryption successful\n");
    printf("Ciphertext length: %zu bytes\n", ciphertext_len);
    printf("Ciphertext: ");

    for (size_t i = 0; i < ciphertext_len; i++)
        printf("%02x", ciphertext[i]);

    printf("\n");

    rsa_decrypt(private_key, ciphertext, ciphertext_len, &plaintext, &plaintext_len);

    printf("Decryption successful\n");
    printf("Decrypted message: %.*s\n", (int)plaintext_len, plaintext);

    if (plaintext_len == message_len && memcmp(message, plaintext, message_len) == 0)
        printf("TEST PASSED\n");
    else
        printf("TEST FAILED\n");

    free(ciphertext);
    free(plaintext);
    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);

    return 0;
}