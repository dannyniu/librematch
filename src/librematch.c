/* DannyNiu/NJF, 2025-07-08. Public Domain. */

#include "librematch.h"

#include <stddef.h>
#define ctx subctx
#ifndef eprintf
#define eprintf(...) //printf(__VA_ARGS__)
#endif // eprintf
#define tprintf(fmt, ...) do { eprintf("%*s" fmt, (int)subctx.stack_depth*3, "" __VA_OPT__(,) __VA_ARGS__); } while(false)
#define dumpstate(pre, post, ret) tprintf(pre " %d; %p:%td, q=%zd, offsub=%td, info=%d " post, __LINE__, subctx.atom, subctx.atom_offset, subctx.q, offsub, ret)

// 2025-07-13, Idea(s) of matching algorithm:
// 2025-07-20: moved to "librematch.md".

typedef struct match_ctx match_ctx_t;

struct match_ctx {
    match_ctx_t   *parent, *root;

    re_atom_t   *atom;
    ptrdiff_t   atom_offset;

    // the current value of the parameter `q`,
    // i.e. the "quantification" to match a single atom with.
    size_t  q;

    ptrdiff_t   stack_depth;

    // these 4 are in the same domain,
    // i.e. offset into subject string.
    ptrdiff_t   overall, rm_so, rm_eo, *record;
};

// @returns
// Better: 1; not better: 0.
// Verifies the qualification of the quantifier.
int not_shorter(match_ctx_t *pctx, ptrdiff_t record, int flags);
int is_shorter(match_ctx_t *pctx, ptrdiff_t record, int flags);
int is_longer(match_ctx_t *pctx, ptrdiff_t record, int flags);

// @returns
// Match: true, false otherwise.
int match_atom_scalar(
    const re_atom_t *atom, const char *subject,
    size_t slen, ptrdiff_t offsub, int flags);

static inline int fDefiniteBetterMatch(int flags){ return flags | (1 << 0); }
static inline int fProbableBetterMatch(int flags){ return flags | (1 << 1); }
static inline int fNotABetterMatch    (int flags){ return flags | (1 << 2); }
static inline int fSuccessfulMatch    (int flags){ return flags | (1 << 3); }
static inline int fRegMinimal         (int flags){ return flags | (1 << 4); }
static inline int fRegICase           (int flags){ return flags | (1 << 5); }
static inline int fRegNotBOL          (int flags){ return flags | (1 << 6); }
static inline int fRegNotEOL          (int flags){ return flags | (1 << 7); }
static inline int fRegBRE             (int flags){ return flags | (1 << 8); }

// If the atom at the current `atom_offset` is a subexpression,
// then save its matched range if it hasn't already.
void subexpr_save1match(
    match_ctx_t *ctx, libre_match_t *matches, size_t nmatches, int ret);

// @returns
// On a better match, 2 is returned; on a match that's otherwise not better,
// 1 is returned; when not matched, 0 is returned; -1 is returned on error.
int match_atom_withq(
    const char *subject, size_t slen, ptrdiff_t offsub,
    libre_match_t *matches, size_t nmatches, int flags,
    match_ctx_t *matchctx);

int try_match_next_atom(
    const char *subject, size_t slen, ptrdiff_t offsub,
    libre_match_t *matches, size_t nmatches, int flags,
    match_ctx_t *matchctx, int ret)
{
    match_ctx_t ctx = *matchctx;
    int flags_saved = flags;
    int subret;
    (void)offsub;

    if( ctx.q >= (size_t)ctx.atom[ctx.atom_offset].rep_min )
    {
        if( not_shorter(matchctx, *matchctx->record, flags_saved) )
            flags = fNotABetterMatch(0);

        if( is_shorter(matchctx, *matchctx->record, flags_saved) )
            // override the decision of the overall match.
            flags = fDefiniteBetterMatch(flags);

        if( is_longer(matchctx, *matchctx->record, flags_saved) )
            // defer the decision to the overall match.
            flags = *matchctx->record >= 0 ?
                fProbableBetterMatch(flags) :
                fSuccessfulMatch(flags);

        tprintf("-- subsequent atom: {\n");
        ++ ctx.atom_offset;
        ctx.q = 0;
        ctx.rm_so = -1;
        ctx.rm_eo = -1;
        ctx.record = NULL;

        ++ ctx.stack_depth;
        subret = match_atom_withq(
            subject, slen, matchctx->rm_eo,
            matches, nmatches, flags, &ctx);
        -- ctx.atom_offset;
        tprintf("}\n");

        if( subret == -1 ) ret = subret; // TODO: handle errors.
        else if( subret == 0 )
        {
            if( ret > 0 );
            // 2025-07-22: a nop is probably ok.
            else ret = subret;
        }
        else if( subret == 2 )
        {
            ret = subret;
            *matchctx->record = matchctx->rm_eo;
        }

        else if( subret == 1 )
        {
            if( *matchctx->record < 0 )
                *matchctx->record = matchctx->rm_eo;
            ret = subret;
        }

        else assert( 0 );
    }

    return ret;
}

