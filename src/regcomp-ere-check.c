/* DannyNiu/NJF, 2025-07-08. Public Domain. */

#include "regcomp.h"

static const int indent_width = 4;
#define indentation (indent * indent_width)

void printatoms(re_atom_t *atom, int indent);

void print1atom(re_atom_t *atom, int indent)
{
    if( atom->type == 0 ) // invalid / end-of-array
    {
        return;
    }

    else if( atom->type == 1 ) // single character/byte
    {
        if( atom->rep_min == 1 && atom->rep_max == 1 )
        {
            printf("%c", atom->value);
        }
        else
        {
            printf("\n%*s", indentation, "");
            printf("%c x %d..%d %d", atom->value,
                   atom->rep_min, atom->rep_max, atom->quantification);
            printf("\n%*s", indentation, "");
        }
    }

    else if( atom->type == 2 ) // bracket expression
    {
        bracket_t *be = atom->bmap;
        int i;
        printf("\n%*s[", indentation, "");

        for(i=32; i<127; i++)
            if( (be->cbits[i / 32] >> (i % 32)) & 1 )
                printf("%c", i);

        printf("] x %d..%d %d",
               atom->rep_min, atom->rep_max,
               atom->quantification);
        printf("\n%*s", indentation, "");
    }

    else if( atom->type == 3 ) // subexpression
    {
        printf("( \\%d \n%*s", atom->value, indentation + indent_width, "");
        printatoms(atom->re_sub, indent+1);
        printf("\n%*s)", indentation + indent_width, "");
        printf(" x %d..%d %d",
               atom->rep_min, atom->rep_max,
               atom->quantification);
        printf("\n%*s", indentation, "");
    }

    else if( atom->type == 4 || atom->type == 5 ) // anchor
    {
        printf("\n%*s", indentation, "");
        printf("<%c>", atom->type == 4 ? '^' : '$');
        printf("\n%*s", indentation, "");
    }

    else if( atom->type == 6 ) // alternative
    {
        printf("\n%*s", indentation, "");
        printf("<%c>", '|');
        printf("\n%*s", indentation, "");
    }

    else if( atom->type == 7 ) // back-reference
    {
        printf("\n%*s", indentation, "");
        printf("\\backref:%d", atom->value);
        printf(" x %d..%d %d",
               atom->rep_min, atom->rep_max,
               atom->quantification);
        printf("\n%*s", indentation, "");
    }
}

void printatoms(re_atom_t *atoms, int indent)
{
    int i;
    for(i=0; atoms[i].type; i++)
        print1atom(atoms+i, indent);
}

#ifndef JUST_PRINT_ATOMS
int main()
{
    re_atom_t *ere;
    int groupno = 0;
    int ret = regcomp_extended(
        &ere, "a{3,5}b^(cdef|g(hij)123)+?[k-n]*$",
        &groupno, 0, '\\');

    printf("RegComp Returned: %d.\n", ret);
    printatoms(ere, 0);
    regfree_atoms(ere);
    return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
#endif /* JUST_PRINT_ATOMS */
