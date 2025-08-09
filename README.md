The `librematch` Regular Expression Library.
====

The `librematch` regex library implements a byte-oriented ASCII-based subset of
the POSIX-2024 regular expressions specification.

Getting Started
====

The library comes with a POSIX-compliant Makefile and a typical `configure`
script. To use the library, do what most people would typically do:

```
./configure
make
make install # optional.
```

Here's a brief interface listing, they're further described in the
`librematch.h` header, which contain API that're POSIX-compatible.

- Types
  - `libregexp_t`
  - `libre_match_t := { rm_so, rm_eo, ... }`
- Functions
  - `int libregcomp(preg, pattern, cflags);`
  - `int libregexec(preg, string, nmatch, pmatch, eflags);`
  - `void libregfree(preg);`
- Compile Flags
  - `LIBREG_{EXTENDED,ICASE,MINIMAL,NOSUB,NEWLINE,...}`
- Regular Expression Execution Flags
  - `LIBREG_{NOTBOL,NOTEOL}`

***Caveats***

The interface corresponding to `regerror` had not been implemented yet,
although, it's not sure if anyone would pick up interest to use it.

Design
====

The librematch regular expression engine is byte-oriented and ASCII-based.
The reason for this is twofold:

First, UTF-8 is not the only character set out there, even if it is, a single
glyph my have multiple grapheme representations. As such the issue of
normalization comes into play.

Second, restricting to ASCII makes the behavior of both the implementation,
and application codes deterministic, the implementation can have less burdon,
and the application can rely on its pattern not being interpreted differently
depending on the language setting.

Additionally, it makes sense to restrict to the ASCII character set. POSIX
didn't specify the behavior for additional locales, as such, applications
that depend on them (being provided by the system rather than `localedef` it
by themselves) are already non-portable.

Implementation
----

The implementation make use of recursive function call. At a glance, this
may look dangerous as it carry the risk of stack overflow. However, balancing
the effort required to allocate and resize memory on the heap, as well as
potential stack frame optimization that might be performed by the compiler,
@dannyniu don't think this risk merits the hassle of inventing custom
stack allocator.

If it's ever done, there could be 2 choices:

1. Chained stack, where a failure will cause the leak of those chained
"stack frames".

2. Single relocatable stack chuck with offsets replacing pointers, which
would complicate address calculation.

And finally, if it's ever a concern, one can just create a secondary thread,
allocate a *big* stack for it, execute the regular expression in the thread
and wait for it to complete.