int try_increase_q(
    const char *subject, size_t slen, ptrdiff_t offsub,
    libre_match_t *matches, size_t nmatches, int flags,
    match_ctx_t *matchctx, int ret)
{
    match_ctx_t ctx = *matchctx;
    int flags_saved = flags;
    int subret;
    (void)offsub;

    if( ctx.q <= (size_t)ctx.atom[ctx.atom_offset].rep_max )
    {
        if( ctx.q >= (size_t)ctx.atom[ctx.atom_offset].rep_min )
        {
            if( not_shorter(matchctx, *matchctx->record, flags_saved) )
                flags = fNotABetterMatch(0);

            if( is_shorter(matchctx, *matchctx->record, flags_saved) )
                // override the decision of the overall match.
                flags = fDefiniteBetterMatch(flags);

            if( is_longer(matchctx, *matchctx->record, flags_saved) )
                // defer the decision to the overall match.
                flags = *matchctx->record >= 0 ?
                    fProbableBetterMatch(flags) :
                    fSuccessfulMatch(flags);
        }

        if( ctx.q < (size_t)ctx.atom[ctx.atom_offset].rep_max )
        {
            ctx.q ++;

            tprintf("-- repeat atom: {\n");
            ++ ctx.stack_depth;
            subret = match_atom_withq(
                subject, slen, matchctx->rm_eo,
                matches, nmatches, flags, &ctx);
            -- ctx.stack_depth;
            tprintf("}\n");
        }
        else
        {
            tprintf("-- repeat end --\n");
            subret = 0;
        }

        if( subret == -1 ) ret = subret; // TODO: handle errors.
        else if( subret == 0 )
        {
            if( ret > 0 );
            // 2025-07-22: a nop is probably ok.
            else ret = subret;
        }

        else if( subret == 2 )
        {
            if( matchctx->q >= (size_t)matchctx->atom[
                    matchctx->atom_offset].rep_min )
            {
                *matchctx->record = matchctx->rm_eo;
            }
            ret = subret;
        }
        else if( subret == 1 )
        {
            if( matchctx->q >= (size_t)matchctx->atom[
                    matchctx->atom_offset].rep_min )
            {
                if( *matchctx->record < 0 )
                    *matchctx->record = matchctx->rm_eo;
            }
            ret = subret;
        }

        else assert( 0 );
    }

    return ret;
}

