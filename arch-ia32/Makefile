include $(TOPDIR)/Make.opts

OBJS=entry.o setup.o io.o vga.o idt.o 8259a.o \
	intr.o irq.o pc_keyb.o timer.o task.o \
	floppy.o dma.o pci.o mm.o

kernel.elf: $(OBJS)
	$(GCC) -o $@ -T kernel.lnk -nostdlib -Wl,-N -Wl \
		$^ ../kernel/kernel.o ../fs/fs.o
	$(STRIP) kernel.elf

kernel.elf.gz: kernel.elf
	gzip -c < $< > $@

all: kernel.elf.gz

clean:
	$(RM) -f *.o Make.dep kernel.elf kernel.elf.gz
