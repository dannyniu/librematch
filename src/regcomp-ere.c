/* DannyNiu/NJF, 2025-06-18. Public Domain. */

#ifndef eprintf
#define eprintf(...) // dprintf(2, __VA_ARGS__)
#endif // eprintf

#include "regcomp.h"

#define ADD_1ATOM do {                                  \
        tmp = realloc(atoms, sizeof(re_atom_t) * ++ta); \
        if( !tmp ) {                                    \
            ret = -1;                                   \
            goto fail; }                                \
        atoms = tmp;                                    \
        tmp = NULL;                                     \
    } while(0)

ptrdiff_t regcomp_extended(
    re_atom_t **out, const char *regexp,
    size_t *groupno, int sublevel, int escape_char)
{
    ptrdiff_t ret = 0, subret;
    re_atom_t *atoms = NULL, *tmp = NULL;
    size_t tx = 0, ta = 0;
    int i;

    while( true )
    {
        if( regexp[tx] == '(' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 3,
                .quantification = 0,
                .value = ++(*groupno),
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };

            subret = regcomp_extended(
                &atoms[ta-1].re_sub, regexp+tx+1, groupno,
                sublevel + 1, escape_char);
            if( subret < 0 )
            {
                ret = subret;
                goto fail;
            }
            else tx += subret + 2;
        }

        else if( regexp[tx] == '\0' || regexp[tx] == ')' )
        {
            if( (regexp[tx] == ')' && sublevel == 0) ||
                (regexp[tx] == '\0' && sublevel > 0) )
            {
                ret = -2; // unexpected closing parenthises.
                goto fail;
            }

            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 0,
                .quantification = 0,
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };

            ret = tx;
            *out = atoms;
            return ret;
        }

        else if( strchr("?*+", regexp[tx]) )
        {
            if( ta < 1 )
            {
                ret = -3; // quantifier syntax error.
                goto fail;
            }

            switch( regexp[tx] )
            {
            case '?':
                atoms[ta-1].rep_min = 0;
                atoms[ta-1].rep_max = 1;
                break;
            case '*':
                atoms[ta-1].rep_min = 0;
                atoms[ta-1].rep_max = -1;
                break;
            case '+':
                atoms[ta-1].rep_min = 1;
                atoms[ta-1].rep_max = -1;
                break;
            default:
                ret = -3; // quantifier syntax error.
                goto fail;
                break;
            }

            if( regexp[tx+1] == '?' )
            {
                atoms[ta-1].quantification = 1;
                tx = tx + 2;
            }
            else
            {
                atoms[ta-1].quantification = 0;
                tx = tx + 1;
            }
        }

        else if( regexp[tx] == '{' )
        {
            subret = regcomp_interval(
                &atoms[ta-1].rep_min,
                &atoms[ta-1].rep_max,
                regexp+tx+1, false);

            if( subret < 0 )
            {
                ret = subret - (
                    subret == -1 ? 0 :
                    REGCOMP_INTERVAL_ERROR_RANGE);
                goto fail;
            }
            else tx += subret + 2;
        }

        else if( regexp[tx] == '[' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 2,
                .quantification = 0,
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };

            // It relies on `regcomp_brackets` set the `bmap` member
            // to `NULL` on failure to avoid freeing non-allocated memory.
            subret = regcomp_brackets(&atoms[ta-1].bmap, regexp+tx+1);
            if( subret < 0 )
            {
                ret = subret - (
                    subret == -1 ? 0 :
                    REGCOMP_BRACKETS_ERROR_RANGE);
                goto fail;
            }
            else tx += subret + 2;
        }

        else if( regexp[tx] == '.' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 2,
                .quantification = 0,
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };

            atoms[ta-1].bmap = calloc(1, sizeof(bracket_t));
            if( !atoms[ta-1].bmap )
            {
                ret = -1;
                goto fail;
            }

            atoms[ta-1].bmap->cbits[0] = ~(uint32_t)1; // nul not matched.
            for(i=1; i<8; i++)
                atoms[ta-1].bmap->cbits[i] = ~(uint32_t)0;
            ++ tx;
        }

        else if( regexp[tx] == '|' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 6,
                .quantification = 0, // not applicable actually.
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };
            ++ tx;
        }

        else if( regexp[tx] == '^' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 4,
                .quantification = 0, // not applicable actually.
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };
            ++ tx;
        }

        else if( regexp[tx] == '$' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 5,
                .quantification = 0, // not applicable actually.
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };
            ++ tx;
        }

        else if( regexp[tx] == escape_char ) // usually <backslash>.
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 1,
                .quantification = 0,
                .value = regexp[tx+1],
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };
            tx += 2;
        }

        else
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 1,
                .quantification = 0,
                .value = regexp[tx],
                .rep_min = 1,
                .rep_max = 1,
                .expp = regexp+tx,
            };
            ++ tx;
        }
    }

fail:
    regfree_atoms(atoms);
    *out = NULL; // robustness.
    return ret;
}

void regfree_atoms(re_atom_t *re)
{
    size_t t;

    for(t=0; re[t].type != 0; t++)
    {
        switch( re[t].type ) {
        case 2: regfree_brackets(re[t].bmap); break;
        case 3: regfree_atoms(re[t].re_sub); break; }
    }

    free(re);
}
