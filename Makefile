PROG=sysfuzz
SRCS=argpool.c \
     fork.c \
     mmap.c \
     sched.c \
     sysfuzz.c
MK_MAN=no
WARNS=6

.include <bsd.prog.mk>
