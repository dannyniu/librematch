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
  - LIBREG_{EXTENDED,ICASE,MINIMAL,NOSUB,NEWLINE,...}
- Regular Expression Execution Flags
  - LIBREG_{NOTBOL,NOTEOL}

*Caveats*

The interface corresponding to `regerror` had not been implemented yet,
although, it's not sure if anyone would pick up interest to use it.
