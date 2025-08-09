/* DannyNiu/NJF, 2025-06-21. Public Domain. */

#include "regcomp.h"

void checkbe(const char *expr)
{
    bracket_t *be;
    ptrdiff_t ret;
    int i;

    ret = regcomp_brackets(&be, expr+1);
    printf("%td %s:", ret, expr);
    if( ret < 0 ) return;

    for(i=32; i<127; i++)
        if( (be->cbits[i / 32] >> (i % 32)) & 1 )
            printf("%c", i);
    printf("%c\n", expr[ret+1]);
    regfree_brackets(be);
}

int main()
{
    checkbe("[-ac]");
    checkbe("[ac-]");
    checkbe("[^-ac]");
    checkbe("[^ac-]");
    checkbe("[%--]");
    checkbe("[--@]");
    checkbe("[a--@]");
    checkbe("[][.-.]-0]");
    checkbe("[]-]");
    checkbe("[^-[=]=]]");
    checkbe("[[:alpha:]]");
    checkbe("[[:punct:]]");
    checkbe("[[:upper:][:lower:]]");
    checkbe("[^[:upper:][:lower:]]");
    checkbe("[[.^.][:digit:]]");
    checkbe("[[.^.]&[:digit:]]");

    return 0;
}
