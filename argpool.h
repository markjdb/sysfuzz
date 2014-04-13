#ifndef _ARGPOOL_H_
#define _ARGPOOL_H_

#include <sys/types.h>

struct arg_memblk {
	void	*addr;
	size_t	len;
};

void argpool_init(void);
void memblk_random(struct arg_memblk *);
int unmapblk(struct arg_memblk *);
u_int pagesize(void);

#endif /* _ARGPOOL_H_ */
