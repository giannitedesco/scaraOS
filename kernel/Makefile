include $(TOPDIR)/Make.opts

OBJS=printk.o ctype.o vsprintf.o string.o \
	kmalloc.o buddy.o \
	blk.o blkdev.o \
	sched.o

kernel.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

all: kernel.o

clean:
	$(RM) -f *.o Make.dep