// see declaration above for info.
int match_atom_withq(
    const char *subject, size_t slen, ptrdiff_t offsub,
    libre_match_t *matches, size_t nmatches, int flags,
    match_ctx_t *matchctx)
{
    // chained contexts stack.
    match_ctx_t subctx = *matchctx; //, savedctx;

    // local working variables.
    ptrdiff_t record = -1;
    short ret = -1, subret = -1;
    short unless_match;

    if( !subctx.record ) subctx.record = &record;

    if( !subctx.root )
    {
        subctx.root = matchctx;
    }

    dumpstate("// Reached", "//\n", flags);

    if( (subctx.atom[subctx.atom_offset].type == 0 ||
         subctx.atom[subctx.atom_offset].type == 6) &&
        !subctx.parent )
    {
        ptrdiff_t overall;

        if( subctx.parent )
        {
            assert( 0 );
            subctx.parent->rm_eo = offsub;
            dumpstate("-- Returning", "--\n", 1);
            return 1;
        }

        if( subctx.root )
            overall = subctx.root->overall;
        else overall = subctx.overall;

        ret = -1; // shouldn't happen.

        // leftwards shorter match.
        if( flags == fDefiniteBetterMatch(flags) )
        {
            tprintf("Definite Better Match.\n");
            ret = 2;
        }

        else if( flags == fNotABetterMatch(flags) )
        {
            tprintf("Not a Better Match.\n");
            ret = 1;
        }

        // greedy match.
        else if( flags != fRegMinimal(flags) && offsub > overall )
        {
            tprintf(".. reached %d %d %td ..\n", __LINE__, flags, overall);
            ret = 2;
        }
        else if( flags != fRegMinimal(flags) && offsub < overall )
        {
            tprintf(".. reached %d %d %td ..\n", __LINE__, flags, overall);
            ret = 1;
        }

        // minimal match.
        else if( flags == fRegMinimal(flags) && offsub < overall )
        {
            tprintf(".. reached %d %d %td ..\n", __LINE__, flags, overall);
            ret = 2;
        }
        else if( flags == fRegMinimal(flags) && offsub > overall )
        {
            tprintf(".. reached %d %d %td ..\n", __LINE__, flags, overall);
            ret = 1;
        }

        // leftwards longer match.
        else if( flags == fProbableBetterMatch(flags) )
        {
            tprintf("Probable Better Match.\n");
            ret = 2;
        }

        else if( flags == fSuccessfulMatch(flags) )
        {
            tprintf("Successful Match.\n");
            ret = 1;
        }

        // not a better match.
        else ret = 1;

        // overall was {NULL}, this is in fact better.
        if( ret == 1 && overall == -1 )
            ret = 2;

        if( ret == 2 )
        {
            size_t n;
            tprintf("Finally Better, %td %td\n", overall, offsub);
            overall = subctx.root ?
                (subctx.root->overall = offsub) :
                (subctx.overall = offsub);
            if( nmatches > 0 )
            {
                assert( matches );
                matches[0].rm_eo = overall;
            }
            for(n=1; n<nmatches; n++)
            {
                eprintf("best: %td %td %td %td %td.\n", matches[n].rm_so, matches[n].rm_eo, matches[n].sv_so, matches[n].sv_eo, matches[n].q);
                if( (ptrdiff_t)matches[n].q < 0 ) continue;
                matches[n].rm_so = matches[n].sv_so;
                matches[n].rm_eo = matches[n].sv_eo;
            }
        }
        else if( ret == 1 )
        {
            size_t n;
            for(n=1; n<nmatches; n++)
            {
                eprintf("good: %td %td %td %td %td.\n", matches[n].rm_so, matches[n].rm_eo, matches[n].sv_so, matches[n].sv_eo, matches[n].q);
                if( (ptrdiff_t)matches[n].q < 0 ) continue;
                matches[n].rm_so = matches[n].sv_so;
                matches[n].rm_eo = matches[n].sv_eo;
            }
        }

        dumpstate("-- Returning", "--\n", ret);
        return ret;
    }

    while( true )
    {
        match_ctx_t spwnctx;
        unless_match = 0;
        if( subctx.atom[subctx.atom_offset].type == 3 )
        {
            spwnctx = subctx;
            spwnctx.parent = &subctx;
            spwnctx.atom = subctx.atom[subctx.atom_offset].re_sub;
            spwnctx.atom_offset = 0;
            spwnctx.q = 0;
            spwnctx.rm_so = offsub;
            spwnctx.rm_eo = offsub;
            spwnctx.record = NULL;
            tprintf("- SubExpr with Q=%zd. -\n", subctx.q);
            if( subctx.q == 0 )
            {
                // Initialize offset anchors for actually
                // quantified subexpression matching.
                subctx.rm_so = offsub;
                subctx.rm_eo = offsub;
            }
            else
            {
                ret = 0;
                subctx.rm_so = offsub;
                subret = match_atom_withq(
                    subject, slen, offsub,
                    matches, nmatches, flags, &spwnctx);
                if( !subret )
                {
                    subctx.rm_so = subctx.rm_eo = -1;
                    tprintf("-- !! %d; %p:%td, match={NULL} --\n", __LINE__, subctx.atom, subctx.atom_offset);
                }
                else
                {
                    subctx.rm_eo = spwnctx.rm_eo;
                    ret = subret;
                }
                tprintf("- NextAlt -\n");
                goto next_alternative;
            }
        }
        else if( subctx.atom[subctx.atom_offset].type == 7 )
        {
            if( subctx.q == 0 )
            {
                subctx.rm_so = offsub;
                subctx.rm_eo = offsub;
            }
            else
            {
                ptrdiff_t so;
                size_t len;
                int ref = subctx.atom[subctx.atom_offset].value;
                // Backreference, BRE-specific addition.
                if( (size_t)ref >= nmatches )
                {
                    dumpstate("-- Returning", "--\n", -1);
                    return -1;
                }

                so = matches[ref].br_so;
                len = matches[ref].br_eo - matches[ref].br_so;

                if( (size_t)offsub + len > slen )
                {
                    ret = 0;
                }
                else if( strncmp(subject+offsub, subject+so, len) != 0 )
                {
                    ret = 0;
                }
                else
                {
                    subctx.rm_so = offsub;
                    subctx.rm_eo = offsub + len;
                    tprintf("- bre; so=%td, eo=%td.\n", subctx.rm_so, subctx.rm_eo);
                    ret = 1;
                }
                tprintf("- BackRef ret=%d, %td, %td, %zd -\n", ret, offsub, so, len);
                if( ret == 0 ) break;
            }
        }
        else if( subctx.atom[subctx.atom_offset].type == 0 )
        {
            tprintf("- SubExpr Ended, NextAlt. -\n");

            // Added back 2025-09-06.
            // This is otherwise not an error, but `ret` had nonetheless
            // been initialized to -1, so set it to 0 at this point.
            ret = 0; 

            subctx.rm_so = subctx.rm_eo = offsub;
            goto next_alternative;
        }
        else if( subctx.atom[subctx.atom_offset].type == 6 )
        {
            tprintf("- AltDelim %td %td -\n", subctx.parent->rm_so, subctx.parent->rm_eo);
            ret = ret > 0 ? ret : flags == fDefiniteBetterMatch(flags) ? 2 : 1;
            subctx.parent->rm_eo = subctx.rm_so = subctx.rm_eo = offsub;
            subexpr_save1match(matchctx->parent, matches, nmatches, ret);
            goto subsequent_expr;
        }
        else
        {
            if( subctx.q == 0 )
            {
                subctx.rm_so = offsub;
                subctx.rm_eo = offsub;
            }
            else
            {
                subret = match_atom_scalar(
                    &subctx.atom[subctx.atom_offset],
                    subject, slen, offsub, flags);
                if( !subret )
                {
                    tprintf("-- !! %d; %p:%td, match={NULL} --\n", __LINE__, subctx.atom, subctx.atom_offset);
                    tprintf("-- .. slen=%zd, offsub=%td, %c, --\n", slen, offsub, subject[offsub]);
                    ret = 0;
                    goto next_alternative;
                }
                else
                {
                    tprintf("-- @@ %d; %p:%td, matched --\n", __LINE__, subctx.atom, subctx.atom_offset);
                    tprintf("-- @@ slen=%zd, offsub=%td, %c, --\n", slen, offsub, subject[offsub]);
                    subctx.rm_so = subctx.rm_eo = offsub;
                    if( subctx.atom[subctx.atom_offset].type == 1 ||
                        subctx.atom[subctx.atom_offset].type == 2 )
                        subctx.rm_eo += 1;
                }
            }
        }

        // it is assumed at this point that subctx.rm_* are set.
        // 2025-09-06: likewise, `ret` should also be assigned.

        // savedctx = subctx;
        unless_match = 1;
        tprintf("ret(start)==%d; q=%zd\n", ret, subctx.q);

        ret = try_match_next_atom(
            subject, slen, offsub, matches, nmatches,
            flags, &subctx, ret);

        tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

        if( subctx.parent ) // save every potential good match.
        {
            if( subctx.q >= (size_t)subctx.atom[
                    subctx.atom_offset].rep_min )
            {
                subctx.parent->rm_eo = subctx.rm_eo;
                subexpr_save1match(matchctx->parent, matches, nmatches, ret);
            }
        }

        ret = try_increase_q(
            subject, slen, offsub, matches, nmatches,
            flags, &subctx, ret);

        tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

        if( subctx.parent ) // save every potential good match.
        {
            if( subctx.q >= (size_t)subctx.atom[
                    subctx.atom_offset].rep_min )
            {
                subctx.parent->rm_eo = subctx.rm_eo;
                subexpr_save1match(matchctx->parent, matches, nmatches, ret);
            }
        }

        if( subctx.q < (size_t)subctx.atom[
                subctx.atom_offset].rep_min )
            break;

    next_alternative:
        if( subctx.atom_offset > 0 &&
            subctx.atom[subctx.atom_offset-1].type != 6 )
            break; // not the beginning of an alternative.

        while( true )
        {
            // set to one after the beginning or the alternative delimiter,
            ++  subctx.atom_offset;

            // break when the next is reached (delim or end).
            if( subctx.atom[subctx.atom_offset].type == 0 ||
                subctx.atom[subctx.atom_offset].type == 6 )
                break;

            // The order of increment and break should matter, if changing the
            // order doesn't break application, then this code is buggy.
        }

        if( !subctx.parent )
        {
            if( subctx.atom[subctx.atom_offset].type == 6 )
            {
                subctx.atom_offset ++;
                //tprintf("alternative offset %td.\n", subctx.atom_offset);
                continue;
            }
            else break; // no more alternatives.
        }
        // else we're in a subexpression.

        tprintf("ret(%d)==%d; NextAlt (%p:%td) in Parent (%p:%td).\n", __LINE__, ret, &subctx, subctx.atom_offset, subctx.parent, subctx.parent->atom_offset);
        // 2025-08-20: reworked this termination condition.
        if( subctx.parent->rm_so == subctx.parent->rm_eo )
        {
            // 2025-08-20
            // [Interpretives for the recursion termination condition]:
            // To avoid infinite recursion, exclude zero-length matches,
            // as they may confuse optional quantifications
            // and alternative steppers.
            if( subctx.atom[subctx.atom_offset].type == 0 )
                break;
        }

        // This label can preceed or follow the above condition block,
        // it won't make difference, as the only jump source required
        // the atom to be of type 6 (alternative separator) to enter.
    subsequent_expr:
        spwnctx = *subctx.parent;

        tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

        if( unless_match || ret ||
            flags == fRegBRE(flags) || // BRE has back-ref, but no alt-sep.
            spwnctx.atom[spwnctx.atom_offset+1].type == 0 ||
            spwnctx.atom[spwnctx.atom_offset+1].type == 6 )
        {
            ret = try_match_next_atom(
                subject, slen, offsub, matches, nmatches,
                flags, &spwnctx, ret);

            tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

            if( (size_t)offsub <= slen &&
                subctx.parent->rm_so != subctx.parent->rm_eo )
            {
                ret = try_increase_q(
                    subject, slen, offsub, matches, nmatches,
                    flags, &spwnctx, ret);
            }

            tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);
        }

        if( subctx.atom[subctx.atom_offset].type == 6 )
        {
            subctx.atom_offset ++;
            //tprintf("alternative offset %td.\n", subctx.atom_offset);
            continue;
        }
        else break; // no more alternatives.
    }

    tprintf("ret(%d)==%d\n", __LINE__, ret);

    dumpstate("-- Returning", "--\n", ret);
    if( nmatches > 0 && ret > 0 )
    {
        assert( matches );
        matches[0].sv_so = offsub;
        matches[0].rm_so = offsub;
    }
    return ret; // 2025-07-23: ?? supposedly ??
}

