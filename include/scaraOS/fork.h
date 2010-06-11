#ifndef __FORK_HEADER_INCLUDED__
#define __FORK_HEADER_INCLUDED__

#define FORK_MEM	(1<<0) /* copy-on-write */
#define FORK_PID	(1<<1) /* allocate a new PID */
#define FORK_FD		(1<<2) /* separate file-descriptor table */
#define FORK_FS		(1<<3) /* separate file-system context */
#define FORK_NOFUNC	(1<<3) /* if se thent child IP set POSIX stylee */

#ifndef __ASM__
int _sys_fork(unsigned int flags, void (*fn)(void *), void *priv, void *stack);
#endif /* __ASM__ */

#endif /* __FORK_HEADER_INCLUDED__ */
