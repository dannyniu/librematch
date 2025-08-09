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
./match-subexpr-check.c
./regcomp-ere.c
./regcomp-brackets.c
./regcomp-interval.c
"

arch_family=+aarch64
srcset="Plain C"

tests_run
