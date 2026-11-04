/* DannyNiu/NJF, 2025-07-08. Public Domain. */

#include "librematch.h"

#include <stddef.h>
#define ctx subctx

#ifndef eprintf
#define eprintf(...) //printf(__VA_ARGS__)
#define tprintf(...)
#define dumpstate(...)
#else
#warning Debugging code depends on the original expression string not being freed!
#define tprintf(fmt, ...) do { ptrdiff_t ao = subctx.atom_offset; (void)ao; eprintf("[%td/%p]'%c'%*s" fmt, ao, subctx.atom[ao].expp, *subctx.atom[ao].expp, (int)subctx.stack_depth*3, "" __VA_OPT__(,) __VA_ARGS__); } while(false)
#define dumpstate(pre, post, ret) tprintf(pre " %d; q=%zd, offsub=%td, ret=%d, flags=%d " post, __LINE__, subctx.q, offsub, ret, flags)
#endif // eprintf

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

#define stack_overflow_mode false // new 2026-11-04.
#if stack_overflow_mode

#define unparen(...) __VA_ARGS__
#define AddrOf(operand) (&operand)
#define DeRef_T(type, operand) (*operand)

#define DECL_FUNC(name, params) int name params;

#define BEGIN_FUNC(name, params, frame, frame_init)     \
    int name params {                                   \
    struct { unparen frame } fv =                       \
    { unparen frame_init }

#define END_FUNC(name) }

#define PARAM(ident) arg_##ident,
#define LAST_PARAM(ident) arg_##ident
#define ARG(ident) arg_##ident
#define FRAME_VAR(ident) fv.ident

#define CALL(funcname, args, retdest) retdest = funcname args
#define RETURN(v) return v

#else
#define unparen(...) __VA_ARGS__
#define AddrOf(operand) (void *)((char *)&operand - stack)
#define DeRef_T(type, operand) (*(type *)((ptrdiff_t)operand + stack))

#define DECL_FUNC(name, params) struct glue(name,_params_t) { unparen params; }

#define BEGIN_FUNC(name, params, frame, frame_init)                     \
    glue(asmlabel_,name):;                                              \
    struct glue(name,_frame_t) { unparen frame };                       \
    tmp = realloc(stack, sp + sizeof(struct glue(name,_frame_t)));      \
    if( !tmp ) { free(stack); return -1; }                              \
    stack = tmp;                                                        \
    sp += sizeof(struct glue(name,_frame_t));                           \
    ((struct glue(name,_frame_t) *)(                                    \
        stack + sp - sizeof(struct glue(name,_frame_t))                 \
        ))[0] = (struct glue(name,_frame_t)){ unparen frame_init };

#define END_FUNC(name) return -1;

#define PARAM(ident) ident;
#define LAST_PARAM(ident) ident

#define ARG(ident)                                      \
    ((struct glue(funclabel,_params_t) *)(              \
        stack + sp                                      \
        - sizeof(struct glue(funclabel,_frame_t))       \
        - sizeof(struct glue(funclabel,_params_t))      \
        ))->ident

#define FRAME_VAR(ident)                                \
    ((struct glue(funclabel,_frame_t) *)(               \
        stack + sp                                      \
        - sizeof(struct glue(funclabel,_frame_t))       \
        ))->ident

#define glue(a,b) xglue(a,b)
#define xglue(a,b) a##b
#define str(x) xstr(x)
#define xstr(x) #x

#define CALL_X(funcname, args, retdest, lineno)                         \
    tmp = realloc(stack, sp + sizeof(size_t) +                          \
                  sizeof(struct glue(funcname,_params_t)));             \
    if( !tmp ) { free(stack); return -1; }                              \
    stack = tmp;                                                        \
    ((size_t *)(stack + sp))[0] = lineno;                               \
    ((struct glue(funcname,_params_t) *)(                               \
        stack + sp + sizeof(size_t)))[0] =                              \
        (struct glue(funcname,_params_t)){ unparen args };              \
    sp += sizeof(size_t) + sizeof(struct glue(funcname,_params_t));     \
    goto glue(asmlabel_,funcname);                                      \
    case lineno: retdest = ax;

#define CALL(funcname, args, retdest) CALL_X(funcname, args, retdest, glue(0x,__LINE__))

#define RETURN(v)                                       \
    ax = v;                                             \
    sp -= sizeof(size_t) +                              \
        sizeof(struct glue(funclabel,_params_t)) +      \
        sizeof(struct glue(funclabel,_frame_t));        \
    ip = *(size_t *)(stack + sp);                       \
    goto fsm_start;

#endif

// @returns
// On a better match, 2 is returned; on a match that's otherwise not better,
// 1 is returned; when not matched, 0 is returned; -1 is returned on error.

DECL_FUNC(
    try_match_next_atom,

    ( const char *PARAM(subject)
      size_t PARAM(slen)
      libre_match_t *PARAM(matches)
      size_t PARAM(nmatches)
      int PARAM(flags)
      match_ctx_t *PARAM(matchctx)
      int LAST_PARAM(ret) ) );

DECL_FUNC(
    try_increase_q,

    ( const char *PARAM(subject)
      size_t PARAM(slen)
      libre_match_t *PARAM(matches)
      size_t PARAM(nmatches)
      int PARAM(flags)
      match_ctx_t *PARAM(matchctx)
      int LAST_PARAM(ret) ) );

DECL_FUNC(
    match_atom_withq,

    ( const char *PARAM(subject)
      size_t PARAM(slen)
      ptrdiff_t PARAM(offsub)
      libre_match_t *PARAM(matches)
      size_t PARAM(nmatches)
      int PARAM(flags)
      match_ctx_t *LAST_PARAM(matchctx) ) );

#if !stack_overflow_mode
int match_atom_withq(
    const char *subject, size_t slen, ptrdiff_t offsub,
    libre_match_t *matches, size_t nmatches, int flags,
    match_ctx_t *matchctx)
{
    char *stack = NULL;
    void *tmp = NULL;
    ptrdiff_t sp = 0;
    int final_return, ax, ip = 0;

    stack = calloc(1, sizeof(match_ctx_t));
    if( !stack ) return -1;
    sp = sizeof(match_ctx_t);
    *(match_ctx_t *)stack = *matchctx;

fsm_start:
    switch( ip )
    {
    case 0:
    CALL( match_atom_withq, (
              subject, slen, offsub,
              matches, nmatches,
              flags, (void *)(stack - stack)
              ), final_return);
    free(stack);
    return final_return;
#endif // !stack_overflow_mode

// MARK: Subroutines for Next Choices.

#define funclabel try_match_next_atom
        BEGIN_FUNC(
            try_match_next_atom,

            ( const char *PARAM(subject)
              size_t PARAM(slen) //- ptrdiff_t offsub,
              libre_match_t *PARAM(matches)
              size_t PARAM(nmatches)
              int PARAM(flags)
              match_ctx_t *PARAM(matchctx)
              int LAST_PARAM(ret) ),

            ( match_ctx_t ctx;
              int flags_saved;
              int subret; ),
            ( .ctx = DeRef_T(match_ctx_t, ARG(matchctx)), .flags_saved = ARG(flags) ) );

#define subject ARG(subject)
#define slen ARG(slen)
#define offsub ARG(offsub)
#define matches ARG(matches)
#define nmatches ARG(nmatches)
#define flags ARG(flags)
#define matchctx ARG(matchctx)
#define ret ARG(ret)

#define subctx FRAME_VAR(subctx)
#define spwnctx FRAME_VAR(spwnctx)
#define flags_saved FRAME_VAR(flags_saved)
#define subret FRAME_VAR(subret)

        if( ctx.q >= (size_t)ctx.atom[ctx.atom_offset].rep_min )
        {
            if( not_shorter(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                flags = fNotABetterMatch(0);

            if( is_shorter(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                // override the decision of the overall match.
                flags = fDefiniteBetterMatch(flags);

            if( is_longer(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                // defer the decision to the overall match.
                flags = DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) >= 0 ?
                    fProbableBetterMatch(flags) :
                    fSuccessfulMatch(flags);

            tprintf("-- subsequent atom: {\n");
            ++ ctx.atom_offset;
            ctx.q = 0;
            ctx.rm_so = -1;
            ctx.rm_eo = -1;
            ctx.record = NULL;

            ++ ctx.stack_depth;
            CALL( match_atom_withq, (
                      subject, slen, DeRef_T(match_ctx_t, matchctx).rm_eo,
                      matches, nmatches, flags, AddrOf(ctx)
                      ), subret);
            -- ctx.stack_depth;
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
                DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) = DeRef_T(match_ctx_t, matchctx).rm_eo;
            }

            else if( subret == 1 )
            {
                if( DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) < 0 )
                    DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) = DeRef_T(match_ctx_t, matchctx).rm_eo;
                ret = subret;
            }

            else assert( 0 );
        }
        else tprintf("-- under-quantified, no-conti.\n");

        RETURN(ret);

#undef subject
#undef slen
#undef offsub
#undef matches
#undef nmatches
#undef flags
#undef matchctx
#undef ret

#undef subctx
#undef spwnctx
#undef flags_saved
#undef subret
        END_FUNC(try_match_next_atom);
#undef funclabel


#define funclabel try_increase_q
        BEGIN_FUNC(
            try_increase_q,

            ( const char *PARAM(subject)
              size_t PARAM(slen)
              libre_match_t *PARAM(matches)
              size_t PARAM(nmatches)
              int PARAM(flags)
              match_ctx_t *PARAM(matchctx)
              int LAST_PARAM(ret) ),

            ( match_ctx_t ctx;
              int flags_saved;
              int subret; ),
            ( .ctx = DeRef_T(match_ctx_t, ARG(matchctx)), .flags_saved = ARG(flags) ) );

#define subject ARG(subject)
#define slen ARG(slen)
#define offsub ARG(offsub)
#define matches ARG(matches)
#define nmatches ARG(nmatches)
#define flags ARG(flags)
#define matchctx ARG(matchctx)
#define ret ARG(ret)

#define subctx FRAME_VAR(subctx)
#define spwnctx FRAME_VAR(spwnctx)
#define flags_saved FRAME_VAR(flags_saved)
#define subret FRAME_VAR(subret)


        if( ctx.q <= (size_t)ctx.atom[ctx.atom_offset].rep_max )
        {
            if( ctx.q >= (size_t)ctx.atom[ctx.atom_offset].rep_min )
            {
                if( not_shorter(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                    flags = fNotABetterMatch(0);

                if( is_shorter(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                    // override the decision of the overall match.
                    flags = fDefiniteBetterMatch(flags);

                if( is_longer(&DeRef_T(match_ctx_t, matchctx), DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record), flags_saved) )
                    // defer the decision to the overall match.
                    flags = DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) >= 0 ?
                        fProbableBetterMatch(flags) :
                        fSuccessfulMatch(flags);
            }

            if( ctx.q < (size_t)ctx.atom[ctx.atom_offset].rep_max )
            {
                ctx.q ++;

                tprintf("-- repeat atom: {\n");
                ++ ctx.stack_depth;
                CALL( match_atom_withq, (
                          subject, slen, DeRef_T(match_ctx_t, matchctx).rm_eo,
                          matches, nmatches, flags, AddrOf(ctx)
                          ), subret);
                -- ctx.stack_depth;
                tprintf("}\n");
            }
            else
            {
                tprintf("-- last repeat. --\n");
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
                if( DeRef_T(match_ctx_t, matchctx).q >= (size_t)DeRef_T(match_ctx_t, matchctx).atom[
                        DeRef_T(match_ctx_t, matchctx).atom_offset].rep_min )
                {
                    DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) = DeRef_T(match_ctx_t, matchctx).rm_eo;
                }
                ret = subret;
            }
            else if( subret == 1 )
            {
                if( DeRef_T(match_ctx_t, matchctx).q >= (size_t)DeRef_T(match_ctx_t, matchctx).atom[
                        DeRef_T(match_ctx_t, matchctx).atom_offset].rep_min )
                {
                    if( DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) < 0 )
                        DeRef_T(ptrdiff_t, DeRef_T(match_ctx_t, matchctx).record) = DeRef_T(match_ctx_t, matchctx).rm_eo;
                }
                ret = subret;
            }

            else assert( 0 );
        }
        else tprintf("-- over-quantified, no-repeat.\n");

        RETURN(ret);

#undef subject
#undef slen
#undef offsub
#undef matches
#undef nmatches
#undef flags
#undef matchctx
#undef ret

#undef subctx
#undef spwnctx
#undef flags_saved
#undef subret
        END_FUNC(try_increase_q);
#undef funclabel

// see declaration above for info.
#define funclabel match_atom_withq
        BEGIN_FUNC(
            match_atom_withq,

            ( const char *PARAM(subject)
              size_t PARAM(slen)
              ptrdiff_t PARAM(offsub)
              libre_match_t *PARAM(matches)
              size_t PARAM(nmatches)
              int PARAM(flags)
              match_ctx_t *LAST_PARAM(matchctx) ),

            ( // chained contexts stack.
                match_ctx_t subctx; //-, savedctx;
                match_ctx_t spwnctx;

                // local working variables.
                ptrdiff_t record;
                short ret, subret; // 2026-11-03: needs to be `short` to avoid bus error.
                int AtomType; ),

            ( .subctx = DeRef_T(match_ctx_t, ARG(matchctx)), .record = -1, .ret = -1, .subret = -1, ) );

#define subject ARG(subject)
#define slen ARG(slen)
#define offsub ARG(offsub)
#define matches ARG(matches)
#define nmatches ARG(nmatches)
#define flags ARG(flags)
#define matchctx ARG(matchctx)

#define subctx FRAME_VAR(subctx)
#define spwnctx FRAME_VAR(spwnctx)
#define ret FRAME_VAR(ret)
#define subret FRAME_VAR(subret)
#define AtomType FRAME_VAR(AtomType)

#define atomoff subctx.atom_offset

        if( !subctx.record ) subctx.record = AddrOf(FRAME_VAR(record));

        if( !subctx.root )
        {
            subctx.root = matchctx;
        }

        dumpstate("// Reached", "//\n", ret);

// MARK: Handle End of Expression.

        if( (subctx.atom[atomoff].type == 0 ||
             subctx.atom[atomoff].type == 6) &&
            !subctx.parent )
        {
            register ptrdiff_t overall;

            if( subctx.parent )
            {
                assert( 0 );
                DeRef_T(match_ctx_t, subctx.parent).rm_eo = offsub;
                dumpstate("-- Returning", "--\n", 1);
                RETURN(1);
            }

            if( subctx.root )
                overall = DeRef_T(match_ctx_t, subctx.root).overall;
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

            // overall was (NULL), this is in fact better.
            if( ret == 1 && overall == -1 )
                ret = 2;

            if( ret == 2 )
            {
                register size_t n;
                tprintf("Finally Better, %td %td\n", overall, offsub);
                overall = subctx.root ?
                    (DeRef_T(match_ctx_t, subctx.root).overall = offsub) :
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
                register size_t n;
                for(n=1; n<nmatches; n++)
                {
                    eprintf("good: %td %td %td %td %td.\n", matches[n].rm_so, matches[n].rm_eo, matches[n].sv_so, matches[n].sv_eo, matches[n].q);
                    if( (ptrdiff_t)matches[n].q < 0 ) continue;
                    matches[n].rm_so = matches[n].sv_so;
                    matches[n].rm_eo = matches[n].sv_eo;
                }
            }

            dumpstate("-- Returning", "--\n", ret);
            RETURN(ret);
        }

        while( true )
        {
            AtomType = subctx.atom[atomoff].type;

            // MARK: Match the current atom.

            if( AtomType == 3 )
            {
                spwnctx = subctx;
                spwnctx.parent = AddrOf(subctx);
                spwnctx.atom = subctx.atom[atomoff].re_sub;
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
                    CALL( match_atom_withq, (
                              subject, slen, offsub,
                              matches, nmatches, flags, AddrOf(spwnctx)
                              ), subret);
                    if( !subret )
                    {
                        subctx.rm_so = subctx.rm_eo = -1;
                        tprintf("-- !! %d; match=(NULL) --\n", __LINE__);
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
            else if( AtomType == 7 )
            {
                if( subctx.q == 0 )
                {
                    subctx.rm_so = offsub;
                    subctx.rm_eo = offsub;
                }
                else
                {
                    register ptrdiff_t so;
                    register size_t len;
                    register int ref = subctx.atom[atomoff].value;
                    // Backreference, BRE-specific addition.
                    if( (size_t)ref >= nmatches )
                    {
                        dumpstate("-- Returning", "--\n", -1);
                        RETURN(-1);
                    }

                    so = matches[ref].br_so;
                    len = matches[ref].br_eo - matches[ref].br_so;

                    if( (size_t)offsub + len > slen )
                    {
                        // 2025-09-07:
                        // Setting to 0 would've been wrong, as it could invalidate
                        // the match of a previous alternative. But since this
                        // implementation of BRE don't support alternatives, nor
                        // does it intend to, there's no concern here.
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
                    tprintf("- BackRef ret=%d, offsub=%td, (so,len)=(%td,%zd) -\n", ret, offsub, so, len);
                    if( ret == 0 ) break;
                }
            }
            else if( AtomType == 0 || AtomType == 6 )
            {
                // 2025-09-07:
                // Consider the possibility of empty alternative(s).
                // As such, the following assertions doesn't always hold.
                //- assert( ret == -1 );
                //- assert( subctx.parent );

                if( AtomType == 6 )
                {
                    // 2025-09-07 retro note:
                    // This block may be entered by a recursive call that
                    // reaches an atom that happens to be an alternative
                    // separator (i.e. a `try_match_next_atom` call).
                    // Actions are therefore required.
                    tprintf("- AltDelim %td %td -\n", DeRef_T(match_ctx_t, subctx.parent).rm_so, DeRef_T(match_ctx_t, subctx.parent).rm_eo);
                }
                else
                {
                    tprintf("- SubExpr Ended, NextAlt. -\n");

                    // Added back 2025-09-06.
                    // This is otherwise not an error, but `ret` had nonetheless
                    // been initialized to -1, so set it to some successful match
                    // at this point.
                    // Note 2025-09-07:
                    // Subsummed to the following assignment to `ret`.
                }
                if( ret == -1 ) ret = 0;

                subret = flags == fDefiniteBetterMatch(flags) ? 2 : 1;
                subret = ret ? ret : subret;

                DeRef_T(match_ctx_t, subctx.parent).rm_eo = subctx.rm_so = subctx.rm_eo = offsub;
                subexpr_save1match(&(DeRef_T(match_ctx_t, DeRef_T(match_ctx_t, matchctx).parent)), matches, nmatches, subret);

                spwnctx = DeRef_T(match_ctx_t, subctx.parent);

                tprintf("- SubSeqExpr rm_eo=%td offsub=%td -\n", subctx.rm_eo, offsub);
                tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

                subret = ret;
                tprintf("subret(%d)==%d; q=%zd\n", __LINE__, subret, subctx.q);

                CALL( try_match_next_atom, (
                          subject, slen, matches, nmatches,
                          flags, AddrOf(spwnctx), subret
                          ), subret);

                tprintf("subret(%d)==%d; q=%zd\n", __LINE__, subret, subctx.q);

                if( (size_t)offsub <= slen &&
                    DeRef_T(match_ctx_t, subctx.parent).rm_so != DeRef_T(match_ctx_t, subctx.parent).rm_eo )
                {
                    if( (atomoff > 0 && subctx.atom[atomoff-1].type != 6) ||
                        (subctx.q <= (size_t)subctx.atom[atomoff].rep_max &&
                         subctx.q <= (size_t)subctx.atom[atomoff].rep_min) )
                    {
                        CALL( try_increase_q, (
                                  subject, slen, matches, nmatches,
                                  flags, AddrOf(spwnctx), subret
                                  ), subret);
                    }
                }

                tprintf("subret(%d)==%d; q=%zd\n", __LINE__, subret, subctx.q);
                ret = subret;

                // 2025-09-09:
                // Reached as an end of a slice of (sub-)expression.
                // Breaking handled *as part of* 'next-alternative'.
                //: if( atomoff > 0 && subctx.atom[atomoff-1].type != 6 ) break; else
                goto next_alternative;
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
                        &subctx.atom[atomoff],
                        subject, slen, offsub, flags);
                    if( !subret )
                    {
                        tprintf("-- !! %d; match=(NULL) --\n", __LINE__);
                        tprintf("-- .. slen=%zd, offsub=%td, %c, --\n", slen, offsub, subject[offsub]);
                        if( ret == -1 )
                        {
                            // 2025-09-07:
                            // The condition had been added, because
                            // this may be entered from a previous alternative.
                            ret = 0;
                        }
                        goto next_alternative;
                    }
                    else
                    {
                        tprintf("-- @@ %d; matched --\n", __LINE__);
                        tprintf("-- @@ slen=%zd, offsub=%td, %c, --\n", slen, offsub, subject[offsub]);
                        subctx.rm_so = subctx.rm_eo = offsub;
                        if( AtomType == 1 || AtomType == 2 )
                            subctx.rm_eo += 1;
                    }
                }
            }

            // MARK: Alternative Choice with Adjacent Atom.

            // it is assumed at this point that subctx.rm_* are set.
            // 2025-09-06: likewise, `ret` should also be assigned.

            //- savedctx = subctx;
            tprintf("ret(start)==%d; q=%zd\n", ret, subctx.q);

            CALL( try_match_next_atom, (
                      subject, slen, matches, nmatches,
                      flags, AddrOf(subctx), ret
                      ), ret);

            tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

            if( subctx.parent ) // save every potential good match.
            {
                if( subctx.q >= (size_t)subctx.atom[atomoff].rep_min )
                {
                    DeRef_T(match_ctx_t, subctx.parent).rm_eo = subctx.rm_eo;
                    subexpr_save1match(&(DeRef_T(match_ctx_t, DeRef_T(match_ctx_t, matchctx).parent)), matches, nmatches, ret);
                }
            }

            CALL( try_increase_q, (
                      subject, slen, matches, nmatches,
                      flags, AddrOf(subctx), ret
                      ), ret);

            tprintf("ret(%d)==%d; q=%zd\n", __LINE__, ret, subctx.q);

            if( subctx.parent ) // save every potential good match.
            {
                if( subctx.q >= (size_t)subctx.atom[atomoff].rep_min )
                {
                    DeRef_T(match_ctx_t, subctx.parent).rm_eo = subctx.rm_eo;
                    subexpr_save1match(&(DeRef_T(match_ctx_t, DeRef_T(match_ctx_t, matchctx).parent)), matches, nmatches, ret);
                }
            }

            if( subctx.q < (size_t)subctx.atom[atomoff].rep_min )
            {
                if( subctx.q > 0 ) break;
                else goto next_alternative;
            }

            // MARK: Proceeding to Next Alternative.

        next_alternative:
            if( atomoff > 0 && subctx.atom[atomoff-1].type != 6 )
                break; // not the beginning of an alternative.

            while( true )
            {
                // set to one after the beginning or the alternative delimiter,
                ++ atomoff;

                // break when the next is reached (delim or end).
                if( subctx.atom[atomoff].type == 0 ||
                    subctx.atom[atomoff].type == 6 )
                    break;

                // The order of increment and break should matter, if changing the
                // order doesn't break application, then this code is buggy.
            }

            if( subctx.atom[atomoff].type == 6 )
            {
                ++ atomoff;
                //tprintf("<<-- alternative offset.\n");
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
        RETURN(ret); // 2025-07-23: ?? supposedly ??

#undef subject
#undef slen
#undef offsub
#undef matches
#undef nmatches
#undef flags
#undef matchctx

#undef subctx
#undef spwnctx
#undef ret
#undef subret
#undef AtomType
        END_FUNC(match_atom_withq);
#undef funclabel

#if !stack_overflow_mode
    }
    free(stack);
    return -1;
}
#endif // !stack_overflow_mode

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

    eprintf("save1match %d; q_m=%td, q_x=%td, subret=%d.\n",
            __LINE__, matches[atom->value].q, ctx->q, subret);
    eprintf("save1match %d; n=%d, sv_so=%td, sv_eo=%td.\n",
            __LINE__, atom->value,
            matches[atom->value].sv_so,
            matches[atom->value].sv_eo);
    eprintf("save1match %d; n=%d, rm_so=%td, rm_eo=%td.\n",
            __LINE__, atom->value, ctx->rm_so, ctx->rm_eo);

    matches[atom->value].br_so = ctx->rm_so;
    matches[atom->value].br_eo = ctx->rm_eo;

    eprintf("save1match %d; n=%d, br_so=%td, br_eo=%td.\n",
            __LINE__, atom->value,
            matches[atom->value].br_so,
            matches[atom->value].br_eo);

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
            // 2025-09-07: Q: ??!! `<` Or `<=` ??!!
            //
            // 2025-09-07:
            // Only a deeper stack level saves a match.
            // Deeper stack levels correspond to later
            // atoms in the sequence, as the engine is
            // based on recursive descending of atoms
            // from first to last.
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

    eprintf("savedmatch %d; n=%d, sv_so=%td, sv_eo=%td, q_m=%td.\n",
            __LINE__, atom->value,
            matches[atom->value].sv_so,
            matches[atom->value].sv_eo,
            matches[atom->value].q);
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

    else if( (size_t)offsub >= slen )
        return false;

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
    preg->expb = pattern;
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

            subret = match_atom_withq(string, slen, offsub, m, n, xflags, &root);

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

                subret = match_atom_withq(string+offptr, slen, offsub, m, n, rflags, &root);

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
    regfree_atoms(preg->re_atoms);
    preg->re_atoms = NULL;
}
