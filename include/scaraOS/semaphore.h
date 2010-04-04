#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <scaraOS/task.h>

typedef int sem_t;

#define SEMAPHORE_INIT(name, initval) \
	{.waiters = WAITQ_INIT(name.waiters), .val = initval}
#define INIT_SEMAPHORE(ptr, initval) \
	do { INIT_WAITQ(&(ptr)->waiters); (ptr)->val = initval; } while(0)

struct semaphore {
	struct waitq	waiters;
	sem_t		val;
};

void sem_init(struct semaphore *sem, sem_t val);
void sem_V(struct semaphore *sem);
void sem_P(struct semaphore *sem);

#endif /* SEMAPHORE_H */
