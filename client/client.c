#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <pthread.h>

#include "services.h"
#include "dh.h"
#include "aes.h"
#include "cert.h"
#include "rsa.h"
#include "e2e.h"

struct e2e_timer_data
{
    int s;
    unsigned char *aes_key;
    char *current_peer;
    BN_CTX *ctx;
};

void *e2e_timer(void *arg)
{
    struct e2e_timer_data *data = arg;

    while (1)
    {
        sleep(60);

        if (data->current_peer[0] == '\0')
            continue;

        char packet[E2E_PACKET_SIZE];

        if (e2e_create_init(data->current_peer, packet, sizeof(packet), data->ctx))
        {
            send_command(data->s, packet, data->aes_key);
            printf("\nE2E request automatically sent to %s.\n", data->current_peer);
        }
    }

    return NULL;
}

int main()
{
    printf("Enter your username: ");

    char username[1000];

    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    EVP_PKEY *client_key = generate_client_keys();
    X509_REQ *client_csr = generate_client_csr(client_key, username);

    save_client_key(client_key);
    save_client_csr(client_csr);

    X509 *client_cert = request_signed_certificate(client_csr);
    save_client_certificate(client_cert);

    X509_free(client_cert);
    X509_REQ_free(client_csr);
    EVP_PKEY_free(client_key);

    int s;
    struct sockaddr_in server_addr;
    struct hostent *host;

    s = socket(AF_INET, SOCK_STREAM, 0);

    host = gethostbyname("server");

    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    server_addr.sin_port = htons(8080);

    init_dh_params();

    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *client_sec = BN_new();
    BIGNUM *secret = BN_new();
    BIGNUM *share = BN_new();

    char x[513];
    char *hexa = "0123456789ABCDEF";

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < 512; i++)
        x[i] = hexa[rand() % 16];

    x[512] = '\0';

    BN_hex2bn(&client_sec, x);
    sq_mult(client_sec, share, ctx);

    X509 *ca_cert = download_ca_certificate();

    printf("[CLIENT] CA certificate downloaded.\n");

    connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[CLIENT] Connected to server.\n");

    uint32_t cert_len;
    uint32_t network_cert_len;

    read_all(s, &network_cert_len, sizeof(network_cert_len));

    cert_len = ntohl(network_cert_len);

    unsigned char *cert_data = malloc(cert_len);

    read_all(s, cert_data, cert_len);

    const unsigned char *p = cert_data;

    X509 *server_cert = d2i_X509(NULL, &p, cert_len);

    free(cert_data);

    printf("[CLIENT] Server certificate received.\n");
    printf("[CLIENT] Server certificate parsed.\n");

    if (!validate_server_certificate(server_cert, ca_cert))
    {
        printf("[CLIENT] Server certificate validation failed.\n");
        printf("[CLIENT] Closing connection.\n");

        close(s);

        X509_free(ca_cert);
        X509_free(server_cert);

        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);

        BN_CTX_free(ctx);
        free_dh_params();

        return 1;
    }

    printf("[CLIENT] Server certificate verified successfully.\n");

    unsigned char challenge[32];

    generate_challenge(challenge);
    write_all(s, challenge, sizeof(challenge));

    uint32_t signature_len;
    uint32_t network_signature_len;

    read_all(s, &network_signature_len, sizeof(network_signature_len));

    signature_len = ntohl(network_signature_len);

    unsigned char signature[256];

    read_all(s, signature, signature_len);

    if (!verify_challenge(server_cert, challenge, 32, signature, signature_len))
    {
        printf("[CLIENT] Server proof-of-possession failed.\n");

        close(s);

        X509_free(ca_cert);
        X509_free(server_cert);

        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);

        BN_CTX_free(ctx);
        free_dh_params();

        return 1;
    }

    printf("[CLIENT] Server proof-of-possession verified.\n");

    char buf[E2E_PACKET_SIZE];

    char *share_hex = BN_bn2hex(share);

    uint32_t share_len = strlen(share_hex);
    uint32_t network_share_len = htonl(share_len);

    write_all(s, &network_share_len, sizeof(network_share_len));
    write_all(s, share_hex, share_len);

    OPENSSL_free(share_hex);

    uint32_t server_share_len;
    uint32_t network_server_share_len;

    read_all(s, &network_server_share_len, sizeof(network_server_share_len));

    server_share_len = ntohl(network_server_share_len);

    read_all(s, buf, server_share_len);

    buf[server_share_len] = '\0';

    BIGNUM *server_share = BN_new();

    BN_hex2bn(&server_share, buf);

    BIGNUM *KEY = BN_new();

    secret_maker(server_share, client_sec, KEY, ctx);

    unsigned char aes_key[AES_KEY_SIZE];

    derive_aes_key(KEY, aes_key);

    printf("\nAES KEY\n");
    print_hex("", aes_key, AES_KEY_SIZE);
    printf("\n");

    print_key_fingerprint(aes_key);

    printf("\n");

    while (1)
    {
        strcpy(buf, username);

        send_command(s, buf, aes_key);

        int n = receive_command(s, aes_key, buf, sizeof(buf));

        if (n == 0)
        {
            printf("Server closed connection during registration\n");

            close(s);

            return 1;
        }

        if (n < 0)
        {
            printf("Failed to receive/decrypt registration response\n");

            close(s);

            return 1;
        }

        printf("Server: %s\n", buf);

        if (strstr(buf, "User Registration Successful.") != NULL)
            break;
    }

    if (fork() == 0)
    {
        while (1)
        {
            int n = receive_command(s, aes_key, buf, sizeof(buf));

            if (n <= 0)
                break;

            printf("\n[CLIENT %s] %s\n", username, buf);

            char sender[100];
            char message[E2E_PACKET_SIZE];

            char *colon = strchr(buf, ':');

            if (colon != NULL)
            {
                int sender_len = colon - buf;

                if (sender_len >= sizeof(sender))
                    continue;

                memcpy(sender, buf, sender_len);
                sender[sender_len] = '\0';

                if (colon[1] == ' ')
                    strcpy(message, colon + 2);
                else
                    strcpy(message, colon + 1);

                if (e2e_is_init(message))
                {
                    if (e2e_process_init(sender, message + strlen(E2E_INIT_TAG)))
                        printf("\nE2E INIT from %s processed successfully. Type /e2e %s to respond.\n", sender, sender);

                    continue;
                }

                if (e2e_is_ack(message))
                {
                    if (e2e_process_ack(sender, message + strlen(E2E_ACK_TAG), ctx))
                        printf("\nE2E key established with %s.\n", sender);

                    continue;
                }

                if (e2e_is_msg(message))
                {
                    char plaintext[1000];

                    if (e2e_decrypt(sender, message + strlen(E2E_MSG_TAG), plaintext, sizeof(plaintext)))
                        printf("\n%s: %s\n", sender, plaintext);

                    continue;
                }
            }

            printf("Server:\n\n%s\n", buf);
        }

        close(s);

        return 0;
    }
    else
    {
        char current_peer[100] = "";

        struct e2e_timer_data timer_data;
        timer_data.s = s;
        timer_data.aes_key = (char *)aes_key;
        timer_data.current_peer = current_peer;
        timer_data.ctx = ctx;

        pthread_t timer_thread;
        pthread_create(&timer_thread, NULL, e2e_timer, &timer_data);

        while (1)
        {
            {
                if (fgets(buf, sizeof(buf), stdin) == NULL)
                    break;

                buf[strcspn(buf, "\n")] = '\0';

                if (!strncmp(buf, "/e2e ", 5))
                {
                    char *peer = buf + 5;

                    strcpy(current_peer, peer);

                    char packet[E2E_PACKET_SIZE];

                    int pending = e2e_has_pending(peer);

                    printf("Checking pending request for %s: %d\n", peer, pending);

                    if (pending)
                    {
                        if (e2e_create_ack(peer, packet, sizeof(packet), ctx))
                        {
                            send_command(s, packet, aes_key);
                            printf("E2E ACK sent to %s.\n", peer);
                        }
                    }
                    else
                    {
                        if (e2e_create_init(peer, packet, sizeof(packet), ctx))
                        {
                            send_command(s, packet, aes_key);
                            printf("E2E INIT sent to %s.\n", peer);
                        }
                    }

                    continue;
                }

                if (!strcmp(buf, "/quit"))
                {
                    send_command(s, buf, aes_key);

                    close(s);

                    return 0;
                }

                if (current_peer[0] != '\0')
                {
                    char packet[E2E_PACKET_SIZE];

                    if (e2e_encrypt(current_peer, buf, packet, sizeof(packet)))
                    {
                        send_command(s, packet, aes_key);

                        continue;
                    }
                }

                send_command(s, buf, aes_key);
            }
        }

        close(s);

        X509_free(ca_cert);
        X509_free(server_cert);

        BN_free(client_sec);
        BN_free(secret);
        BN_free(share);
        BN_free(server_share);
        BN_free(KEY);

        BN_CTX_free(ctx);

        free_dh_params();

        return 0;
    }
}