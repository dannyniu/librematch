/* DannyNiu/NJF, 2025-07-23. Public Domain. */

#include "librematch.c"

const char subj[] = "aaaabaaabaa";

int main()
{
    re_atom_t *regex;
    size_t groupno[1];

    //regcomp_extended(&regex, "^[ab]*?baa$", groupno, 0, '\\');
    regcomp_extended(&regex, "aa^a|^aaaabaaa|aaaabaaabaa$", groupno, 0, '\\');
    for(int i=0; regex[i].type; i++)
        printf("%d %02x\n", regex[i].type, regex[i].value);
    printf("\n");

    match_ctx_t root;
    root.parent = NULL;
    root.root = NULL;
    root.atom = regex;
    root.atom_offset = 0;
    root.overall = root.rm_so = root.rm_eo = -1;
    root.record = NULL;
    root.q = 0;
    root.stack_depth = 0;

    libre_match_t match[1];
    match[0].q = -1;
    match[0].rm_so = -1;
    match[0].rm_eo = -1;

    int subret = match_atom_withq(subj, strlen(subj), 0, match, 1, fRegMinimal(0), &root);
    printf("%d %td %td\n", subret, match->rm_so, match->rm_eo);

    return 0;
}
