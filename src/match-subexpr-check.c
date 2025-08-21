/* DannyNiu/NJF, 2025-08-03. Public Domain. */

#include "librematch.c"

#define JUST_PRINT_ATOMS
#include "regcomp-ere-check.c"

//const        char subj[] = "aaaabaaabaabcccc";
const          char subj[] = "weeknigh";

int main()
{
    re_atom_t *regex;
    size_t groupno[1] = {0};

    //regcomp_extended(&regex, "(wee|week)(night|knights)", groupno, 0, '\\');

    // TODO (2025-08-18): BUG. FIXED 2025-08-21.
    // The match should've been { 0: (0,8), 1: (6,8) },
    // but it's actually { 0: (0,8), 1: (5,8) }.
    // See how to fix.
    regcomp_extended(&regex, "(.{2,3})+", groupno, 0, '\\');
    //regcomp_extended(&regex, "((a*)b)+?", groupno, 0, '\\');
    printatoms(regex, 0);
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

#define nmatches 5
    libre_match_t match[nmatches];
    for(int i=0; i<nmatches; i++)
        match[i].q = match[i].rm_so = match[i].rm_eo = -1;

    int subret = match_atom_withq(subj, strlen(subj), 0, match, nmatches, 0, &root);
    printf("matching: \"%s\".\n", subj);
    printf("final return: %d.\n", subret);
    for(int i=0; i<nmatches; i++)
        printf("%d %td %td\n", i, match[i].rm_so, match[i].rm_eo);

    return 0;
}