#undef ctx

void subexpr_save1match(
    match_ctx_t *ctx, libre_match_t *matches, size_t nmatches, int subret)
{
#define subctx (*ctx)
    re_atom_t *atom = ctx->atom + ctx->atom_offset;
    if( atom->type != 3 || (size_t)atom->value >= nmatches )
    {
        tprintf("unsavedmatch %d; %d %d.\n", __LINE__, atom->type, atom->value);
        return;
    }

    //*
    eprintf("save1match %d; q_m=%td, q_x=%td, subret=%d.\n",
            __LINE__, matches[atom->value].q, ctx->q, subret);
    eprintf("save1match %d; n=%d, sv_so=%td, sv_eo=%td.\n",
            __LINE__, atom->value,
            matches[atom->value].sv_so,
            matches[atom->value].sv_eo);
    eprintf("save1match %d; n=%d, rm_so=%td, rm_eo=%td.\n",
            __LINE__, atom->value, ctx->rm_so, ctx->rm_eo);//*/

    matches[atom->value].br_so = ctx->rm_so;
    matches[atom->value].br_eo = ctx->rm_eo;

    while( true )
    {
        if( subret <= 0 ) return;
        if( (ptrdiff_t)matches[atom->value].q < (ptrdiff_t)ctx->q )
        {
            matches[atom->value].depth = 0;
            break;
        }
        if( matches[atom->value].q == ctx->q && subret == 2 )
        {
            if( matches[atom->value].depth < ctx->stack_depth )
            {
                matches[atom->value].depth = ctx->stack_depth;
                break;
            }
        }
        return;
    }

    matches[atom->value].sv_so = ctx->rm_so;
    matches[atom->value].sv_eo = ctx->rm_eo;
    matches[atom->value].q = ctx->q;
}

