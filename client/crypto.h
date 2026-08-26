#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/bn.h>

#define AES_KEY_SIZE 32
#define GCM_IV_SIZE 12
#define GCM_TAG_SIZE 16

#define AES_KEY_SIZE 32
#define GCM_IV_SIZE 12
#define GCM_TAG_SIZE 16

int derive_aes_key( const BIGNUM *shared_secret,unsigned char *aes_key);

int aes_encrypt(
    const unsigned char *plaintext,
    int plaintext_len,

    const unsigned char *aes_key,

    unsigned char *iv,

    unsigned char *ciphertext,

    unsigned char *tag
);


int aes_decrypt(
    const unsigned char *ciphertext,
    int ciphertext_len,

    const unsigned char *aes_key,

    const unsigned char *iv,

    const unsigned char *tag,

    unsigned char *plaintext
);

void print_hex(
    const char *label,

    const unsigned char *data,
    
    int len
);

#endif