include $(TOPDIR)/Make.opts

OBJS=entry.o setup.o io.o vga.o idt.o 8259a.o \
	intr.o irq.o pc_keyb.o timer.o task.o \
	floppy.o dma.o pci.o

all: $(OBJS)
	$(GCC) -o kernel.elf -T kernel.lnk -nostdlib -Wl,-N -Wl \
		$(OBJS) ../kernel/kernel.o ../fs/fs.o
	$(STRIP) kernel.elf
	gzip kernel.elf
	mv kernel.elf.gz kernel.elf

clean:
	$(RM) -f *.o .depend kernel.elf
