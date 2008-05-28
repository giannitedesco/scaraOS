
.PHONY: multiboot deps

TOPDIR :=  $(shell /bin/pwd)
export TOPDIR

include Make.opts

./include/arch:
	$(LN) -sf arch-$(ARCH) ./include/arch

deps: 
	$(MAKE) -C kernel depend
	$(MAKE) -C fs depend
	$(MAKE) -C arch-$(ARCH) depend

all: ./include/arch deps
	$(MAKE) -C kernel all
	$(MAKE) -C fs all
	$(MAKE) -C arch-$(ARCH) all

clean:
	$(RM) -f ./include/arch
	$(MAKE) -C kernel clean
	$(MAKE) -C fs clean
	$(MAKE) -C arch-$(ARCH) clean

multiboot:
	e2fsck -y ./qemu/boot.img || true
#	mount -t ext2 ./qemu/boot.img ./qemu/tmp -o loop
#	cp ./arch-ia32/kernel.elf.gz ./qemu/tmp/kernel
#	cp menu.lst ./qemu/tmp/boot/grub/menu.lst
	e2cp arch-ia32/kernel.elf.gz ./qemu/boot.img:kernel
	e2cp menu.lst ./qemu/boot.img:boot/grub/
#	umount ./qemu/tmp
#	grub --device-map=/boot/grub/device.map --batch --no-floppy < grub_script
