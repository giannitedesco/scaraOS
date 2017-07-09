## Architecture
ARCH := ia32
EXTRA_DEFS=-DKDEBUG=1
EXTRA_DEFS+=-DPAGE_POISON=1
EXTRA_DEFS+=-DOBJCACHE_POISON=1
EXTRA_DEFS+=-DOBJCACHE_DEBUG_FREE=1

TOPDIR :=  .
#$(shell /bin/pwd)
KERNEL_DIR := $(TOPDIR)/kernel
ARCH_DIR := $(TOPDIR)/arch-$(ARCH)
FS_DIR := $(TOPDIR)/fs
DRIVERS_DIR := $(TOPDIR)/drivers

.PHONY: all clean squeaky boot_floppy userland

## Target toolchain prefix
CROSS_COMPILE :=

## Boot and root image(s)
BOOT_IMAGE := boot.img

# path relative from user dir
ROOT_IMAGE := ../boot.img

## command locations
SH := /bin/sh
RM := rm
MAKE := make
LN := ln
CP := cp

ifdef SPARSE
GCC := $(CROSS_COMPILE)cgcc
EXTRA_DEFS+=-Wno-old-initializer
else
GCC := $(CROSS_COMPILE)gcc
endif
CC  := $(CROSS_COMPILE)gcc
AR  := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

# Default target
TARGET: all

# Compiler flags
#	-fno-inline \
#
CFLAGS  := -ggdb -O2 -nostdlib \
	-flto -fwhole-program -fno-fat-lto-objects \
	-finline-functions -ftree-partial-pre -fgcse-after-reload -fipa-cp-clone -fipa-pta \
	-z execstack \
	-static \
	-static-libgcc \
	-fno-pie -fno-PIE \
	-m32 -ffreestanding -fno-stack-protector \
	-Wsign-compare \
	-Wcast-align \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wmissing-noreturn \
	-Wmissing-format-attribute \
	-Wno-cast-align \
	-I$(TOPDIR)/include $(EXTRA_DEFS)

./include/arch:
	@echo " [SYMLINK] ./include/arch -> arch-$(ARCH)"
	@$(LN) -sf arch-$(ARCH) ./include/arch

# templates
%.o %.d: %.c ./include/arch Makefile
	@echo " [C] $(patsubst %.d, %.c, $@)"
	@$(GCC) $(CFLAGS) \
		-MMD -MF $(patsubst %.o, %.d, $@) -MT $(patsubst %.d, %.o, $@) \
		-c -o $(patsubst %.d, %.o, $@) $< 
%.d %.o: %.S ./include/arch Makefile
	@echo " [ASM] $(patsubst %.d, %.S, $@)"
	@$(GCC) $(CFLAGS) -D__ASM__ \
		-MMD -MF $(patsubst %.o, %.d, $@) -MT $(patsubst %.d, %.o, $@) \
		-c -o $(patsubst %.d, %.o, $@) $< 

include arch-$(ARCH)/Makefile
include kernel/Makefile
include fs/Makefile
include drivers/Makefile

IMAGE_OBJ := $(patsubst %.S, %.o, $(ARCH_ASM_SOURCES)) \
		$(patsubst %.c, %.o, $(ARCH_C_SOURCES)) \
		$(patsubst %.c, %.o, $(KERNEL_C_SOURCES)) \
		$(patsubst %.c, %.o, $(FS_C_SOURCES)) \
		$(patsubst %.c, %.o, $(DRIVERS_C_SOURCES))

ALL_SOURCES := $(ARCH_C_SOURCES) $(ARCH_ASM_SOURCES) \
		$(KERNEL_C_SOURCES) \
		$(FS_C_SOURCES) \
		$(DRIVERS_C_SOURCES)

# Generate dependencies
ALL_DEPS := $(patsubst %.S, %.d, $(ARCH_ASM_SOURCES)) \
		$(patsubst %.c, %.d, $(ARCH_C_SOURCES)) \
		$(patsubst %.c, %.d, $(KERNEL_C_SOURCES)) \
		$(patsubst %.c, %.d, $(FS_C_SOURCES)) \
		$(patsubst %.c, %.d, $(DRIVERS_C_SOURCES))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),squeaky)
-include $(ALL_DEPS)
endif
endif

kernel.elf: Makefile $(IMAGE_OBJ) $(ARCH_DIR)/kernel.lnk
	@echo " [LINK] $@"
	@$(GCC) $(CFLAGS) -Wl,-melf_i386 -Wl,-nostdlib \
		-nostartfiles \
		-Wl,-z,execstack \
		-Wl,--build-id=none \
		-Wl,-T,$(ARCH_DIR)/kernel.lnk -o $@ \
		$(IMAGE_OBJ) -nostdlib -lgcc

kernel.elf.stripped: kernel.elf
	@echo " [STRIP] $@"
	@$(CP) $< $@
	@$(STRIP) $@

kernel.elf.gz: kernel.elf.stripped
	@echo " [COMPRESS] $@"
	@gzip -c < $< > $@

all: kernel.elf.gz

userland:
	@echo " [USERLAND]"
	+$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) \
		ROOT_IMAGE="$(ROOT_IMAGE)" \
		-C user root_image

$(BOOT_IMAGE): userland kernel.elf.gz menu.lst
	@echo " [BOOTFLOPPY] $@"
	@e2fsck -y $(BOOT_IMAGE) || true
	@e2cp kernel.elf.gz $(BOOT_IMAGE):kernel
	@e2cp menu.lst $(BOOT_IMAGE):

boot_floppy: $(BOOT_IMAGE)

clean:
	$(RM) -f $(IMAGE_OBJ) $(ALL_DEPS) \
		$(KERNEL_OBJ) $(KERNEL_DIR)/kernel.o \
		$(ARCH_OBJ) $(ARCH_DIR)/arch.o \
		$(FS_OBJ) $(FS_DIR)/fs.o \
		kernel.elf kernel.elf.stripped kernel.elf.gz
	make -C user clean

squeaky: clean
	$(RM) -f ./include/arch
