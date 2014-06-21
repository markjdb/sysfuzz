PROG=	sysfuzz
SRCS=	argpool.c \
	fork.c \
	options.c \
	sched.c \
	syscall.c \
	sysfuzz.c \
	util.c \
	vm.c

DEBUG_FLAGS+=-g

LDADD+=	-lnv

MAN=
WARNS?=	6

.include <bsd.prog.mk>
