#include <sys/types.h>
#include <sys/queue.h>

struct resource {
	TAILQ_ENTRY(resource) r_next;
	u_long	r_start;
	u_long	r_len;
};

struct rman {
	TAILQ_HEAD(, resource)	rm_res;
	u_int	rm_blksz;
	int	rm_entries;
};

typedef int (*rman_pool_init)(struct rman *);

int	rman_init(struct rman *, u_int blksz, rman_pool_init);
void	rman_add(struct rman *, u_long, u_long);
int	rman_select(struct rman *, u_long *, u_long *, u_int);
void	rman_release(struct rman *, u_long, u_long);
