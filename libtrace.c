/**
 * libtrace.c
 *
 * Main library interface implementation.
 */

#include "libtrace.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>


static ftrace_result_t start(void);
static void trace_task(struct ftrace_task* task);


// Extract ptrace event from wait status code.
#define FTRACE_EVENT(status) ( (status) >> 16 )


// Add new task entry into list.
static int add_task(int pid) 
{
	struct ftrace_task* task = malloc(sizeof(struct ftrace_task));
	if(!task) {
		return -1;
	}

	memset(task, 0, sizeof(*task));
	task->pid = pid;

	task->next = g_task_list;
	g_task_list = task;

	return 0;	
}	


// Find task by pid.
static struct ftrace_task* find_task(int pid)
{
	struct ftrace_task* task = g_task_list;

	while(task) {
		if(task->pid == pid) {
			return task;
		}

		task = task->next;
	}

	return NULL;
}


// Remove task from list
static void remove_task(struct ftrace_task* task)
{
	if(task == g_task_list) {
		g_task_list = task->next;
	} else {
		struct ftrace_task* prev = g_task_list;
		while(prev && (prev->next != task)) {
			prev = prev->next;
		}

		FTRACE_ASSERT(prev && (prev->next == task));
		prev->next = task->next;
	}

	free(task);
}

// Set ptrace options for new processes.
static void setup_ptrace_options(int pid)
{
	ptrace(PTRACE_SETOPTIONS, pid, PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE); 
}


trace_result_t trace_create_target(const char* path, char** argv)
{
	int pid = vfork();
	if(pid < 0) {
		FTRACE_LOG("Error: vfork failed\n");
		return -1;
	}

	if(pid == 0) {
		
		/* Child. */

		long rc = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		if(rc < 0) {
			FTRACE_LOG("Error: ptrace failed\n");
			return -1;
		}

		execvp(name, argv);

		/* Should not be reachable if ok. */
		FTRACE_LOG("Error: exec failed\n");
		exit(0);
	}

	/* Parent. Wait for child to exec and start tracing. */
	
	int status = 0;
	waitpid(pid, &status, WUNTRACED);
	
	if(WIFEXITED(status)) {
		FTRACE_LOG("Error: child process failed to execute\n");
		return FTRACE_INVALID_PROCESS;
	}

	// Add first task.
	if(0 != add_task(pid)) {
		return FTRACE_NO_MEMORY;
	}

	setup_ptrace_options(pid);
	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

	FTRACE_LOG("Starting trace for pid %d\n", pid);

	return start();
}

trace_result_t trace_attach_target(int pid)
{
	return FTRACE_INVALID_PROCESS;
}


// Start tracing of created or attached process.
static trace_result_t start(void) 
{
	int status;
	int pid;
	int signo;

	// wait on all child processes.
	while((pid = waitpid(-1, &status, 0)) != -1) {

		signo = 0;
		struct ftrace_task* task = find_task(pid);

		// Precondition: stopped task is known.
		FTRACE_ASSERT(task != NULL);

		// task exited, need to remove it from trace list.
		if(WIFEXITED(status) || WIFSIGNALED(status)) {

			FTRACE_LOG("task %d terminated\n", pid);
			remove_task(task);
		
		// task is stopped
		} else if(WIFSTOPPED(status)) {
		
			signo = WSTOPSIG(status);
		
			// SIGSTOP/SIGTRAP is our cue.
			if((signo == SIGTRAP) || (signo == SIGSTOP)) {

				// We stop process in case of cloning/forking and when it is entering/exiting syscalls.
				// To deffirentiate we use ptrace event code in process status. 
				switch(FTRACE_EVENT(status)) {
				case PTRACE_EVENT_FORK: 	/* fallthru */
				case PTRACE_EVENT_VFORK:	/* fallthru */
				case PTRACE_EVENT_CLONE: {
					// task cloned/forked - add another pid context.
					int new_pid;
					ptrace(PTRACE_GETEVENTMSG, task->pid, NULL, &new_pid);
					add_task(new_pid);
					FTRACE_LOG("task %d cloned, new pid is %d\n", task->pid, new_pid);
					break;
				}
				
				case 0: {
					// syscall trap
					trace_task(task);
				}

				default: break;
				};
			}

		} else {
			FTRACE_LOG("Unknown task %d status\n", pid);
		}

		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	}

	// stop tracing.
	return FTRACE_SUCCESS;
}
