PROG=	sysfuzz
SRCS=	argpool.c \
	fork.c \
	params.c \
	sched.c \
	syscall.c \
	sysfuzz.c \
	util.c \
	vm.c

BINDIR=/usr/local/bin

DEBUG_FLAGS+=-g

LDADD+=	-lnv

MAN=
WARNS?=	6

.include <bsd.prog.mk>