int not_shorter(match_ctx_t *pctx, ptrdiff_t record, int flags)
{
    if( pctx->rm_eo < 0 )
        return 0; // this is instead a "better" match.

    if( (flags != fRegMinimal(flags)) == // flag negation.
        (pctx->atom[pctx->atom_offset].quantification == 0) ) // greedy
        return 0; // not the relevant condition.

    if( pctx->rm_eo < record && record >= 0 ) // not a longer match
        return 0; // this is instead a "better" match.

    return 1;
}

int is_shorter(match_ctx_t *pctx, ptrdiff_t record, int flags)
{
    if( pctx->rm_eo < 0 )
        return 0; // this is a worse match.

    if( (flags != fRegMinimal(flags)) == // flag negation.
        (pctx->atom[pctx->atom_offset].quantification == 0) ) // greedy
        return 0; // not the relevant condition.

    if( pctx->rm_eo >= record && record >= 0 ) // a longer match
        return 0; // this is a worse match.

    // this is a better match.

    return 1;
}

int is_longer(match_ctx_t *pctx, ptrdiff_t record, int flags)
{
    if( pctx->rm_eo < 0 )
        return 0; // this is a worse match.

    if( (flags != fRegMinimal(flags)) != // flag negation.
        (pctx->atom[pctx->atom_offset].quantification == 0) ) // greedy
        return 0; // not the relevant condition.

    if( pctx->rm_eo <= record && record >= 0 ) // a shorter match
        return 0; // this is a worse match.

    // this is a better match.

    return 1;
}

