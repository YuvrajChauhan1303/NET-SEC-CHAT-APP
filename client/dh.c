#include <stdio.h>
#include <openssl/bn.h>

#include "dh.h"

BIGNUM *MODP = NULL;
BIGNUM *G = NULL;

void init_dh_params(void)
{
    BN_hex2bn(&MODP,
        "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
        "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
        "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
        "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
        "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
        "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
        "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
        "670C354E4ABC9804F1746C08CA18217C32905E46E36CE3B"
        "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
        "DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
        "15728E5A8AACAA68FFFFFFFFFFFFFFFF"
    );

    G = BN_new();
    BN_set_word(G, 2);
}

void free_dh_params(void)
{
    BN_free(MODP);
    BN_free(G);
}


void sq_mult(const BIGNUM *a, BIGNUM *share, BN_CTX *ctx)
{
    BIGNUM *base = BN_new();
    BIGNUM *temp = BN_new();

    if (base == NULL || temp == NULL)
    {
        printf("BN_new failed\n");
        return;
    }

    BN_copy(base, G);

    int n = BN_num_bits(a);

    for (int i = n - 2; i >= 0; i--)
    {
        BN_mul(temp, base, base, ctx);
        BN_mod(base, temp, MODP, ctx);

        if (BN_is_bit_set(a, i))
        {
            BN_mul(temp, base, G, ctx);
            BN_mod(base, temp, MODP, ctx);
        }
    }

    BN_copy(share, base);

    BN_free(temp);
    BN_free(base);
}

void secret_maker(const BIGNUM *ga, const BIGNUM *b, BIGNUM *secret, BN_CTX *ctx)
{
    BIGNUM *base = BN_new();
    BIGNUM *temp = BN_new();

    if (base == NULL || temp == NULL)
    {
        printf("BN_new failed\n");
        return;
    }

    BN_copy(base, ga);

    int n = BN_num_bits(b);

    for (int i = n - 2; i >= 0; i--)
    {
        BN_mul(temp, base, base, ctx);
        BN_mod(base, temp, MODP, ctx);

        if (BN_is_bit_set(b, i))
        {
            BN_mul(temp, base, ga, ctx);
            BN_mod(base, temp, MODP, ctx);
        }
    }

    BN_copy(secret, base);

    BN_free(temp);
    BN_free(base);
}