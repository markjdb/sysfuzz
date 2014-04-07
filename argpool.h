#ifndef _ARGPOOL_H_
#define _ARGPOOL_H_

#include <sys/types.h>

struct arg_memblk {
	void	*addr;
	size_t	len;
};

void argpool_init(void);
struct arg_memblk *memblk_random(void);

#endif /* _ARGPOOL_H_ */
