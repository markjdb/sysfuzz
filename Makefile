PROG=sysfuzz
SRCS=argpool.c \
     mem.c \
     sysfuzz.c
NO_MAN=
WARNS=6

.include <bsd.prog.mk>
