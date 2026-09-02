#include <openssl/bn.h>

extern BIGNUM *MODP;
extern BIGNUM *G;

void init_dh_params(void);
void free_dh_params(void);

void sq_mult(const BIGNUM *a, BIGNUM *share, BN_CTX *ctx);

void secret_maker(const BIGNUM *ga,const BIGNUM *b, BIGNUM *secret, BN_CTX *ctx);