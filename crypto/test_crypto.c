#include <stdio.h>
#include <string.h>

#include "crypto.h"

int main()
{
    unsigned char key[AES_KEY_SIZE];

    for (int i = 0; i < AES_KEY_SIZE; i++)
        key[i] = i;

    char *message = "Hello C2";

    unsigned char encrypted[1024];
    unsigned char decrypted[1024];

    int encrypted_len = encrypt_message(
        (unsigned char *)message,
        strlen(message),
        key,
        encrypted
    );

    printf("Encrypted length: %d\n",
           encrypted_len);

    int decrypted_len = decrypt_message(
        encrypted,
        encrypted_len,
        key,
        decrypted
    );

    if (decrypted_len < 0)
    {
        printf("Decryption failed\n");
        return 1;
    }

    decrypted[decrypted_len] = '\0';

    printf("Decrypted: %s\n",
           decrypted);

    return 0;
}