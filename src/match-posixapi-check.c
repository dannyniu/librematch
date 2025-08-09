/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#include "librematch.h"

int main()
{
    libregexp_t preg;
    libre_match_t matches;
    int ret;
    libregcomp(&preg, "^abcd$", LIBREG_EXTENDED|LIBREG_ICASE);

    ret = libregexec(&preg, "ABCD", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    printf("// multi-line //\n");

    ret = libregexec(&preg, "12abcd34\nabcde", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nab-cd\nabcd", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nabcd\nfsck", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    printf("// multi-line + NOT[BE]OL //\n");

    ret = libregexec(&preg, "12abcd34\nabcde", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nab-cd\nabcd", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nabcd\nfsck", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    preg.flags |= LIBREG_NEWLINE;
    printf("// newline-flag //\n");

    ret = libregexec(&preg, "12abcd34\nabcde", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nab-cd\nabcd", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nabcd\nfsck", 1, &matches, 0);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    printf("// newline-flag + NOT[BE]OL //\n");

    ret = libregexec(&preg, "12abcd34\nabcde", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nab-cd\nabcd", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    ret = libregexec(&preg, "12abcd34\nabcd\nfsck", 1, &matches, LIBREG_NOTBOL|LIBREG_NOTEOL);
    printf("%d, %td, %td.\n", ret, matches.rm_so, matches.rm_eo);

    return 0;
}
