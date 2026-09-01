#include <string.h>
#include <unistd.h>

#include "services.h"
#include "../crypto/crypto.h"

char *read_command(int c, char *buf, unsigned char *aes_key)
{
    unsigned char encrypted[2048];

    int n = read(c, encrypted, sizeof(encrypted));

    if (n <= 0)
        return NULL;

    int plaintext_len = decrypt_message(encrypted, n, aes_key, (unsigned char *)buf);

    if (plaintext_len < 0)
        return NULL;

    buf[plaintext_len] = '\0';

    return buf;
}

void send_command(int c, char *buf, unsigned char *aes_key)
{
    unsigned char encrypted[2048];

    int encrypted_len = encrypt_message((unsigned char *)buf, strlen(buf), aes_key, encrypted);

    if (encrypted_len < 0)
        return;

    write(c, encrypted, encrypted_len);
}