int match_atom_scalar(
    const re_atom_t *atom, const char *subject,
    size_t slen, ptrdiff_t offsub, int flags)
{
    int t = (uint8_t)subject[offsub];

    /*
      tprintf("%p ", atom);
      tprintf("%c(%02x) @ %td, ", t, t, offsub);
      tprintf("q=%d ", atom->quantification);
      tprintf("t=%d c=%c(%02x) \n", atom->type, atom->value, atom->value);
    //*/

    if( (size_t)offsub > slen ) return false;

    if( atom->type == 4 && flags != fRegNotBOL(flags) )
        return offsub == 0;

    else if( atom->type == 5 && flags != fRegNotEOL(flags)  )
        return (size_t)offsub == (size_t)slen;

    else if( flags != fRegICase(flags) )
    {
        if( atom->type == 1 )
            return atom->value == t;

        else if( atom->type == 2 )
            return ((atom->bmap->cbits[t/32] >> (t % 32)) & 1) != 0;

        else return false;
    }

    else if( flags == fRegICase(flags) )
    {
        int u = t, l = t;
        if( 'a' <= u && u <= 'z' ) u -= 32;
        if( 'A' <= l && l <= 'Z' ) l += 32;

        if( atom->type == 1 )
            return atom->value == u || atom->value == l;

        else if( atom->type == 2 )
            return
                ((atom->bmap->cbits[u/32] >> (u % 32)) & 1) ||
                ((atom->bmap->cbits[l/32] >> (l % 32)) & 1);

        else return false;
    }

    else return false;
}

// == MARK: POSIX-compatible API Wrapper == //

