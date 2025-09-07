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
./match-bre-check.c
./regcomp-bre.c
./regcomp-ere.c
./regcomp-brackets.c
./regcomp-interval.c
"

arch_family=+aarch64
srcset="Plain C"
#cflags="-Deprintf(...)=printf(__VA_ARGS__)"

tests_run
