## Architecture
ARCH=ia32

TOPDIR :=  $(shell /bin/pwd)
KERNEL_DIR = $(TOPDIR)/kernel
ARCH_DIR = $(TOPDIR)/arch-$(ARCH)
FS_DIR = $(TOPDIR)/fs

.PHONY: dep all clean squeaky boot_floppy

## Target toolchain prefix
CROSS_COMPILE=

## command locations
SH=/bin/sh
RM=rm
MAKE=make
LN=ln
CP=cp

AS=$(CROSS_COMPILE)as
AS86=$(CROSS_COMPILE)as
GCC=$(CROSS_COMPILE)gcc
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

# Default target
TARGET: all

# Compiler flags
CFLAGS=-pipe -ggdb -Os -Wall -ffreestanding -fno-stack-protector \
	-Wsign-compare -Wcast-align -Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute \
	-I$(TOPDIR)/include

# templates
%.o: %.c
	$(GCC) $(CFLAGS) -c -o $@ $<
%.o: %.S
	$(GCC) $(CFLAGS) -c -o $@ $<

./include/arch:
	$(LN) -sf arch-$(ARCH) ./include/arch

include arch-$(ARCH)/Makefile
include kernel/Makefile
include fs/Makefile

ARCH_OBJ = $(patsubst %.S, %.o, $(ARCH_ASM_SOURCES)) \
		$(patsubst %.c, %.o, $(ARCH_C_SOURCES))
KERNEL_OBJ = $(patsubst %.c, %.o, $(KERNEL_C_SOURCES))
FS_OBJ = $(patsubst %.c, %.o, $(FS_C_SOURCES))

ALL_SOURCES = $(ARCH_C_SOURCES) $(ARCH_ASM_SOURCES) \
		$(KERNEL_C_SOURCES) \
		$(FS_C_SOURCES)

# Generate dependencies
Make.dep: Makefile
	$(GCC) $(CFLAGS) -M $(ALL_SOURCES) > $@

dep: ./include/arch Make.dep

$(KERNEL_DIR)/kernel.o: $(KERNEL_OBJ)
	$(LD) -r -o $@ $^

$(ARCH_DIR)/arch.o: $(ARCH_OBJ)
	$(LD) -r -o $@ $^

$(FS_DIR)/fs.o: $(FS_OBJ)
	$(LD) -r -o $@ $^

IMAGE_OBJ = $(ARCH_DIR)/arch.o \
		$(KERNEL_DIR)/kernel.o \
		$(FS_DIR)/fs.o
kernel.elf: dep $(IMAGE_OBJ) $(ARCH_DIR)/kernel.lnk
	$(LD) -o $@ -T $(ARCH_DIR)/kernel.lnk -nostdlib -N $(IMAGE_OBJ)

kernel.elf.stripped: kernel.elf
	$(CP) $< $@
	$(STRIP) $@

kernel.elf.gz: kernel.elf.stripped
	gzip -c < $< > $@

all: kernel.elf.gz

boot.img: kernel.elf.gz menu.lst
	e2fsck -y ./boot.img || true
	e2cp kernel.elf.gz ./boot.img:kernel
	e2cp menu.lst ./boot.img:boot/grub/

boot_floppy: boot.img

clean:
	$(RM) -f Make.dep \
		$(KERNEL_OBJ) $(KERNEL_DIR)/kernel.o \
		$(ARCH_OBJ) $(ARCH_DIR)/arch.o \
		$(FS_OBJ) $(FS_DIR)/fs.o \
		kernel.elf kernel.elf.stripped kernel.elf.gz

squeaky: clean
	$(RM) -f ./include/arch

# Include the dependency file if one exists
ifeq (Make.dep, $(wildcard Make.dep))
include Make.dep
endif
