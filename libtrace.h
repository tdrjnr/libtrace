/**
 * libtrace.h
 *
 * Main library include file.
 */

#ifndef LIBTRACE_LIBTRACE_H
#define LIBTRACE_LIBTRACE_H

#include "task.h"

/// libtrace return codes.
typedef enum {
	LIBTRACE_SUCCESS = 0,
	LIBTRACE_NO_MEMORY,
	LIBTRACE_INVALID_TARGET,
} trace_result_t;


/// User-supplied callback type to call when task enters a syscall.
typedef void (*syscall_entry_handler_t) (struct trace_task* task);

/// User-supplied callback type to call when task returns from a syscall.
typedef void (*syscall_exit_handler_t) (struct trace_task* task);


/**
 * User-specified set of callbacks to perform upon events on specific syscall.
 *
 * Library has a default "do-nothing" handlers for all syscalls.
 * User can override both the default handler and handlers for each specific syscalls.
 *
 * If you would like to use a default handler for some of the events leave it as NULL.
 */
struct trace_syscall_handler
{
	syscall_entry_handler_t	on_entry;
	syscall_exit_handler_t	on_exit;
};


struct trace_context
{
	struct trace_task*	task_head;
	trace_syscall_handler	default;
	trace_syscall_handler*	handlers;
};


/**
 * Run and trace new process.
 */
trace_result_t trace_create_target(const char* path, char** argv);


/**
 * Attach to existing process and trace it.
 */
trace_result_t trace_attach_target(int pid);


#endif

