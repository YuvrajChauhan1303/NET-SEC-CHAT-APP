#include <openssl/bn.h>



void sq_mult(const BIGNUM *g, const BIGNUM *a, const BIGNUM *p, BIGNUM *share, BN_CTX *ctx);

void secret_maker(const BIGNUM *ga, const BIGNUM *b, const BIGNUM *p, BIGNUM *secret, BN_CTX *ctx);