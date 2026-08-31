#include <stddef.h>
#include <openssl/evp.h>

int rsa_generate_keypair(const char *private_key_path, const char *public_key_path);

int rsa_load_private_key(const char *private_key_path, EVP_PKEY **key);
int rsa_load_public_key(const char *public_key_path, EVP_PKEY **key);

int rsa_encrypt(EVP_PKEY *public_key, const unsigned char *plaintext, size_t plaintext_len, unsigned char **ciphertext, size_t *ciphertext_len);
int rsa_decrypt(EVP_PKEY *private_key, const unsigned char *ciphertext, size_t ciphertext_len, unsigned char **plaintext, size_t *plaintext_len);