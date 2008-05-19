include $(TOPDIR)/Make.opts

OBJS=vfs.o inode.o dentry.o \
	ext2.o

fs.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

all: fs.o

clean:
	$(RM) -f *.o .depend
