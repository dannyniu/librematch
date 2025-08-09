/* DannyNiu/NJF, 2025-06-21. Public Domain. */

#include "regcomp.h"

#define eprintf(...) // dprintf(2, __VA_ARGS__)

static inline void be_set_range(bracket_t *be, int start, int end)
{
    int c;
    for(c=start; c<=end; c++)
        be->cbits[c / 32] |= (uint32_t)1 << (c % 32);
}

static inline void be_set_single(bracket_t *be, int c)
{
    be->cbits[c / 32] |= (uint32_t)1 << (c % 32);
}

ptrdiff_t regcomp_brackets(bracket_t **out, const char *expr)
{
    ptrdiff_t ret = 0;
    bracket_t *be = NULL;
    ptrdiff_t tx = 0;
    bool compl = false;
    bool ran = false; // the dash of the range expression is encountered.
    int ran1 = -1, ran2 = -1; // start and end of a range expression.

    be = calloc(1, sizeof(bracket_t));
    if( !be )
    {
        ret = -1; // -1 are host platform errors.
        goto fail;
    }

    while( expr[tx] )
    {
        if( expr[tx] == '^' && tx == 0 )
        {
            compl = true;
            ++ tx;
        }
        else if( expr[tx] == ']' && tx > (compl ? 1 : 0) )
        {
            // Here handles the case where '-' was the last
            // character and the 2nd to the last character
            // was the start of a range expression.
            if( ran ) be_set_single(be, '-');

            // Still, if there were a character that was a potential start
            // point of a range expression, add it as the final element
            // to the set represented by the bracket expresssion.
            if( 0 <= ran1 && ran1 < 256 ) be_set_single(be, ran1);

            // Closing bracket encountered.
            ret = tx;
            if( compl )
            {
                for(ran1 = 0; ran1<8; ran1++)
                    be->cbits[ran1] = ~be->cbits[ran1];
            }
            *out = be;
            return ret;
        }
        else if( expr[tx] == '[' )
        {
            if( expr[tx+1] == '=' || expr[tx+1] == '.' )
            {
                // Equivalence class (EC) or collating symbol (CS).
                // License is given by the standard to treat EC as CS
                // should there be no defined EC for any character;
                // License is given by the standard to treat as invalid
                // the non-existent collating symbols.

                if( !expr[tx+2] ||
                    expr[tx+3] != expr[tx+1] ||
                    expr[tx+4] != ']' )
                {
                    ret = -2; // invalid EC or CS.
                    goto fail;
                }

                if( ran && 0 <= ran1 && ran1 < 256 )
                {
                    // set the end point and complete the range expression.
                    ran2 = (unsigned char)expr[tx+2];
                    be_set_range(be, ran1, ran2);
                    ran2 = ran1 = -1;
                    ran = false;
                }
                else
                {
                    // add the last character into the set.
                    if( 0 <= ran1 && ran1 < 256 )
                        be_set_single(be, ran1);

                    // then overwrite the previous start point
                    // of the range expression.
                    ran1 = (unsigned char)expr[tx+2];
                }

                tx += 5;
            }
            else if( expr[tx+1] == ':' ) // character class (CC) expression.
            {
                if( ran )
                {
                    ret = -3; // end of a range shouldn't be a CC.
                    goto fail;
                }

                if( 0 <= ran1 && ran1 < 256 )
                    be_set_single(be, ran1);
                ran1 = -1;

                if( strncmp(expr+tx, "[:alnum:]", 9) == 0 )
                {
                    be_set_range(be, '0', '9');
                    be_set_range(be, 'A', 'Z');
                    be_set_range(be, 'a', 'z');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:alpha:]", 9) == 0 )
                {
                    be_set_range(be, 'A', 'Z');
                    be_set_range(be, 'a', 'z');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:blank:]", 9) == 0 )
                {
                    be_set_single(be, ' ');
                    be_set_single(be, '\t');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:cntrl:]", 9) == 0 )
                {
                    be_set_range(be, 0, 31);
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:digit:]", 9) == 0 )
                {
                    be_set_range(be, '0', '9');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:graph:]", 9) == 0 )
                {
                    be_set_range(be, 33, 126);
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:lower:]", 9) == 0 )
                {
                    be_set_range(be, 'a', 'z');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:print:]", 9) == 0 )
                {
                    be_set_range(be, 32, 126);
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:punct:]", 9) == 0 )
                {
                    be_set_single(be, '!');
                    be_set_single(be, '"');
                    be_set_single(be, '#');
                    be_set_single(be, '$');
                    be_set_single(be, '%');
                    be_set_single(be, '&');
                    be_set_single(be, '\'');
                    be_set_single(be, '(');
                    be_set_single(be, ')');
                    be_set_single(be, '*');
                    be_set_single(be, '+');
                    be_set_single(be, ',');
                    be_set_single(be, '-');
                    be_set_single(be, '.');
                    be_set_single(be, '/');
                    be_set_single(be, ':');
                    be_set_single(be, ';');
                    be_set_single(be, '<');
                    be_set_single(be, '=');
                    be_set_single(be, '>');
                    be_set_single(be, '?');
                    be_set_single(be, '@');
                    be_set_single(be, '[');
                    be_set_single(be, '\\');
                    be_set_single(be, ']');
                    be_set_single(be, '^');
                    be_set_single(be, '_');
                    be_set_single(be, '`');
                    be_set_single(be, '{');
                    be_set_single(be, '|');
                    be_set_single(be, '}');
                    be_set_single(be, '~');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:space:]", 9) == 0 )
                {
                    be_set_single(be, ' ');
                    be_set_single(be, '\f');
                    be_set_single(be, '\n');
                    be_set_single(be, '\r');
                    be_set_single(be, '\t');
                    be_set_single(be, '\v');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:upper:]", 9) == 0 )
                {
                    be_set_range(be, 'A', 'Z');
                    tx += 9;
                }

                else if( strncmp(expr+tx, "[:xdigit:]", 10) == 0 )
                {
                    be_set_range(be, '0', '9');
                    be_set_range(be, 'A', 'F');
                    be_set_range(be, 'a', 'f');
                    tx += 10;
                }

                else if( strncmp(expr+tx, "[:undef:]", 9) == 0 )
                {
                    // Implementation-specific to librematch.
                    be_set_range(be, 128, 255);
                    tx += 9;
                }

                else
                {
                    ret = -4; // unrecognized character class.
                    goto fail;
                }
            }
        }
        else
        {
            if( ran && 0 <= ran1 && ran1 < 256 )
            {
                // set the end point and complete the range expression.
                ran2 = (unsigned char)expr[tx];
                be_set_range(be, ran1, ran2);
                ran2 = ran1 = -1;
                ran = false;
            }
            else
            {
                if( 0 <= ran1 && ran1 < 256 && expr[tx] == '-' )
                    // there's already a start point,
                    ran = true;

                // it's another ordinary character.
                else if( 0 <= ran1 && ran1 < 256 )
                {
                    // add the last character into the set.
                    be_set_single(be, ran1);
                }

                if( !ran )
                {
                    // set the start point of the range expression,
                    // but do not let it be overwritten by a dash.
                    ran1 = (unsigned char)expr[tx];
                }
            }

            ++ tx;
        }
    }

    ret = -5; // The nul character was reached before a closing bracket.

fail:
    regfree_brackets(be);
    *out = NULL; // robustness.
    return ret;
}

void regfree_brackets(bracket_t *be)
{
    return free(be);
}
