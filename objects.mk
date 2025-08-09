# DannyNiu/NJF, 2023-03-17. Public Domain.

OBJ_COMMON = \
    src/librematch.o \
    src/regcomp-ere.o \
    src/regcomp-bre.o \
    src/regcomp-brackets.o \
    src/regcomp-interval.o

OBJ_SYMM_X86 =
CFLAGS_SYMM_X86 =

OBJ_SYMM_ARM =
CFLAGS_SYMM_ARM =

OBJ_SYMM_PPC =
CFLAGS_SYMM_PPC =

OBJS_GROUP_ALL = \
    ${OBJ_COMMON}

OBJS_GROUP_X86_ADDITION = \
    ${OBJ_SYMM_X86}

CFLAGS_GROUP_X86 = \
    ${CFLAGS_SYMM_X86}

OBJS_GROUP_ARM_ADDITION = \
    ${OBJ_SYMM_ARM} ${OBJ_SYMM_ShangMi_ARM}

CFLAGS_GROUP_ARM = \
    ${CFLAGS_SYMM_ARM} ${CFLAGS_SYMM_ShangMi_ARM}

OBJS_GROUP_PPC_ADDITION = \
    ${OBJ_SYMM_PPC}

CFLAGS_GROUP_PPC = \
    ${CFLAGS_SYMM_PPC}
