A case of bug fixing on 2025-08-20.
====

During real-world testing of librematch in the dcc expression parsing project,
it was discovered that the regex `\.?[0-9]('?[_0-9a-zA-Z]|[eEpP][-+]|\.)*`
is failing to match fraction numbers. To investigate, the testcase "38.4e+6"
was introduced to trace the behavior of the matcher.

Additionally, it was discovered, the 1st subexpression of the regex `(.{2,3})+`
incorrectly captured the last 3 instead of 2 characters of
the string `weeknigh`.

Several deficiencies were identified.

First, the handling of alternative separator wasn't rigorous. Second, the
conditions for saving captures were underspecified and allowed for too much
false positives and false negatives.

The fix for captures
====

The captures are first saved to `sv_so` and `sv_eo`, and the best candidate
were moved to `rm_so` and `rm_eo` when the end of the entire regex is reached.

The quantification were consulted during saving, but not during moving - this
resulted in garbled output upon return from public API in some situations.
With the fix, the saved values won't be moved unless the quantifications are
valid, which *should imply* the saved capture is valid.

When saving the captures, the successfulness of the overall match wasn't
considered. This *was* on purpose, as BRE back reference depended on it.
With the fix, the quantifications are first compared - if the new capture has
higher quantification, then it's saved; otherwise, if the overall match is
better, it's also saved. However, states at earlier positions in the subject
string would also receive indication that the match is better, so a stack depth
variable is introduced, and only when the stack depth is deeper would a better
match be saved. Because quantification has higher precedence over betterness
when saving matches, the remembered stack depth in the match capture structure
is reset whenever the quantification "improves".

The captures for BRE back references are now saved in separate variables.

The fix for handling alternatives
====

When a zero-length match occurs, if the current subexpression is repeated, then
infinite recursion will occur - this is way the code must proceed to the next
alternative (or to the next atom in the parent expression) in this case.
While this should be obvious on itself, how to do it in practice isn't. The
previous buggy code proceeded directly to the next atom in the parent
expression without considering further alternatives - this is the root cause
of why the regex matching fraction numbers is failing.

Handling of alternatives were improved. Namely, over the following aspects:

1. the breaking of the loop were limited to only end of (sub-)expression,
   previously, any zero-length match breaks it.

2. the alternative delimiter is explicitly handled at the beginning of the loop
   body as a "valid" atom that can be encountered in the stream - it saves the
   current candidate match of the subexpression to `sv_so` and `sv_eo`, and
   indicates a success match by setting the `ret` working variable.

Request for Feedback.
====

Librematch is a brand new library, and is certainly immature in a lot of ways.
I encourage people to use this library, even for just for cross checking their
regular expressions on various POSIX regex implementations, and even just for
the dull task of verifying that they wrote the expressions correct.
