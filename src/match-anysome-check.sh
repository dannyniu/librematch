#!/bin/sh

optimize=debug
testfunc()
{
    regex_strlit='(u8|[uUL])?"([^\"]|\\[\"])*"'
    $exec 2 $'\.?[0-9](\x27?[_0-9a-zA-Z]|[eEpP][-+]|\\.)*' 38.4e+6
    $exec 3 "$regex_strlit" 'u8"Hi\""'
    : :::::::::::::::::::::  01234567890123456789
    $exec 3 "$regex_strlit" 'U"abcd"'
    $exec 3 "$regex_strlit" 'utf32'
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
