/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#include "librematch.h"

#define JUST_PRINT_ATOMS
#include "regcomp-ere-check.c"

int main(int argc, char *argv[])
{
    libregexp_t preg;
    int nmatches = atoi(argv[1]);
    libre_match_t *matches = calloc(nmatches, sizeof(libre_match_t));
    int ret;
    libregcomp(&preg, argv[2], LIBREG_EXTENDED);

    printatoms(preg.re_atoms, 0);

    ret = libregexec(&preg, argv[3], nmatches, matches, 0);
    printf("%d.\n", ret);
    for(int i=0; i<nmatches; i++)
        printf("%d, %td, %td.\n", i, matches[i].rm_so, matches[i].rm_eo);

    return 0;
}
