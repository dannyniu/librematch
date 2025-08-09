/* DannyNiu/NJF, 2025-06-25. Public Domain. */

#include "atoms.h"

#define eprintf(...) // dprintf(2, __VA_ARGS__)

ptrdiff_t regcomp_interval(
    int32_t *outmin, int32_t *outmax,
    const char *expr, int isbre)
{
    // 0: expecting digits for min,
    // 1: expecting digits for min or a comma,
    // 2: expecting digits for max,
    // 3: expecting digits for max or a closing brace.
    size_t tx = 0;
    int fsm = 0;
    int32_t x = 0;

    while( true )
    {
        if( '0' <= expr[tx] && expr[tx] <= '9' )
        {
            x = x * 10 + expr[tx] - '0';
            if( fsm == 0 ) fsm = 1;
            if( fsm == 2 ) fsm = 3;
        }

        else if( expr[tx] == ',' && fsm == 1 )
        {
            *outmin = x;
            fsm = 2;
            x = 0;
        }

        else if( expr[tx] == '}' && fsm == 2 && !isbre )
        {
            *outmax = -1;
            break;
        }

        else if( expr[tx] == '}' && fsm == 3 && !isbre )
        {
            *outmax = x;
            break;
        }

        else if( expr[tx] == '\\' && expr[tx+1] == '}' && fsm == 2 && isbre )
        {
            *outmax = -1;
            break;
        }

        else if( expr[tx] == '\\' && expr[tx+1] == '}' && fsm == 3 && isbre )
        {
            *outmax = x;
            break;
        }

        else
        {
            return -2 - (fsm == 3 ? 3 : 2);
        }

        ++ tx;
    }

    return tx;
}
