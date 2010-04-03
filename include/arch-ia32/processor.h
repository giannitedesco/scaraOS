#ifndef __ARCH_IA32_PROCESSOR__
#define __ARCH_IA32_PROCESSOR__

#ifndef __ASM__

enum cpu_feature_e {
	CPUID_FPU,
	CPUID_VME,
	CPUID_DE,
	CPUID_PSE,
	CPUID_TSC,
	CPUID_MSR,
	CPUID_PAE,
	CPUID_MCE,
	CPUID_CX8,
	CPUID_APIC,
	CPUID__R1,
	CPUID_SEP,
	CPUID_MTRR,
	CPUID_PGE,
	CPUID_MCA,
	CPUID_CMOV,
	CPUID_PAT,
	CPUID_PSE36,
	CPUID_PSN,
	CPUID_CLFSH,
	CPUID__R2,
	CPUID_DS,
	CPUID_ACPI,
	CPUID_MMX,
	CPUID_FXSR,
	CPUID_SSE,
	CPUID_SSE2,
	CPUID_SS,
	CPUID_HTT,
	CPUID_TM,
	CPUID__R3,
	CPUID_PBE,
};

struct cpu_info {
	uint32_t features;
	uint32_t loops_ms;
};

static inline unsigned long pf_address(void)
{
	unsigned long address;
	asm volatile("movl %%cr2, %0":"=r" (address));
	return address;
}

static inline uint32_t get_eflags(void)
{
	unsigned long ef;
	asm volatile("pushf; popl %0":"=r" (ef));
	return ef;
}

static inline void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
	asm volatile ("cpuid"
		: "=a" (*eax),
		  "=b" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "0" (op));
}

/* CPU information */
extern struct cpu_info cpu_bsp;
#define __this_cpu (&cpu_bsp)

#endif

#endif /* __ARCH_IA32_PROCESSOR__ */
