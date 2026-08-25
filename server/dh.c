#include<stdio.h>
#include<string.h>

#include "dh.h"


void sq_mult(int g, int a, int p, int *share)
{
    char secret[1024];
    int_to_bin(a, secret); 
    // printf("%s\n", secret);

    int n = strlen(secret);

    int base = g;
    int i = 1;

    while(1)
    {
        if (secret[i] == '\0')
            break;

        base = base * base;
        base = base % p;

        if(secret[i] == '1'){     
            base = base * g;
            base = base % p;
        }
        printf("base:\t%d\n", base);
        i++;
    }

    *share = base;
}


void int_to_bin(int num, char *res)
{
    int i = 0;

    while (num > 0) {
        res[i++] = (num % 2) + '0';
        num /= 2;
    }

    for (int j = 0; j < i / 2; j++) {
        char temp = res[j];
        res[j] = res[i - j - 1];
        res[i - j - 1] = temp;
    }

    res[i] = '\0';
}

void secret_maker(int ga, int b, int p, int *secret)
{
    sq_mult(ga, b, p, secret);
}

int main()
{
    int share_a;
    int share_b;
    int g = 9;
    int a = 35;
    int b = 45;
    int p = 7;
    sq_mult(g, a, p, &share_a);
    sq_mult(g, b, p, &share_b);

    printf("share_a:\t%d\n", share_a); 
    printf("share_b:\t%d\n", share_b);


    int secret_a;
    int secret_b;

    secret_maker(share_a, b, p, &secret_b);
    secret_maker(share_b, a, p, &secret_a);

    printf("secret_a:\t%d\n", secret_a);
    printf("secret_b:\t%d\n", secret_b);
}