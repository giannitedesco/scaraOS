## Architecture
ARCH := ia32
EXTRA_DEFS=-DKDEBUG=1
EXTRA_DEFS+=-DPAGE_POISON=1
EXTRA_DEFS+=-DOBJCACHE_POISON=1
EXTRA_DEFS+=-DMULTI_TASKING_DEMO=0

TOPDIR :=  .
#$(shell /bin/pwd)
KERNEL_DIR := $(TOPDIR)/kernel
ARCH_DIR := $(TOPDIR)/arch-$(ARCH)
FS_DIR := $(TOPDIR)/fs

.PHONY: all clean squeaky boot_floppy

## Target toolchain prefix
CROSS_COMPILE=

## command locations
SH := /bin/sh
RM := rm
MAKE := make
LN := ln
CP := cp

GCC := $(CROSS_COMPILE)gcc
CC  := $(CROSS_COMPILE)gcc
LD  := $(CROSS_COMPILE)ld
AR  := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

# Default target
TARGET: all

# Compiler flags
LDFLAGS := -melf_i386 -nostdlib -N
CFLAGS  :=-pipe -ggdb -Os -Wall -ffreestanding -fno-stack-protector \
	-Wsign-compare -Wcast-align -Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute -m32 \
	-I$(TOPDIR)/include $(EXTRA_DEFS)

./include/arch:
	@echo " [SYMLINK] ./include/arch -> arch-$(ARCH)"
	@$(LN) -sf arch-$(ARCH) ./include/arch

# templates
%.o: %.c ./include/arch Makefile
	@echo " [C] $@"
	@$(GCC) $(CFLAGS) -c -o $@ $<
%.o: %.S ./include/arch Makefile
	@echo " [ASM] $@"
	@$(GCC) $(CFLAGS) -D__ASM__ -c -o $@ $<
%.a:
	@echo " [AR] $@"
	@$(AR) crs $@ $^

%.d: %.c ./include/arch Makefile
#	@echo " [DEP:C] $@"
	@$(GCC) $(CFLAGS) -MM $< -MF $@ -MT $(patsubst %.d, %.o, $@)
%.d: %.S ./include/arch Makefile
#	@echo " [DEP:ASM] $@"
	@$(GCC) $(CFLAGS) -MM $< -MF $@ -MT $(patsubst %.d, %.o, $@)

include arch-$(ARCH)/Makefile
include kernel/Makefile
include fs/Makefile

ARCH_OBJ := $(patsubst %.S, %.o, $(ARCH_ASM_SOURCES)) \
		$(patsubst %.c, %.o, $(ARCH_C_SOURCES))
KERNEL_OBJ := $(patsubst %.c, %.o, $(KERNEL_C_SOURCES))
FS_OBJ := $(patsubst %.c, %.o, $(FS_C_SOURCES))

ALL_SOURCES := $(ARCH_C_SOURCES) $(ARCH_ASM_SOURCES) \
		$(KERNEL_C_SOURCES) \
		$(FS_C_SOURCES)

IMAGE_OBJ := $(ARCH_DIR)/arch.a \
		$(KERNEL_DIR)/kernel.a \
		$(FS_DIR)/fs.a

# Generate dependencies
ARCH_DEP := $(patsubst %.S, %.d, $(ARCH_ASM_SOURCES)) \
		$(patsubst %.c, %.d, $(ARCH_C_SOURCES))
KERNEL_DEP := $(patsubst %.c, %.d, $(KERNEL_C_SOURCES))
FS_DEP := $(patsubst %.c, %.d, $(FS_C_SOURCES))
ALL_DEPS := $(ARCH_DEP) $(KERNEL_DEP) $(FS_DEP)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),squeaky)
include $(ALL_DEPS)
endif
endif

$(KERNEL_DIR)/kernel.a: $(KERNEL_OBJ)
$(ARCH_DIR)/arch.a: $(ARCH_OBJ)
$(FS_DIR)/fs.a: $(FS_OBJ)

kernel.elf: $(ALL_DEPS) $(IMAGE_OBJ) $(ARCH_DIR)/kernel.lnk
	@echo " [LINK] $@"
	@$(LD) $(LDFLAGS) -T $(ARCH_DIR)/kernel.lnk -o $@ \
		--whole-archive $(IMAGE_OBJ)

kernel.elf.stripped: kernel.elf
	@echo " [STRIP] $@"
	@$(CP) $< $@
	@$(STRIP) $@

kernel.elf.gz: kernel.elf.stripped
	@echo " [COMPRESS] $@"
	@gzip -c < $< > $@

all: Makefile kernel.elf.gz

boot.img: kernel.elf.gz menu.lst
	@echo " [BOOTFLOPPY] $@"
	@e2fsck -y ./boot.img || true
	@e2cp kernel.elf.gz ./boot.img:kernel
	@e2cp menu.lst ./boot.img:

boot_floppy: boot.img

clean:
	$(RM) -f $(ALL_DEPS) \
		$(KERNEL_OBJ) $(KERNEL_DIR)/kernel.o \
		$(ARCH_OBJ) $(ARCH_DIR)/arch.o \
		$(FS_OBJ) $(FS_DIR)/fs.o \
		kernel.elf kernel.elf.stripped kernel.elf.gz

squeaky: clean
	$(RM) -f ./include/arch
