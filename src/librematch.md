
To match a single atom with quantification `q`:
- for each alternatives in the atom:
  1. if `q >= rep_min`, match the subsequent sequence of atoms.
     1. if this is a better match, record it.
  2. if `q < rep_max`, then match the current atom again with `q+1`.
     1. if this is a better match, record it.
  3. if the end of the subject string is reached,
     1. if there are unmatched atoms, return match failure - i.e. {NULL}.

idea: reuse a field in the match_ctx_t structure type to record the overall
length record, the reused field will be found in `root`.

To match a sequence (a.k.a. a slice) of atoms:
1. match the first atom in the sequence with quantification 0.

Comments:
Step 2 of "match a single atom with quantification" is the self-recursion
that realizes the match of the sequences/slices.
A quantification of 0 is intentional, as it is meant to invoke the
recursion of repetitions.

To determine if a match is better:
1. if the previously recorded match is {NULL}, then this is a better match.
2. if the quantifier of the current atom is 'lazy', then:
   1. if the subject string matched by the current (repeated) atom,
      is shorter in number of characters, then it's a better match,
   2. if longer, worse,
   3. tie otherwise.
3. if the result of 2.x is tie, and the overall match, depending on
   whether `REG_MINIMAL` is unset/set:
   1. is longer/shorter, better,
   2. is shorter/longer, worse,
   3. tie otherwise.
4. if the result of 3.x is tie, the quantifier of the current atom
   is 'greedy', and the subject string matched by the current (repeated)
   atom is longer in number of characters, then it's a better match,
5. otherwise, it's not a better match.

To match the current atom once:
1. if the atom is of type 1(single character/byte), then the atom match
   if and only if the current character value match that of the atom.
2. if the atom is of type 2(bracket expression), then the atom match
   if and only if the current character is a member in the expression.
3. if the atom is of type 4(start anchor), then the atom match
   if and only if the current position is at the beginning of
   the subject string.
4. if the atom is of type 5(end anchor), then the atom match
   if and only if the current position is at the end of the subject string.
