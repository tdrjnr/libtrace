/**
 * task.c
 *
 * Primary perpose of this file is to contain all architecture dependencies in accessing traced task memory and info.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>

#include "task.h"
#include "ftrace.h"


ftrace_word_t task_peekword(struct ftrace_task* task, const void* uaddr)
{
	return ptrace(PTRACE_PEEKDATA, task->pid, uaddr, NULL);
}


ftrace_word_t task_peekwordoff(struct ftrace_task* task, const void* uaddr, unsigned offset)
{
	return ptrace(PTRACE_PEEKDATA, task->pid, uaddr + offset, NULL);
}


void task_peekmem(struct ftrace_task* task, const void* uaddr, void* out_buf, unsigned nbytes)
{
	unsigned nwords = nbytes / FTRACE_WORD_SIZE;
	unsigned residue = nbytes % FTRACE_WORD_SIZE;

	while(nwords--) {
		ftrace_word_t word = task_peekword(task, uaddr);
		memcpy(out_buf, &word, sizeof(word));

		out_buf += sizeof(word);
		uaddr += sizeof(word);
	}

	if(residue) {
		ftrace_word_t word = task_peekword(task, uaddr);
		memcpy(out_buf, &word, residue);
	}	
}


unsigned task_peekstr(struct ftrace_task* task, const void* uaddr, char* out_str, unsigned bufsiz)
{
	union {
		ftrace_word_t 	word;
		char		bytes[sizeof(ftrace_word_t)];
	} udata;

	int eos = 0;
	unsigned offset = 0;

	
	while(!eos && (offset < bufsiz) /* -1 byte for NULL-terminator */) {

		memset(&udata, 0, sizeof(udata));
		udata.word = task_peekwordoff(task, uaddr, offset);

		int i;
		for(i = 0; i < sizeof(ftrace_word_t); ++i) {

			//FTRACE_LOG("%c", udata.bytes[i]);
			*out_str++ = udata.bytes[i];
			if(udata.bytes[i] == '\0') {
				eos++;
				break;
			}

		}

		offset += sizeof(udata.bytes);
	}

	//out_str[offset] = '\0';
	
	return offset;
}


void task_syscall_info(struct ftrace_task* task, struct syscall_info* out_info)
{
  struct user_regs_struct regs;
  ptrace(PTRACE_GETREGS, task->pid, NULL, &regs);

#ifdef x86_64
  out_info->sysno = regs.orig_rax;
  out_info->rc = regs.rax;
  out_info->p1 = regs.rdi;
  out_info->p2 = regs.rsi;
  out_info->p3 = regs.rdx;
  out_info->p4 = regs.r10;
  out_info->p5 = regs.r8;
  out_info->p6 = regs.r9;
#elif defined(x86_32)
  out_info->sysno = regs.orig_eax;
  out_info->rc = regs.eax;
  out_info->p1 = regs.ebx;
  out_info->p2 = regs.ecx;
  out_info->p3 = regs.edx;
  out_info->p4 = regs.esi;
  out_info->p5 = regs.edi;
  out_info->p6 = regs.ebp;
#endif
}


