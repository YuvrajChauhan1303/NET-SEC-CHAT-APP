#include <stdio.h>
#include <string.h>

#include <openssl/bn.h>

#include "crypto.h"


int main(void)
{
    BIGNUM *shared_secret = BN_new();

    if (shared_secret == NULL)
    {
        printf("Failed to create shared secret\n");
        return 1;
    }


    BN_hex2bn(&shared_secret,
        "3920317a893721540b6eccbb61415c7340e6cd7231135983e2650d532bb875f0" // this is just a demo key, the real hashkey needs to be brought here
    );


    printf("\nDH SHARED SECRET-----------------------\n");

    printf("Shared secret: ");

    BN_print_fp( stdout, shared_secret);

    printf("\n");

    unsigned char aes_key[AES_KEY_SIZE];


    if (derive_aes_key(shared_secret, aes_key) != 1)
    {
        printf("AES key derivation failed\n");

        BN_free(shared_secret);

        return 1;
    }


    printf("\nAES KEY---------------------------\n");

    print_hex( "AES key: ",aes_key, AES_KEY_SIZE);

    const char *msg =
        "Hello, this is a secret message!";

    int msg_len = strlen(msg);


    printf("\n PLAINTEXT -----------------------------\n");

    printf("Message: %s\n", msg);

    printf("Length: %d bytes\n", msg_len);


    unsigned char iv[GCM_IV_SIZE];

    unsigned char ciphertext[1024];

    unsigned char tag[GCM_TAG_SIZE];

    unsigned char decrypted[1024];

    int ciphertext_len = aes_encrypt(
        (unsigned char *)msg,
        msg_len,
        aes_key,
        iv,
        ciphertext,
        tag
    );


    if (ciphertext_len < 0)
    {
        printf("\nEncryption failed!\n");

        BN_free(shared_secret);

        return 1;
    }

    printf("\nENCRYPTION-----------------------\n");

    print_hex("IV:", iv, GCM_IV_SIZE);

    print_hex( "Ciphertext: ", ciphertext, ciphertext_len );

    print_hex( "Tag:        ", tag, GCM_TAG_SIZE );

    int decrypted_len = aes_decrypt( ciphertext, ciphertext_len, aes_key, iv, tag, decrypted);


    if (decrypted_len < 0)
    {
        printf("\nDecryption failed!\n");

        BN_free(shared_secret);

        return 1;
    }

    decrypted[decrypted_len] = '\0';


    printf("\nDECRYPTION -----------------------------\n");

    printf("Decrypted: %s\n",
        decrypted
    );

    printf( "Length: %d bytes\n",
        decrypted_len
    );


    printf("\nVERIFICATION -------------------------\n");


    if (msg_len == decrypted_len &&
        memcmp(
            msg,
            decrypted,
            msg_len) == 0)
    {
        printf("SUCCESS: Original and decrypted messages match!\n");
    }
    else
    {
        printf("FAILURE: Messages do not match!\n");

        BN_free(shared_secret);

        return 1;
    }

    BN_free(shared_secret);


    return 0;
}