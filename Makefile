LIBTRACE=libtrace.so
LIBTRACE_OBJS=task.o libtrace.o
LIBTRACE_CFLAGS=-g -Wall -DLIBTRACE_DEBUG -DX86_64 -dynamic

all:$(LIBTRACE) Makefile

$(LIBTRACE):$(LIBTRACE_OBJS)
	gcc $(LIBTRACE_OBJS) -o $@

%.o:%.c
	gcc $(LIBTRACE_CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(LIBTRACE)

