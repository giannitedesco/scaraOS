#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/semaphore.h>

void sem_V(struct semaphore *sem)
{
	long flags;
	lock_irq(flags);
	sem->val++;
	if ( sem->val > 0 )
		wake_one(&sem->waiters);
	unlock_irq(flags);
}

void sem_P(struct semaphore *sem)
{
	long flags;
	lock_irq(flags);
	while ( sem->val <= 0 )
		sleep_on(&sem->waiters);
	sem->val--;
	unlock_irq(flags);
}
