#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        $exec 2 $'\.?[0-9](\x27?[_0-9a-zA-Z]|[eEpP][-+]|\\.)*' 38.4e+6
    $exec 3 '(u8|[uUL])?"([^\"]|\\[\"])*"' 'u8"Hi\""'
    : ::::::::::::::::::: ::::::::::::::::: 01234567890123456789
    $exec 3 '(u8|[uUL])?"([^\"]|\\[\"])*"' 'U"abcd"'
    $exec 3 '(u8|[uUL])*"([^\"]|\\[\"])*"' '"utf32"'
    $exec 3 '/(|abc)+/' '//'
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
