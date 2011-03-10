/**
 * task.h
 *
 * Provides traced task context and interface.
 * Takes care of platform differences in accessing task memory and regs.
 */

#ifndef LIBTRACE_TASK_H
#define LIBTRACE_TASK_H


#define LIBTRACE_WORD_SIZE sizeof(long)


/**
 * Syscall tracing info.
 */
struct syscall_info {
	long int sysnum;
	long int rc;
	long int p1, p2, p3, p4, p5, p6;
};


/**
 * Trace task.
 * Single execution thread to trace syscall entering/exiting. 
 * Tracer will keep track of all threads in traced process and its children one task for each thread.
 */
struct trace_task {
	int			pid;		// task process id.
	int			status;		// task waitpid(2) status.
	int			in_syscall;	// task is currently inside syscall.
	struct trace_task*	next;		// next task.
};


/**
 * Read word from task memory.
 * 
 * @uaddr	Process user space address to read the word from.
 * 
 * @return	Read word.
 */
ftrace_word_t task_peekword(struct trace_task* task, const void* uaddr);


/**
 * Read memory from task process.
 * 
 * @uaddr	Process user space address to start reading from.
 * @out_buf	Caller buffer to read into.
 * @nbytes	Number of bytes to read.
 */
void task_peekmem(struct trace_task* task, const void* uaddr, void* out_buf, unsigned nbytes);


/**
 * Read memory from task process.
 * 
 * @uaddr	Process user space base address to start reading from.
 * @offset	Base address offset.
 * @out_buf	Caller buffer to read into.
 * @nbytes	Number of bytes to read.
 */
static inline void task_peekmemoff(struct trace_task* task, const void* uaddr, unsigned offset, void* out_buf, unsigned nbytes) {
	task_peekmem(task, (const char*)uaddr + offset, out_buf, nbytes);
}


/**
 * Read ASCIIZ string from task memory.
 * 
 * @uaddr	String adress in process memory.
 * @out_buf	Caller buffer to read string into.
 * @bufsiz	Maximum number of bytes to read, including NULL-terminator.
 * 
 * @return	Total number of bytes read. Either bufsiz or less if NULL-terminator was found.
 */
unsigned task_peekstr(struct trace_task* task, const void* uaddr, char* out_str, unsigned bufsiz);


/**
 * Get syscall info for traced task.
 */
void task_syscall_info(struct trace_task* task, struct syscall_info* out_info);

#endif

