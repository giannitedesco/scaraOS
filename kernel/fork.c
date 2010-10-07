#include <scaraOS/kernel.h>
#include <scaraOS/task.h>
#include <scaraOS/fork.h>
#include <scaraOS/vfs.h>

int _sys_fork(unsigned int flags, void (*fn)(void *), void *priv, void *stack)
{
	struct task *tsk, *current;
	vaddr_t ip, sp;
	pid_t pid;

	current = __this_task;

	dprintk("fork: flags=0x%x fn=%p priv=%p stack=%p\n",
		flags, fn, priv, stack);

	if ( flags & FORK_NOFUNC ) {
		dprintk("fork: - no thread func\n");
		ip = MAP_INVALID;
		sp = MAP_INVALID;
		if ( !(flags & FORK_MEM) ) {
			dprintk("fork: warning: parent/child "
				"sharing same stack\n");
		}
	}else{
		dprintk("fork: - using thread func %p / stack %p\n", fn, stack);
		if ( !uaddr_ok((vaddr_t)fn, 0) ||
				!uaddr_ok((vaddr_t)stack, 0) ) {
			dprintk("fork: ... which are no damn good\n");
			return -1;
		}
		ip = (vaddr_t)fn;
		sp = (vaddr_t)stack;
	}

	if ( flags & FORK_PID ) {
		pid = pid_alloc();
		dprintk("fork: - new pid %lu\n", pid);
	}else{
		pid = current->pid;
		dprintk("fork: - keep old pid %lu\n", pid);
	}

	tsk = alloc_page();
	if ( NULL == tsk ) {
		if ( flags & FORK_PID )
			pid_release(pid);
		return -1;
	}

	memset(tsk, 0, sizeof(*tsk));

	tsk->pid = pid;
	tsk->name = "[forked-task]";
	tsk->ctx = mem_ctx_get(current->ctx);
	if ( current->root )
		tsk->root = iref(current->root);
	if ( current->cwd )
		tsk->cwd = iref(current->cwd);
	task_clone(current, tsk, ip, sp);

	task_to_runq(tsk);
	return pid;
}
