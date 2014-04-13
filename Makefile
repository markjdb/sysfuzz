PROG=sysfuzz
SRCS=argpool.c \
     mmap.c \
     sysfuzz.c
NO_MAN=
WARNS=6

.include <bsd.prog.mk>
