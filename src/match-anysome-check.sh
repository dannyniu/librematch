#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        #$exec 2 $'\.?[0-9](\x27?[_0-9a-zA-Z]|[eEpP][-+]|\\.)*' 38.4e+6
    $exec 3 '(u8|[uUL])?"([^\"]|\\[\"])*"' 'L"Hello World! \\"'
    : ::::::::::::::::::: ::::::::::::::::: 0123456789012345678
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
#cflags="-Deprintf(...)=printf(__VA_ARGS__)"

tests_run
