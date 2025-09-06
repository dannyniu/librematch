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

ptrdiff_t regcomp_basic(
    re_atom_t **out, const char *regexp,
    size_t *groupno, int sublevel)
{
    ptrdiff_t ret = 0, subret;
    re_atom_t *atoms = NULL, *tmp = NULL;
    size_t tx = 0, ta = 0;
    int i;

    while( true )
    {
        if( strncmp(regexp+tx, "\\(", 2) == 0 )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 3,
                .quantification = 0,
                .value = ++(*groupno),
                .rep_min = 1,
                .rep_max = 1,
            };

            subret = regcomp_basic(
                &atoms[ta-1].re_sub, regexp+tx+2, groupno, sublevel + 1);
            if( subret < 0 )
            {
                ret = subret;
                goto fail;
            }
            else tx += subret + 4;
        }

        else if( regexp[tx] == '\0' || strncmp(regexp+tx, "\\)", 2) == 0 )
        {
            if( (strncmp(regexp+tx, "\\)", 2) == 0 && sublevel == 0) ||
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
            };

            ret = tx;
            *out = atoms;
            return ret;
        }

        else if( strchr("*", regexp[tx]) )
        {
            if( ta < 1 )
            {
                ret = -3; // quantifier syntax error.
                goto fail;
            }

            switch( regexp[tx] )
            {
            case '?':
                assert( 0 );
                atoms[ta-1].rep_min = 0;
                atoms[ta-1].rep_max = 1;
                break;
            case '*':
                atoms[ta-1].rep_min = 0;
                atoms[ta-1].rep_max = -1;
                break;
            case '+':
                assert( 0 );
                atoms[ta-1].rep_min = 1;
                atoms[ta-1].rep_max = -1;
                break;
            default:
                ret = -3; // quantifier syntax error.
                goto fail;
                break;
            }

            atoms[ta-1].quantification = 0;
            tx = tx + 1;
        }

        else if( strncmp(regexp+tx, "\\{", 2) == 0 )
        {
            subret = regcomp_interval(
                &atoms[ta-1].rep_min,
                &atoms[ta-1].rep_max,
                regexp+tx+2, true);

            if( subret < 0 )
            {
                ret = subret - (
                    subret == -1 ? 0 :
                    REGCOMP_INTERVAL_ERROR_RANGE);
                goto fail;
            }
            else tx += subret + 4;
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

        else if( regexp[tx] == '^' && tx == 0 && sublevel == 0 )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 4,
                .quantification = 0, // not applicable actually.
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
            };
            ++ tx;
        }

        else if( regexp[tx] == '$' && regexp[tx+1] == '\0' && sublevel == 0 )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 5,
                .quantification = 0, // not applicable actually.
                .value = 0,
                .rep_min = 1,
                .rep_max = 1,
            };
            ++ tx;
        }

        else if( regexp[tx] == '\\' &&
                 '0' <= regexp[tx+1] &&
                 '9' >= regexp[tx+1] )
        {
            // Backreference. Specific to BRE.
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 7,
                .quantification = 0,
                .value = regexp[tx+1] - '0',
                .rep_min = 1,
                .rep_max = 1,
            };
            tx += 2;
        }
        else if( regexp[tx] == '\\' )
        {
            ADD_1ATOM;
            atoms[ta-1] = (re_atom_t){
                .type = 1,
                .quantification = 0,
                .value = regexp[tx+1],
                .rep_min = 1,
                .rep_max = 1,
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
            };
            ++ tx;
        }
    }

fail:
    regfree_atoms(atoms);
    *out = NULL; // robustness.
    return ret;
}
