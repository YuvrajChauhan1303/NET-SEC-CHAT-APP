#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "services.h"
#include "aes.h"

#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

void send_command(int s, char *buf, unsigned char *aes_key)
{
    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];

    int encrypted_len = encrypt_message((unsigned char *)buf, strlen(buf), aes_key, encrypted);

    if (encrypted_len < 0)
        return;

    write(s, encrypted, encrypted_len);
}

int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size)
{
    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];

    int n = read(s, encrypted, sizeof(encrypted));

    if (n <= 0)
        return n;

    int plaintext_len = decrypt_message(encrypted, n, aes_key, (unsigned char *)buf);

    if (plaintext_len < 0)
        return -1;

    if (plaintext_len >= buf_size)
        return -1;

    buf[plaintext_len] = '\0';

    return plaintext_len;
}