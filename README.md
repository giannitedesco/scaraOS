# ScaraOS: A modern multiboot kernel for learning OS fundamentals
## Copyright (c) 2001-2010 Gianni Tedesco (gianni at scaramanga dot co dot uk)
Released under the terms of the GNU GPL version 3 (see: COPYING)
---

ScaraOS started out as a good way for me to learn a lot of interesting stuff
all at once, things such as:
 - x86 assembly
 - deeper understanding of ia32
 - learning about the components of the IBM PC/AT
 - OS design fundamentals
 - How crap a typical PC BIOS is
 - How crap DOS is
 - How a computer system works as a whole, from hardware to BIOS, to
   bootloader, to kernel, to OS subsystems, to applications, to user interfaces.

ScaraOS might be of use to someone who also wants to write their own
kernel for fun. If so they are free to use my code, which is under GPL.
GPL ensures the spirit of co-operation and code sharing are passed on 
with my work.

Current features:
 - Mostly written in C
 - Multiboot compliant
 - Pre-emptive multi-tasking
 - Paged memory management including demand loading of files, anonymous
   memory areas and ELF binaries.
 - Virtual filesystem switch including inode and page cache
 - EXT2 support
 - Slab allocator
 - Block I/O subsystem
 - Floppy disk controller driver
 - 32bit protected mode
 - 8254 PIT support
 - 8259A PIC support
 - PCI support
 - VGA text mode output
 - PC serial driver
 - IDE hard disk driver

If you like and use this software then press [<img src="http://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif">](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=gianni%40scaramanga%2eco%2euk&lc=GB&item_name=Gianni%20Tedesco&item_number=scaramanga&currency_code=GBP&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted) to donate towards its development progress and email me to say what features you would like added.
