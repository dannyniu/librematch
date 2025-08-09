/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#ifndef librematch_h
#define librematch_h 1

#include "regcomp.h"

/// @typedef libregexp_t
/// @brief
/// The context representing the compiled regular expression.
/// The internals should not be altered, and it should be treated as opaque.
typedef struct {
    re_atom_t *re_atoms;
    size_t re_nsub;
    int flags;
} libregexp_t;

/// @def LIBREG_ALTEXC(c)
/// @brief
/// Use the character `c` instead of backslash as the escape character
/// Only applicable in ERE.
#define LIBREG_ALTEXC(c)    (c & 255)

/// @def LIBREG_EXTENDED
/// @brief Use ERE the extended regex syntax.
#define LIBREG_EXTENDED     (1 <<  8)

/// @def LIBREG_EXTENDED
/// @brief Match without regard to case (in ASCII alphabet only).
#define LIBREG_ICASE        (1 <<  9)

/// @def LIBREG_MINIMAL
/// @brief The newly introduced POSIX-2024 lazy semantic.
/// @details
/// When specified, quantifiers such as `?`, `*`, and `+` tries to match
/// the shortest possible substring; the overall match is changed to
/// match the (leftmost (as always)) shortest string as well; the lazy
/// quantifiers are altered to greedy on the other hand.
#define LIBREG_MINIMAL      (1 << 10)

/// @def LIBREG_NOSUB
/// @brief Do not try to capture subexpression to speed up matching.
/// @details Currently ignored by librematch.
#define LIBREG_NOSUB        (1 << 11)

/// @def LIBREG_NEWLINE
/// @brief Line-based matching.
#define LIBREG_NEWLINE      (1 << 12)

/// @struct libre_match_t
/// @brief Represents the match of a/an (sub-)expression.
typedef struct {
    /// @brief The matched (remembered) start and end offset.
    ptrdiff_t rm_so, rm_eo;

    /// @brief The saved offsets. Used internally for choosing between matches.
    ptrdiff_t sv_so, sv_eo;

    /// @brief The quantification position of the current capture. Internal.
    size_t q;
} libre_match_t;

/// @fn
/// @param preg The handle to the context to store the compiled regex.
/// @param pattern The regular expression to be compiled.
/// @param cflags The compilation flags. See above.
/// @returns See below
int libregcomp(
    libregexp_t *restrict preg, const char *restrict pattern, int cflags);

/// @defgroup libregcomp return values.
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

/// @fn
/// @param preg The compiled regular expression to be executed.
/// @param string The string to be matched.
/// @param nmatch Number of entries in `pmatch`.
/// @param pmatch The entries for (sub-)expression capture(s).
/// @param eflags See below.
int libregexec(
    libregexp_t *restrict preg, const char *restrict string,
    size_t nmatch, libre_match_t pmatch[restrict], int eflags);

/// @defgroup eflags Regular expression execution flags.
///
/// @def LIBREG_NOTBOL
/// @details
/// Indicates that the string is not a prefix and the start anchor should not
/// match the beginning of the string.
#define LIBREG_NOTBOL   (1 << 0)

/// @def LIBREG_NOTEOL
/// @details
/// Similar to above, but for end of string / suffix / end anchor, etc.
#define LIBREG_NOTEOL   (1 << 1)

/// @def LIBREG_NOTMATCH
/// @brief
/// The execution of the regex failed on the string because it didn't match,
/// there had been no other errors such as memory exhaustion or stack overflow.
#define LIBREG_NOTMATCH (-3)

/// @fn
/// @param preg The regular expression to free.
/// @brief Releases resources occupied by the compiled regex `preg`.
void libregfree(libregexp_t *preg);

#endif /* librematch_h */
