# DannyNiu/NJF, 2025-12-25. Public Domain.

## basic information. ##

LibraryName = librematch
ProductName = librematch
MajorVer = 0
MinorVer = 1
ProductVer = ${MajorVer}.${MinorVer}
ProductRev = ${ProductVer}.2

PkgConfigName = The librematch POSIX regular expression library
PkgConfigDesc = A POSIX regex library.
PkgConfigURLs = \
	https://github.com/dannyniu/librematch \
	https://gitee.com/dannyniu/librematch

## target product. ##

FILE_EXT_ELF = a
FILE_EXT_MACHO = a

FILE_EXT = ${FILE_EXT_ELF}

ccOpts = -Wall -Wextra -fPIC # If I guessed wrong, specify on command line.
LD = ${CC} # 2024-03-09: direct linker invocation lacks some default flags.

# ``-G'' is the System V and XPG-8/SUSv5 option for producing
# dynamic-linking library. Will need adaptation for pre-existing linkers.
DLLFLAGS = -G

## object files. ##

include objects.mk

OBJS_GROUP_WITH_ADDITION =
CFLAGS_GROUP_WITH =

INPUT_OBJECTS = ${OBJS_GROUP_ALL} ${OBJS_GROUP_WITH_ADDITION}
CFLAGS = ${ccOpts} ${CFLAGS_GROUP_WITH}
