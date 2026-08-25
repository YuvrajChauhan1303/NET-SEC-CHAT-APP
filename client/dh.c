#include <stdio.h>
#include <openssl/bn.h>

void sq_mult(const BIGNUM *g, const BIGNUM *a, const BIGNUM *p, BIGNUM *share, BN_CTX *ctx)
{
    BIGNUM *base = BN_new();

    if (base == NULL) {
        printf("BN_new failed\n");
        return;
    }

    BN_copy(base, g);

    int n = BN_num_bits(a);

    for (int i = n - 2; i >= 0; i--)
    {
        BN_mod_mul(base, base, base, p, ctx);

        if (BN_is_bit_set(a, i))
        {
            BN_mod_mul(base, base, g, p, ctx);
        }
    }

    BN_copy(share, base);

    BN_free(base);
}
void secret_maker(const BIGNUM *ga, const BIGNUM *b, const BIGNUM *p, BIGNUM *secret, BN_CTX *ctx)
{
    sq_mult(ga, b, p, secret, ctx);
}

int main(void)
{
    BN_CTX *ctx = BN_CTX_new();

    if (ctx == NULL) {
        printf("BN_CTX_new failed\n");
        return 1;
    }

    BIGNUM *g = BN_new();
    BIGNUM *a = BN_new();
    BIGNUM *b = BN_new();
    BIGNUM *p = BN_new();

    BIGNUM *share_a = BN_new();
    BIGNUM *share_b = BN_new();

    BIGNUM *secret_a = BN_new();
    BIGNUM *secret_b = BN_new();

    if (!g || !a || !b || !p || !share_a || !share_b || !secret_a || !secret_b)
    {
        printf("BN_new failed\n");
        return 1;
    }

    BN_set_word(g, 9);
    BN_set_word(a, 35);
    BN_set_word(b, 45);
    BN_set_word(p, 7);

    sq_mult(g, a, p, share_a, ctx);
    sq_mult(g, b, p, share_b, ctx);

    printf("share_a: ");
    BN_print_fp(stdout, share_a);
    printf("\n");

    printf("share_b: ");
    BN_print_fp(stdout, share_b);
    printf("\n");

    secret_maker(share_b, a, p, secret_a, ctx);
    secret_maker(share_a, b, p, secret_b, ctx);

    printf("secret_a: ");
    BN_print_fp(stdout, secret_a);
    printf("\n");

    printf("secret_b: ");
    BN_print_fp(stdout, secret_b);
    printf("\n");

    if (BN_cmp(secret_a, secret_b) == 0)
        printf("DH successful!\n");
    else
        printf("DH FAILED!\n");

    BN_free(g);
    BN_free(a);
    BN_free(b);
    BN_free(p);

    BN_free(share_a);
    BN_free(share_b);

    BN_free(secret_a);
    BN_free(secret_b);

    BN_CTX_free(ctx);

    return 0;
}