#ifndef __TASK_HEADER_INCLUDED__
#define __TASK_HEADER_INCLUDED__

#include <arch/task.h>
#include <scaraOS/mm.h>

#define TASK_RUNNING	0
#define TASK_READY	1
#define TASK_SLEEPING	2 
#define TASK_ZOMBIE	3

#define WAITQ_INIT(name) { .list = LIST_HEAD_INIT(name.list) }
#define INIT_WAITQ(name) INIT_LIST_HEAD(&((name)->list))
struct waitq {
	struct list_head	list;
};

/* Task descriptor */
struct task {
	struct list_head 	list;
	uint32_t		state;
	struct thread		t;
	const char		*name;
	pid_t 			pid;
	uint32_t		exit_code;

	struct mem_ctx		*ctx;

	/* Filesystem info */
	struct inode		*root;
	struct inode		*cwd;
};

void task_set_context(struct task *, struct mem_ctx *);

/* Wait-queue manipulation */
void sleep_on(struct waitq *);
void wake_up(struct waitq *);
void wake_one(struct waitq *);

/* Run-queue manupulation */
void task_to_runq(struct task *);

int kernel_thread(const char *proc_name,
			int (*thread_func)(void *),
			void *priv);

/* The scheduler */
void sched_init(void);
void sched(void);

_noreturn _asmlinkage void kthread_init(int (*thread_func)(void *), void *priv);

/* Misc */
int mm_pagefault(struct task *tsk, vaddr_t va, unsigned prot);

/* System calls */
_noreturn void _sys_exit(uint32_t code);
int _sys_fork(uint32_t flags);

#endif /* __TASK_HEADER_INCLUDED__ */
