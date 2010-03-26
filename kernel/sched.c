/* 
 * Contains scheduling primitives such as the scheduler itself,
 * wait queues and eventually semaphores...
*/
#include <kernel.h>
#include <task.h>
#include <arch/processor.h>

static LIST_HEAD(runq);

/* Sleep the current task on a wait queue */
void sleep_on(struct waitq *q)
{
	struct task *t = __this_task;
	long flags;

	lock_irq(flags);

	/* Remove from run queue */
	t->state = TASK_SLEEPING;
	list_del(&t->list);

	/* Add to wait queue */
	list_add_tail(&t->list, &q->list);

	dprintk("Task %x going to sleep\n", t);
	sched();
	unlock_irq(flags);
}

/* Wake up all sleepers on a wait queue */
void wake_up(struct waitq *q)
{
	struct task *t, *tmp;
	long flags;

	lock_irq(flags);

	/* Add all waiting tasks to runq */
	list_for_each_entry_safe(t, tmp, &q->list, list) {
		list_del(&t->list);
		task_to_runq(t);
	}

	unlock_irq(flags);

	sched();
}

/* The task that becomes the idle task needs to
 * call this function - interrupts should be 
 * disabled by the caller */
void sched_init(void)
{
	struct task *t = __this_task;
	t->state = TASK_RUNNING;
	t->preempt = 0;
	t->pid = 0;
	list_add(&t->list, &runq);
}

/* Put a task on the runq */
void task_to_runq(struct task *t)
{
	long flags;
	lock_irq(flags);

	t->state = TASK_READY;
	list_add_tail(&t->list, &runq);
	dprintk("sleeper %x back to runq\n", t);

	unlock_irq(flags);
}

/* Crappy round-robin type scheduler, just picks the
 * next task on the run queue - arg */

 /* FIXME: keep idle task as last choice, if idle task is only item i
  * runq then enable schedule on timer ISR */
void sched(void)
{
	struct task *current;
	struct task *head;
	long flags;

	lock_irq(flags);

	current = __this_task;
	head = list_entry(runq.next, struct task, list);

	if ( current->state == TASK_SLEEPING ) {
		/* Task is sleeping, go to head of runq */
		dprintk("Current task sleeps: switch from %x to %x\n",
			current, head);
		head->state = TASK_RUNNING;
		switch_task(current, head);
		unlock_irq(flags);
		return;
	}

	/* sched called while runnable task is still running, expire the
	 * little fucker
	 */
	if ( current == head ) {
		list_move_tail(&current->list, &runq);
		head = list_entry(runq.next, struct task, list);
	}

	if ( current == head ) {
		unlock_irq(flags);
		return;
	}

	dprintk("Sched: switch from %x to %x\n", current, head);
	current->state = TASK_READY;
	head->state = TASK_RUNNING;
	switch_task(current, head);
}
