/* 
 * Contains scheduling primitives such as the scheduler itself,
 * wait queues and eventually semaphores...
*/
#include <kernel.h>
#include <task.h>
#include <arch/processor.h>

struct task *runq=NULL;

/* Sleep the current task on a wait queue */
void sleep_on(struct waitq *q)
{
	struct task *t=__this_task;

	cli();
	t->state=TASK_SLEEPING;

	/* Remove from run queue */
	t->next->prev=t->prev;
	t->prev->next=t->next;

	/* Add to wait queue */
	t->prev=(struct task *)q;
	t->next=q->next;
	q->next->prev=t;
	q->next=t;

	sti();
	sched();
}

/* Wake up all sleepers on a wait queue */
void wake_up(struct waitq *q)
{
	struct task *w;
	long flags;

	lock_irq(flags);

	/* Add all waiting tasks to runq */
	for(w=q->next; w!=(struct task *)q;) {
		struct task *n=w->next;
		task_to_runq(w);
		w=n;
	}

	/* Reset waitqueue */
	q->next=(struct task *)q;
	q->prev=(struct task *)q;

	unlock_irq(flags);

	sched();
}

/* The task that becomes the idle task needs to
 * call this function - interrupts should be 
 * disabled by the caller */
void sched_init(void)
{
	runq=__this_task;

	runq->state=TASK_RUNNING;
	runq->preempt=0;
	runq->pid=0;

	runq->next=runq;
	runq->prev=runq;
}

/* Put a task on the runq */
void task_to_runq(struct task *t)
{
	long flags;
	lock_irq(flags);

	t->state=TASK_READY;

	t->next=runq->next;
	t->prev=runq;
	runq->next->prev=t;
	runq->next=t;

	unlock_irq(flags);
}

/* Crappy round-robin type scheduler, just picks the
 * next task on the run queue - arg */
void sched(void)
{
	struct task *t=__this_task;

	if ( t->state==TASK_SLEEPING ) {
		/* Task is sleeping, go to head of runq */
		switch_task(t, runq->next);
	}else if ( t->next != t ) {
		struct task *n=t->next;

		/* Try and avoid the idle task */
		if ( n == runq )
			n=n->next;
		
		t->state=TASK_READY;
		n->state=TASK_RUNNING;
		switch_task(t,n);
	}
}
