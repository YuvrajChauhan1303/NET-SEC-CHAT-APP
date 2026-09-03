#include <openssl/bn.h>

#define E2E_INIT_TAG "__E2E_INIT__"
#define E2E_ACK_TAG "__E2E_ACK__"
#define E2E_MSG_TAG "__E2E_MSG__"

#define E2E_SHARE_SIZE 513
#define E2E_PACKET_SIZE 8192

void e2e_generate_pair(BIGNUM **secret, BIGNUM **share, BN_CTX *ctx);

void e2e_save_secret(const char *peer, BIGNUM *secret);
BIGNUM *e2e_load_secret(const char *peer);

void e2e_save_pending(const char *peer, const char *share);
int e2e_load_pending(const char *peer, char *share);
void e2e_remove_pending(const char *peer);
int e2e_has_pending(const char *peer);

void e2e_save_key(const char *peer, unsigned char *key);
int e2e_load_key(const char *peer, unsigned char *key);

int e2e_create_init(const char *peer, char *packet, int packet_size, BN_CTX *ctx);
int e2e_process_init(const char *peer, const char *share);

int e2e_create_ack(const char *peer, char *packet, int packet_size, BN_CTX *ctx);
int e2e_process_ack(const char *peer, const char *share, BN_CTX *ctx);

int e2e_encrypt(const char *peer, const char *message, char *packet, int packet_size);
int e2e_decrypt(const char *peer, const char *data, char *message, int message_size);

int e2e_is_init(const char *message);
int e2e_is_ack(const char *message);
int e2e_is_msg(const char *message);