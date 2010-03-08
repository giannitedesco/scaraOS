include $(TOPDIR)/Make.opts

OBJS=entry.o setup.o io.o vga.o idt.o 8259a.o \
	intr.o irq.o pc_keyb.o timer.o task.o \
	floppy.o dma.o pci.o mm.o

kernel.elf: $(OBJS) kernel.lnk
	$(LD) -o $@ -T kernel.lnk -nostdlib -N \
		$(OBJS) ../kernel/kernel.o ../fs/fs.o

kernel.elf.stripped: kernel.elf
	$(CP) $< $@
	$(STRIP) $@

kernel.elf.gz: kernel.elf.stripped
	gzip -c < $< > $@

all: kernel.elf.gz

clean:
	$(RM) -f *.o Make.dep kernel.elf kernel.elf.gz
