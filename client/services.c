#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "services.h"
#include "aes.h"

#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

int read_all(int s, void *buf, int len)
{
    int total = 0;
    int n;

    while (total < len)
    {
        n = read(s, (char *)buf + total, len - total);

        if (n <= 0)
            return n;

        total += n;
    }

    return total;
}

int write_all(int s, void *buf, int len)
{
    int total = 0;
    int n;

    while (total < len)
    {
        n = write(s, (char *)buf + total, len - total);

        if (n <= 0)
            return n;

        total += n;
    }

    return total;
}

void send_command(int s, char *buf, unsigned char *aes_key)
{
    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];
    int encrypted_len;
    uint32_t len;

    encrypted_len = encrypt_message((unsigned char *)buf, strlen(buf), aes_key, encrypted);

    if (encrypted_len < 0)
        return;

    len = htonl(encrypted_len);

    write_all(s, &len, sizeof(len));
    write_all(s, encrypted, encrypted_len);
}

int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size)
{
    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];
    uint32_t len;
    int plaintext_len;

    if (read_all(s, &len, sizeof(len)) <= 0)
        return 0;

    len = ntohl(len);

    if (len > sizeof(encrypted))
        return -1;

    if (read_all(s, encrypted, len) <= 0)
        return 0;

    plaintext_len = decrypt_message(encrypted, len, aes_key, (unsigned char *)buf);

    if (plaintext_len < 0)
        return -1;

    if (plaintext_len >= buf_size)
        return -1;

    buf[plaintext_len] = '\0';

    return plaintext_len;
}