GCC    := $(CROSS_COMPILE)gcc
CC     := $(CROSS_COMPILE)gcc
LD     := $(CROSS_COMPILE)ld
AR     := $(CROSS_COMPILE)ar
STRIP  := $(CROSS_COMPILE)strip
TOPDIR :=  .

.PHONY: all clean boot_floppy
TARGET: all

LDFLAGS := -melf_i386 -nostdlib -N --strip-all -e_start
CFLAGS  := -pipe -ggdb -Os -Wall -ffreestanding -fno-stack-protector \
	-Wsign-compare -Wcast-align -Waggregate-return \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wmissing-noreturn \
	-Wmissing-format-attribute -m32 \
	-I$(TOPDIR)/../include $(EXTRA_DEFS)

%.o: %.c Makefile
	@echo " [C] $@"
	@$(GCC) $(CFLAGS) -c -o $@ $<
%.o: %.S Makefile
	@echo " [ASM] $@"
	@$(GCC) $(CFLAGS) -D__ASM__ -c -o $@ $<
%.a:
	@echo " [AR] $@"
	@$(AR) crs $@ $^
%.d: %.c Makefile
#	@echo " [DEP:C] $@"
	@$(GCC) $(CFLAGS) -MM $< -MF $@ -MT $(patsubst %.d, %.o, $@)
%.d: %.S Makefile
#	@echo " [DEP:ASM] $@"
	@$(GCC) $(CFLAGS) -MM $< -MF $@ -MT $(patsubst %.d, %.o, $@)

C_RUNTIME := crt0.a
CRT_ASM_SOURCES := _crt0.S
CRT_C_SOURCES := _exit.c _fork.c _exec.c \
		_open.c _close.c _read.c _write.c \
		vfork.c fork.c string.c
CRT_OBJ :=  $(patsubst %.S, %.o, $(CRT_ASM_SOURCES)) \
		$(patsubst %.c, %.o, $(CRT_C_SOURCES))
$(C_RUNTIME): $(CRT_OBJ)

%: %.o $(C_RUNTIME)
	@echo " [LINK] $@"
	@$(LD) $(LDFLAGS) -o $@ $^

INIT_C_SOURCES := init.c
INIT_OBJ := $(patsubst %.c, %.o, $(INIT_C_SOURCES))
init: $(INIT_OBJ)

CPUHOGA_C_SOURCES := cpuhog-a.c
CPUHOGA_OBJ := $(patsubst %.c, %.o, $(CPUHOGA_C_SOURCES))
cpuhog-a: $(CPUHOGA_OBJ)

CPUHOGB_C_SOURCES := cpuhog-b.c
CPUHOGB_OBJ := $(patsubst %.c, %.o, $(CPUHOGB_C_SOURCES))
cpuhog-b: $(CPUHOGB_OBJ)

CAT_C_SOURCES := cat.c
CAT_OBJ := $(patsubst %.c, %.o, $(CAT_C_SOURCES))
cat: $(CAT_OBJ)

TRUE_ASM_SOURCES := true.S
TRUE_OBJ := $(patsubst %.c, %.o, $(CAT_ASM_SOURCES))
true: $(TUE_OBJ)

TRUE_ASM_SOURCES := false.S
TRUE_OBJ := $(patsubst %.c, %.o, $(CAT_ASM_SOURCES))
true: $(TUE_OBJ)

SBIN_TARGETS := init cpuhog-a cpuhog-b
BIN_TARGETS := cat true false
ALL_TARGETS := $(C_RUNTIME) $(SBIN_TARGETS) $(BIN_TARGETS)

ALL_OBJ := $(CRT_OBJ) $(INIT_OBJ) $(CPUHOGA_OBJ) $(CPUHOGB_OBJ) $(CAT_OBJ) \
		$(TRUE_OBJ) $(FALSE_OBJ)
ALL_ASM := $(CRT_ASM_SOURCES) $(TRUE_ASM_SOURCES)
ALL_C   := $(CRT_C_SOURCES) $(INIT_C_SOURCES) $(CPUHOGA_C_SOURCES) $(CPUHOGB_C_SOURCES) $(CAT_C_SOURCES)

ALL_DEP :=  $(patsubst %.S, %.d, $(ALL_ASM)) \
		$(patsubst %.c, %.d, $(ALL_C) )

ifneq ($(MAKECMDGOALS),clean)
-include $(ALL_DEP)
endif

all: Makefile $(ALL_TARGETS)

boot_floppy: all
	@e2cp $(SBIN_TARGETS) $(BOOT_FLOPPY):/sbin/
	@e2cp $(BIN_TARGETS) $(BOOT_FLOPPY):/bin/
	@e2cp ../README $(BOOT_FLOPPY):/README

clean:
	@rm -f $(ALL_OBJ) $(ALL_DEP) $(ALL_TARGETS)
