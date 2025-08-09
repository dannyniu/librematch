/* DannyNiu/NJF, 2025-06-21. Public Domain. */

#ifndef librematch_regcomp_h
#define librematch_regcomp_h 1

#include "atoms.h"

/// @fn
/// @param out the compiled bracket expression,
/// @param expr points to one character past the initial '['.
///
/// @returns
/// positive byte offset from `expr` to the closing ']',
/// or a negative error indication.
ptrdiff_t regcomp_brackets(bracket_t **out, const char *expr);
#define REGCOMP_BRACKETS_ERROR_RANGE 16

/// @fn
/// @param be the bracket expression to free.
void regfree_brackets(bracket_t *be);

/// @fn
/// @param outmin pointer to the `int32_t` object that'll receive the
///        the minimal repetition count.
/// @param outmax pointer to the `int32_t` object that'll receive the
///        the maximum repetition cout.
/// @param expr points to one character past the opening '{'
///        (or `\{` in the case of BRE).
/// @param isbre specify whether to threat it as the interval expression of
///        an ERE (false) or BRE (true).
///
/// @returns
/// positive byte offset from `expr` to the closing '}',
/// or a negative error indication.
ptrdiff_t regcomp_interval(
    int32_t *outmin, int32_t *outmax,
    const char *expr, int isbre);
#define REGCOMP_INTERVAL_ERROR_RANGE 32

/// @fn
/// @param out pointer to an `re_atom_t` pointer that'll receive the
///        array of the atoms in the expression.
/// @param regexp the POSIX extended regular expression to compile.
/// @param groupno pointer to an `size_t` tracking the index of subexpressions.
/// @param sublevel controls whether to expect a closing parenthises
///        or a nul character to end the expression, also affects
///        error reporting.
/// @param escape_char because backslash is difficult to type in C,
///        this feature allow you to use an alternative character
///        such as '%' to substitute the escape character.
///
/// @returns
/// positive byte offset from `regexp` to the terminating nul character,
/// or the closing parenthises if `sublevel` is greater than 0;
/// or a negative value to indicate the error.
ptrdiff_t regcomp_extended(
    re_atom_t **out, const char *regexp,
    size_t *groupno, int sublevel, int escape_char);

/// @fn
/// @param re the regular expression to free.
void regfree_atoms(re_atom_t *re);

ptrdiff_t regcomp_basic(
    re_atom_t **out, const char *regexp, size_t *groupno, int sublevel);

#endif /* librematch_regcomp_h */
