#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/linker_set.h>
#include <sys/syscall.h>

SET_DECLARE(syscalls, struct scdesc);

#define	SYSCALL_ADD(desc)	DATA_SET(syscalls, desc)

enum argtype {
	ARG_UNSPEC,
	ARG_FD,
	ARG_PATH,
	ARG_SOCKET,
	ARG_MEMADDR,
	ARG_MEMLEN,
	ARG_MODE,
	ARG_PID,
	ARG_PROCDESC,
	ARG_IFLAGMASK,
	ARG_LFLAGMASK,
	ARG_CMD,
	ARG_UID,
	ARG_GID,
	ARG_KQUEUE,
};

/* System call argument descriptor. */
struct scargdesc {
	enum argtype	sa_type;	/* argument type */
	const char	*sa_name;	/* argument name */
	union {
		int	sa_iflags[sizeof(int) * NBBY];
		long	sa_lflags[sizeof(long) * NBBY];
#define	SA_MAXCMDS	64
		int	sa_cmds[SA_MAXCMDS];
	};
	int		sa_argcnt;
};

#define	SYSCALL_MAXARGS	8

/*
 * A system call descriptor. This contains all the static information needed
 * to test a given system call.
 */
struct scdesc {
	int		sd_num;		/* system call number */
	const char	*sd_name;	/* system call name */
	int		sd_nargs;	/* number of arguments */
	void (*sd_fixup)(u_long *);	/* pre-syscall hook */
	void (*sd_cleanup)(u_long *, u_long); /* post-syscall hook */
	struct scargdesc sd_args[];	/* argument descriptors */
};

#endif /* _SYSCALL_H_ */
