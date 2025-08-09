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
./regcomp-ere-check.c
./regcomp-ere.c
./regcomp-brackets.c
./regcomp-interval.c
"

arch_family=defaults
srcset="Plain C"
cflags="-fsanitize=address"
ldflags="-fsanitize=address"

tests_run
