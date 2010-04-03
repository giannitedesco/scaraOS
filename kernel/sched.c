/* 
 * Contains scheduling primitives such as the scheduler itself,
 * wait queues and eventually semaphores...
*/

#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/vfs.h>

static LIST_HEAD(runq);
static LIST_HEAD(delq);

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
}

void wake_one(struct waitq *q)
{
	struct task *tsk;
	long flags;

	if ( list_empty(&q->list) )
		return;

	lock_irq(flags);

	/* Add all waiting tasks to runq */
	tsk = list_entry(q->list.next, struct task, list);
	list_del(&tsk->list);
	task_to_runq(tsk);

	unlock_irq(flags);
}
/* The task that becomes the idle task needs to call this function - interrupts
 * should be disabled by the caller */
static struct task *idle_task;
__init void sched_init(void)
{
	idle_task =  __this_task;
	memset(idle_task, 0, sizeof(*idle_task));
	idle_task->state = TASK_RUNNING;
	idle_task->name = "[idle]";
	idle_task->pid = 0;
	idle_task->ctx = get_kthread_ctx();
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

_noreturn static void do_exit(uint32_t code)
{
	struct task *tsk = __this_task;
	long flags;

	lock_irq(flags);
	list_move_tail(&tsk->list, &delq);
	tsk->exit_code = code;
	tsk->state = TASK_ZOMBIE;
	unlock_irq(flags);
	sched();
	panic("A BIT OF FUCKING PRECISION!\n");
}

_noreturn void _sys_exit(uint32_t code)
{
	do_exit(code);
}

_noreturn _asmlinkage void kthread_init(int (*thread_func)(void *), void *priv)
{
	sti();
	dprintk("kthread_init: thread_func=%p priv=%p\n", thread_func, priv);
	do_exit((*thread_func)(priv));
	idle_task_func();
}

int kernel_thread(const char *proc_name,
			int (*thread_func)(void *),
			void *priv)
{
	struct task *tsk, *current;
	static pid_t pid = 1;
	static pid_t ret;

	tsk = alloc_page();
	if ( NULL == tsk )
		return -1;

	current = __this_task;
	memset(tsk, 0, sizeof(*tsk));

	ret = pid++;
	tsk->pid = ret;
	tsk->name = proc_name;
	tsk->ctx = get_kthread_ctx();
	if ( current->root )
		tsk->root = iref(current->root);
	if ( current->cwd )
		tsk->cwd = iref(current->cwd);
	task_init_kthread(tsk, thread_func, priv);

	task_to_runq(tsk);
	return tsk->pid;
}

int _sys_fork(uint32_t flags)
{
	return -1;
}

/* Crappy round-robin type scheduler, just picks the
 * next task on the run queue
 */
static struct task *get_next_task(struct task *current)
{
	struct task *ret;

	if ( list_empty(&runq) ) {
		ret = idle_task;
		if ( current != ret)
			dprintk("Idling...\n");
	}else{
		ret = list_entry(runq.next, struct task, list);
		if ( current == ret ) {
			list_move_tail(&current->list, &runq);
			ret = list_entry(runq.next, struct task, list);
		}
	}

	BUG_ON(ret->state == TASK_SLEEPING);
	return ret;
}

static void flush_delq(void)
{
	struct task *tsk, *tmp;

	list_for_each_entry_safe(tsk, tmp, &delq, list) {
		BUG_ON(tsk->state != TASK_ZOMBIE);
		printk("task: %s exited with code %i (0x%.lx)\n",
			tsk->name, (int)tsk->exit_code, tsk->exit_code);
		list_del(&tsk->list);
		free_page(tsk);
	}
}

void sched(void)
{
	struct task *current;
	struct task *next;
	long flags;

	lock_irq(flags);

	current = __this_task;
	next = get_next_task(current);
	if ( current == next )
		goto out;

	dprintk("sched: switch from %s to %s\n", current->name, next->name);
	BUG_ON(task_stack_overflowed(next));
	if ( likely(current->state == TASK_RUNNING) )
		current->state = TASK_READY;
	next->state = TASK_RUNNING;
	switch_task(current, next);

	if ( unlikely(!list_empty(&delq)) )
		flush_delq();
out:
	unlock_irq(flags);
}
