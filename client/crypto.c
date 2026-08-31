#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "crypto.h"


void print_hex(const char *label,const unsigned char *data,int len){

    printf("%s", label);
    for(int i =0 ; i < len; i++){
        printf("%02x", data[i]);
    }

    printf("\n");
}


int derive_aes_key(const BIGNUM *shared_secret, unsigned char *aes_key)
{
    // hexadecimal representation of the shared key
    char *hex_secret;

    hex_secret = BN_bn2hex(shared_secret);
    if(hex_secret == NULL){

        printf("Failed to convert shared secret to hex\n");
        return 0;
    }

    SHA256((unsigned char * ) hex_secret, strlen(hex_secret), aes_key); //hex_secret is the data being hashed ands stored in aes_key
    OPENSSL_free(hex_secret);

    return 1;
}


int aes_encrypt(
    const unsigned char *plaintext,
    int plaintext_len,
    const unsigned char *aes_key,

    unsigned char *iv,
    unsigned char *ciphertext,
    unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int ciphertext_len;

    ctx = EVP_CIPHER_CTX_new();

    // creating a context to work on here
    if(ctx == NULL){

        printf("Failed to create encryption context\n");

        return -1;
    }

    // IV of 12 byte
    if(RAND_bytes(iv, GCM_IV_SIZE) != 1){

        printf("Failed to generate IV\n");

        return -1;
    }

    
    if(EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    {
        printf("failed to initialize AES-GCM\n");
        
        EVP_CIPHER_CTX_free(ctx);

        return -1;
    }


    if(EVP_EncryptInit_ex(ctx, NULL, NULL, aes_key, iv) != 1)
    {
        printf("FAiled to setAES and IV\n");
        EVP_CIPHER_CTX_free(ctx);

        return -1;
    }

    //encrypt the plain text
    if(EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
    {
        printf("Encryption failed\n");

        EVP_CIPHER_CTX_free(ctx);

        return -1;

    }

    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &len) != 1)
    {
        printf("Encryption finalization failed\n");

        EVP_CIPHER_CTX_free(ctx);

        return -1;
    }

    ciphertext_len += len;

    // the tag here is used for authenticity/integrity
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, GCM_TAG_SIZE, tag) != 1)
    {

        printf("failed to obtain atuh tag\n");

        EVP_CIPHER_CTX_free(ctx);

        return -1;

    }

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;

}


int aes_decrypt(
    
    const unsigned char *ciphertext,
    int ciphertext_len,

    const unsigned char *aes_key,
    const unsigned char *iv,
    const unsigned char *tag,

    unsigned char *plaintext )
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int plaintext_len;
    int ret;

    ctx = EVP_CIPHER_CTX_new();

    if(ctx == NULL)
    {
        printf("FAiled to create decryption \n");

        return -1;
    }

    // initialize decry.
    if(EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    {
    printf("Failed to init AES-GCM\n");
        
    EVP_CIPHER_CTX_free(ctx);

        return -1;
    }

    if(EVP_DecryptInit_ex(ctx, NULL, NULL, aes_key, iv) != 1)
    {
        printf("Failed to set aes key and iv \n");
        EVP_CIPHER_CTX_free(ctx);

        return -1;
    }
    if( EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
    {
        printf("Decryption failed \n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    plaintext_len = len;

    //verifying the tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_SIZE, (void *)tag) != 1)
    {
        printf("Failed to set auhentication tag\n");
        EVP_CIPHER_CTX_free(ctx);

    return -1;
    }

    //final decryption
    ret = EVP_DecryptFinal_ex( ctx, plaintext + plaintext_len, &len );

    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0)
    {
        return -1;
    }

    plaintext_len += len;

    return plaintext_len;

}