int libregcomp(libregexp_t *preg, const char *pattern, int cflags)
{
    int escc = '\\';
    ptrdiff_t subret;
    preg->re_nsub = 0;
    if( LIBREG_ALTEXC(cflags) ) escc = LIBREG_ALTEXC(cflags);

    if( cflags & LIBREG_EXTENDED )
    {
        preg->flags = cflags;
        subret = regcomp_extended(
            &preg->re_atoms, pattern,
            &preg->re_nsub, 0, escc);
        if( subret < 0 ) return subret == -1 ? -1 : -2;
        else return 0;
    }
    else
    {
        preg->flags = cflags;
        subret = regcomp_basic(
            &preg->re_atoms, pattern,
            &preg->re_nsub, 0);
        if( subret < 0 ) return subret == -1 ? -1 : -2;
        else return 0;
    }
}

int libregexec(
    libregexp_t *restrict preg, const char *restrict string,
    size_t nmatch, libre_match_t pmatch[restrict], int eflags)
{
    int xflags = 0, subret = 0;
    libre_match_t tmatches[10], *m;
    size_t n, slen, t;
    ptrdiff_t offsub;

    if( preg->flags & LIBREG_ICASE ) xflags = fRegICase(xflags);
    if( preg->flags & LIBREG_MINIMAL ) xflags = fRegMinimal(xflags);
    if( !(preg->flags & LIBREG_EXTENDED) ) xflags = fRegBRE(xflags);
    if( eflags & LIBREG_NOTBOL ) xflags = fRegNotBOL(xflags);
    if( eflags & LIBREG_NOTEOL ) xflags = fRegNotEOL(xflags);

    if( nmatch < 10 )
    {
        m = tmatches;
        n = 10;
    }
    else
    {
        m = pmatch;
        n = nmatch;
    }

    for(t=0; t<nmatch; t++)
    {
        // 2025-08-21:
        // q is unsigned, it should come last to be "strictly" portable.
        m[t].q = m[t].rm_so = m[t].rm_eo = -1;
    }

    if( (preg->flags ^ LIBREG_NEWLINE) & LIBREG_NEWLINE )
    {
        slen = strlen(string);
        for(offsub = 0; (size_t)offsub < slen; offsub++)
        {
            match_ctx_t root;
            root.parent = NULL;
            root.root = NULL;
            root.atom = preg->re_atoms;
            root.atom_offset = 0;
            root.overall = root.rm_so = root.rm_eo = -1;
            root.record = NULL;
            root.q = 0;
            root.stack_depth = 0;

            subret = match_atom_withq(
                string, slen, offsub, m, n, xflags, &root);

            if( subret > 0 )
            {
                for(n=0; n<nmatch; n++) pmatch[n] = m[n];
                return 0;
            }
            if( subret < 0 ) return subret;
            
            for(n=0; n<nmatch; n++)
                m[n].rm_so = m[n].rm_eo =
                    pmatch[n].rm_so =
                    pmatch[n].rm_eo = -1;
        }
    }
    else // line-based.
    {
        size_t offptr = 0;
        int rflags;

        while( true )
        {
            rflags = xflags;
            slen = strcspn(string+offptr, "\n");
            if( string[offptr+slen] == '\n' ) rflags &= ~fRegNotEOL(0);
            if( offptr > 0 ) rflags &= ~fRegNotBOL(0);

            for(offsub = 0; (size_t)offsub < slen; offsub++)
            {
                match_ctx_t root;
                root.parent = NULL;
                root.root = NULL;
                root.atom = preg->re_atoms;
                root.atom_offset = 0;
                root.overall = root.rm_so = root.rm_eo = -1;
                root.record = NULL;
                root.q = 0;
                root.stack_depth = 0;

                subret = match_atom_withq(
                    string+offptr, slen, offsub, m, n, rflags, &root);

                if( subret > 0 )
                {
                    for(n=0; n<nmatch; n++)
                    {
                        pmatch[n] = m[n];
                        pmatch[n].rm_so += offptr;
                        pmatch[n].rm_eo += offptr;
                    }
                    return 0;
                }
                if( subret < 0 ) return subret;

                for(n=0; n<nmatch; n++)
                    m[n].rm_so = m[n].rm_eo =
                        pmatch[n].rm_so =
                        pmatch[n].rm_eo = -1;
            }

            offptr += offsub;
            if( string[offptr] == '\0' ) break;
            else offptr += 1;
        }
    }

    return LIBREG_NOTMATCH;
}

void libregfree(libregexp_t *preg)
{
    return regfree_atoms(preg->re_atoms);
}
