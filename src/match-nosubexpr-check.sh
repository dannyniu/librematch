#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        $exec
}

cd "$(dirname "$0")"
unitest_sh=./unitest.sh
. $unitest_sh

src="\
./match-nosubexpr-check.c
./regcomp-ere.c
./regcomp-bre.c
./regcomp-brackets.c
./regcomp-interval.c
"

arch_family=defaults
srcset="Plain C"

tests_run
