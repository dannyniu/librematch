#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        #$exec 2 $'\.?[0-9](\x27?[_0-9a-zA-Z]|[eEpP][-+]|\\.)*' 38.4e+6
        $exec 5 '(u8|[uUL])?"[^\"]*"' 'u8"Hello World!"'
}

cd "$(dirname "$0")"
unitest_sh=./unitest.sh
. $unitest_sh

src="\
./match-anysome-check.c
./librematch.c
./regcomp-ere.c
./regcomp-bre.c
./regcomp-brackets.c
./regcomp-interval.c
"

arch_family=defaults
srcset="Plain C"

tests_run
