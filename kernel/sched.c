/* 
 * Contains scheduling primitives such as the scheduler itself,
 * wait queues and eventually semaphores...
*/

#define DEBUG_MODULE 0
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

	dprintk("sched: Task %s going to sleep\n", t->name);
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

/* The task that becomes the idle task needs to call this function - interrupts
 * should be disabled by the caller */
void sched_init(void)
{
	struct task *t = __this_task;
	t->state = TASK_RUNNING;
	t->name = "[idle]";
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
	dprintk("sched: sleeper %s back to runq\n", t->name);

	unlock_irq(flags);
}

static void task_push_word(struct task *tsk, uint32_t word)
{
	tsk->t.esp -= sizeof(word);
	*(uint32_t *)tsk->t.esp = word;
}

static void kthread_init(void (*thread_func)(void *), void *priv)
{
	sti();
	printk("kthread_init: thread_func=%x priv=%x\n", thread_func, priv);
	(*thread_func)(priv);
	printk("FIXME: call __exit() when implemented\n");
	idle_task_func();
}

int kernel_thread(const char *proc_name,
			void (*thread_func)(void *),
			void *priv)
{
	struct task *tsk;
	static pid_t pid = 1;
	static pid_t ret;
	//unsigned int i;
	long eflags;

	tsk = alloc_page();
	if ( NULL == tsk )
		return -1;

	ret = pid++;
	tsk->pid = ret;
	tsk->name = proc_name;
	tsk->t.eip = (uint32_t)kthread_init;
	tsk->t.esp = (uint32_t)tsk + PAGE_SIZE;

	/* push arguments to kthread init */
	task_push_word(tsk, (uint32_t)priv);
	task_push_word(tsk, (uint32_t)thread_func);

	/* pusha - why the fuck this isn't needed i have no idea... */
	//for(i = 0; i < 8; i++)
	//	task_push_word(tsk, 0);

	/* pushfl */
	get_eflags(eflags);
	task_push_word(tsk, eflags);

	task_to_runq(tsk);
	return tsk->pid;
}

/* Crappy round-robin type scheduler, just picks the
 * next task on the run queue
 *
 * FIXME: idle task needs special treatment, don't run it when other tasks
 * are in the runnable state
*/
void sched(void)
{
	struct task *current;
	struct task *head;
	long flags;

	lock_irq(flags);

	current = __this_task;
	head = list_entry(runq.next, struct task, list);

	/* Task is sleeping, go to head of runq */
	if ( current->state == TASK_SLEEPING ) {
		dprintk("sched: Current task sleeps: switch from %s to %s\n",
			current->name, head->name);
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

	/* only one runnable task, nothing to do */
	if ( current == head ) {
		unlock_irq(flags);
		return;
	}

	dprintk("sched: switch from %s to %s\n", current->name, head->name);
	current->state = TASK_READY;
	head->state = TASK_RUNNING;
	switch_task(current, head);
	unlock_irq(flags);
}
