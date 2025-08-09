/* DannyNiu/NJF, 2025-06-18. Public Domain. */

#ifndef librematch_atoms_h
#define librematch_atoms_h 1

#include "common.h"

static_assert( ' ' == 0x20 && '0' == 0x30 && 'A' == 0x41 ,
               "The librematch library is ASCII-based!" );

typedef struct re_atom re_atom_t;
typedef struct {
    uint32_t cbits[8];
} bracket_t;

struct re_atom {
    // 0: invalid / end-of-array,
    // 1: single character/byte,
    // 2: bracket expression,
    // 3: subexpression,
    // 4: start anchor,
    // 5: end anchor,
    // 6: alternatives separator,
    // 7: backreference, (BRE-specific addition).
    int16_t type;

    // 0: none / default - i.e. longest,
    // 1: minimal - i.e. shortest.
    // -- barring the `REG_MINIMAL` flag from XPG-8 --
    int16_t quantification;

    // - when `type` is 1:
    //   character value.
    int32_t value;

    // quantifier applied to the whole atom.
    //
    // regarding using 32-bit integers, it is assumed
    // whatever that needs a DFA for parsing, if it's
    // large enough, it should require a dedicated-purpose
    // optimized parser for that.
    //
    int32_t rep_min;
    int32_t rep_max;

    union {
        re_atom_t *re_sub;
        bracket_t *bmap; // for bracket expression.
    };
};

#endif /* librematch_atoms_h */
