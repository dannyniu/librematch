/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#ifndef librematch_h
#define librematch_h 1

#include "regcomp.h"

typedef struct {
    re_atom_t *re_atoms;
    size_t re_nsub;
    int flags;
} libregexp_t;

#define LIBREG_ALTEXC(c)    (c & 255)
#define LIBREG_EXTENDED     (1 <<  8)
#define LIBREG_ICASE        (1 <<  9)
#define LIBREG_MINIMAL      (1 << 10)
#define LIBREG_NOSUB        (1 << 11) // ignored by librematch.
#define LIBREG_NEWLINE      (1 << 12)

typedef struct {
    ptrdiff_t rm_so, rm_eo;
    ptrdiff_t sv_so, sv_eo;
    size_t q;
} libre_match_t;

int libregcomp(
    libregexp_t *restrict preg, const char *restrict pattern, int cflags);

#define LIBREG_BADPAT   (-2)
#define LIBREG_ECOLLATE (-2)
#define LIBREG_ECTYPE   (-2)
#define LIBREG_EESCAPE  (-2)
#define LIBREG_ESUBREG  (-2)
#define LIBREG_EBRACK   (-2)
#define LIBREG_EPAREN   (-2)
#define LIBREG_EBRACE   (-2)
#define LIBREG_BADBR    (-2)
#define LIBREG_ERANGE   (-2)
#define LIBREG_ESPACE   (-1)
#define LIBREG_BADRPT   (-2)

int libregexec(
    libregexp_t *restrict preg, const char *restrict string,
    size_t nmatch, libre_match_t pmatch[restrict], int eflags);

#define LIBREG_NOTBOL   (1 << 0)
#define LIBREG_NOTEOL   (1 << 1)
#define LIBREG_NOTMATCH (-3)

void libregfree(libregexp_t *preg);

#endif /* librematch_h */
