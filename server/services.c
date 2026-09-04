#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "services.h"
#include "aes.h"

#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

void write_all(int s, void *buf, int len)
{
    int sent = 0;

    while (sent < len)
    {
        int n = write(s, (char *)buf + sent, len - sent);

        if (n <= 0)
            return;

        sent += n;
    }
}

int read_all(int s, void *buf, int len)
{
    int received = 0;

    while (received < len)
    {
        int n = read(s, (char *)buf + received, len - received);

        if (n <= 0)
            return n;

        received += n;
    }

    return received;
}

void send_command(int s, char *buf, unsigned char *aes_key)
{
    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];

    int encrypted_len = encrypt_message((unsigned char *)buf, strlen(buf), aes_key, encrypted);

    if (encrypted_len < 0)
        return;

    uint32_t network_len = htonl(encrypted_len);

    write_all(s, &network_len, sizeof(network_len));
    write_all(s, encrypted, encrypted_len);
}

int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size)
{
    uint32_t network_len;

    int n = read_all(s, &network_len, sizeof(network_len));

    if (n <= 0)
        return n;

    int encrypted_len = ntohl(network_len);

    if (encrypted_len <= 0 || encrypted_len > ENCRYPTED_BUFFER_SIZE)
        return -1;

    unsigned char encrypted[ENCRYPTED_BUFFER_SIZE];

    n = read_all(s, encrypted, encrypted_len);

    if (n <= 0)
        return n;

    int plaintext_len = decrypt_message(encrypted, encrypted_len, aes_key, (unsigned char *)buf);

    if (plaintext_len < 0)
        return -1;

    if (plaintext_len >= buf_size)
        return -1;

    buf[plaintext_len] = '\0';

    return plaintext_len;
}