/*
   cpu.h
   CPU specific inlines and definitions for SHM DOS64
   Copyright (c) 2022, Ilya Shamukov <ilya.shamukov@gmail.com>
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option) 
   any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
   more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
   Place, Suite 330, Boston, MA 02111-1307 USA
*/

#pragma once
#include <common_types.h>
#include <kvector.h>

enum : uint64_t
{
	CPU_EXTENDED_MEMORY_BASE = 0x100000,
	CPU_PHYSICAL_ADDRESS_MASK = 0xFFFFFFFFFFFF
};

enum : uint64_t
{
	PAGE_SIZE = 4096,
	PAGE_MASK = 0xFFF,
	PAGE_SHIFT = 12
};

enum : uint64_t
{
	CPU_UPDATE_TLB_LIMIT = 0x400000
};

enum : uint64_t
{
	PAGE_FLAG_PRESENT = (1 << 0),
	PAGE_FLAG_WRITE = (1 << 1),
	PAGE_FLAG_USER = (1 << 2),
	PAGE_FLAG_WT = (1 << 3),
	PAGE_FLAG_CACHE_DISABLE = (1 << 4),
	PAGE_FLAG_ACCESSED = (1 << 5),
	PAGE_FLAG_DIRTY = (1 << 6),
	PAGE_FLAG_SIZE = (1 << 7),
	PAGE_FLAG_GLOBAL = (1 << 8),
	PAGE_FLAG_ALLOCATED = (1 << 9),
	PAGE_FLAG_MEMZERO = (1 << 10)
};

enum : uint64_t
{
	CR0_CACHE_DISABLE = (1 << 30),
	CR0_NOT_WRITE_TROUGHT = (1 << 29),
	CR0_WRITE_PROTECT = (1 << 16),
	CR0_NUMERIC_ERROR = (1 << 5),
	CR0_EMULATION = (1 << 2)
};

enum : uint64_t
{
	CR4_PAGE_GLOBAL_ENABLE = (1 << 7),
	CR4_OSFXSR = (1 << 9),
	CR4_OSXMMEXCPT = (1 << 10),
	CR4_OSXSAVE = (1 << 18)
};

enum : uint32_t
{
	CPU_MSR_APIC_BASE = 0x1B,
	CPU_MSR_IA32_MTRRCAP = 0xFE,
	CPU_MSR_IA32_MTRR_PHYSBASE0 = 0x200,
	CPU_MSR_IA32_MTRR_PHYSMASK0 = 0x201,
	CPU_MSR_IA32_PAT = 0x277,
	CPU_MSR_FS_BASE = 0xC0000100,
	CPU_MSR_GS_BASE = 0xC0000101,
	CPU_MSR_KERNEL_GS_BASE = 0xC0000102
};

enum : uint32_t
{
	CPUID_PROCESSOR_INFO_EAX = 1,
	CPUID_PROCESSOR_INFOEX_EAX = 0x80000001,
	CPUID_MAX_ADDR = 0x80000008
};

enum : uint32_t
{
	CPUID_PROCESSOR_INFO_ECX_CMPXCHG16B = 1 << 13,
	CPUID_PROCESSOR_INFOEX_EDX_1GBPAGES = 1 << 26
};

enum : unsigned int
{
	BOOT_CPU_ID = 0,
	MAX_CPU = 255
};

enum : unsigned int
{
	CPU_IRQ0_VECTOR = 0xC0,
	CPU_APIC_SPURIOUS_VECTOR = 0xF0,
	CPU_LOCAL_TASK_SW_VECTOR = 0xF1,
	CPU_EXTERN_TASK_SW_VECTOR = 0xF2,
	CPU_STOP_VECTOR	= 0xF3,
	CPU_TLB_SHOOTDOWN_VECTOR = 0xF4
};

static inline uint64_t cpuGetCR0()
{
	uint64_t value;
	asm volatile("mov %0, CR0" : "=r"(value));
	return value;
}

static inline void cpuSetCR0(uint64_t value)
{
	asm volatile("mov CR0, %0"::"r"(value));
}

static inline uint64_t cpuGetCR3()
{
	uint64_t value;
	asm volatile("mov %0, CR3" : "=r"(value));
	return value;
}

static inline uint64_t cpuGetCR2()
{
	uint64_t value;
	asm volatile("mov %0, CR2" : "=r"(value));
	return value;
}

static inline void cpuSetCR3(uint64_t value)
{
	asm volatile("mov CR3, %0"::"r"(value));
}

static inline uint64_t cpuGetCR4()
{
	uint64_t value;
	asm volatile("mov %0, CR4" : "=r"(value));
	return value;
}

static inline void cpuSetCR4(uint64_t value)
{
	asm volatile("mov CR4, %0"::"r"(value));
}

static inline uint64_t cpuGetCR8()
{
	uint64_t value;
	asm volatile("mov %0, CR8" : "=r"(value));
	return value;
}

static inline void cpuSetCR8(uint64_t value)
{
	asm volatile("mov CR8, %0"::"r"(value));
}

static inline void cpuEnableInterrupts()
{
	asm volatile("sti");
}

static inline void cpuDisableInterrupts()
{
	asm volatile("cli");
}

static inline void cpuJumpToKernel(uintptr_t entry, uintptr_t stack, uintptr_t param)
{
	asm volatile("mov RSP, RAX\n"
			"call RBX\n"
			""::"a"(stack), "b"(entry), "c"(param));
}

static inline void cpuHalt()
{
	asm volatile("hlt");
}

template<typename CombineType, typename LoType, typename HiType>
inline bool cpuInterlockedCompareExchange128(volatile CombineType* src, LoType hi, HiType lo, CombineType* ref)
{
	static_assert(sizeof (CombineType) == 16);
	static_assert(sizeof (LoType) == 8);
	static_assert(sizeof (HiType) == 8);
	uint64_t* refInt = reinterpret_cast<uint64_t*>(ref);
	bool result;
	asm volatile("lock cmpxchg16b %1\n"
			"setz %0" : "=q"(result), "+m"(*src), "+a"(refInt[0]), "+d"(refInt[1]) : "b"(lo), "c"(hi) : "cc", "memory");
	return result;
}

inline void cpuCpuid(uint32_t &a, uint32_t& b, uint32_t& d, uint32_t& c)
{
	asm volatile("cpuid" : "+a"(a), "=b"(b), "=d"(d), "=c"(c) ::"cc");
}

static inline void cpuPause()
{
	asm volatile("pause");
}

static inline void cpuFlushTLB(uintptr_t addr)
{
	asm volatile("invlpg [%0]" ::"r" (addr) : "memory");
}

struct GdtDescripor;
struct IdtDescripor;

#pragma pack(push, 1)
struct CpuGdtPointer
{
	uint16_t m_limit;
	GdtDescripor* m_base;
};

struct CpuIdtPointer
{
	uint16_t m_limit;
	void* m_base;
};
#pragma pack(pop)

static inline void cpuLoadGDT(const CpuGdtPointer& gdtPointer)
{
	asm volatile("lgdt %0"::"m"(gdtPointer));
}

static inline void cpuLoadIDT(const CpuIdtPointer& idtPointer)
{
	asm volatile("lidt %0"::"m"(idtPointer));
}

static inline void cpuLoadIDT(const void* idtPointer)
{
	asm volatile("lidt [%0]"::"r"(idtPointer));
}

static inline void cpuLoadSS(uint16_t selector)
{
	asm volatile("mov SS, %0\n"
			"nop\n"::"r"(selector));
}

static inline void cpuInitFpu()
{
	asm volatile("fnclex\n"
			"finit\n");
}

static inline void cpuReadMSR(uint32_t index, uint32_t& lo, uint32_t& hi)
{
	asm volatile("rdmsr" : "=a" (lo), "=d"(hi) : "c" (index));
}

static inline uint64_t cpuReadMSR(uint32_t index)
{
	uint32_t lo;
	uint32_t hi;
	cpuReadMSR(index, lo, hi);
	return (static_cast<uint64_t> (hi) << 32) | static_cast<uint64_t> (lo);
}

static inline void cpuWriteMSR(uint32_t index, uint32_t lo, uint32_t hi)
{
	asm volatile("wrmsr" ::"a" (lo), "d" (hi), "c" (index));
}

static inline void cpuWriteMSR(uint32_t index, uint64_t value)
{
	const uint32_t lo = static_cast<uint32_t> (value & 0xFFFFFFFF);
	const uint32_t hi = static_cast<uint32_t> (value >> 32);
	cpuWriteMSR(index, lo, hi);
}

static inline void cpuLoadTaskRegister(uint16_t selector)
{
	asm volatile("ltr %0" ::"r"(selector));
}

template<typename T>
static inline T cpuLastOneBitIndex(T value)
{
	T index;
	asm volatile("bsr %0, %1" : "=r"(index) : "r"(value));
	return index;
}

static inline uint64_t cpuReadTSC()
{
	uint32_t lo;
	uint32_t hi;
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

template<typename AddrType>
static inline AddrType cpuAlignAddrHi(AddrType addr)
{
	return ((addr + PAGE_MASK) & ~PAGE_MASK);
}

static inline uintptr_t cpuAlignPtrHiInt(void* ptr)
{
	return ((reinterpret_cast<uintptr_t>(ptr) + PAGE_MASK) & ~PAGE_MASK);
}

template<typename AddrType>
static inline AddrType cpuAlignAddrLo(AddrType addr)
{
	return (addr & ~PAGE_MASK);
}

static inline uintptr_t cpuAlignPtrLoInt(void* ptr)
{
	return (reinterpret_cast<uintptr_t>(ptr) & ~PAGE_MASK);
}

static inline void outportb(uint16_t port, uint8_t value)
{
	asm volatile("out DX, AL" ::"d" (port), "a" (value));
}

static inline void outportw(uint16_t port, uint16_t value)
{
	asm volatile("out DX, AX" ::"d" (port), "a" (value));
}

static inline void outportd(uint16_t port, uint32_t value)
{
	asm volatile("out DX, EAX" ::"d" (port), "a" (value));
}

static inline uint8_t inportb(uint16_t port)
{
	uint8_t value;
	asm volatile("in AL, DX" : "=a" (value) : "d" (port));
	return value;
}

static inline uint16_t inportw(uint16_t port)
{
	uint16_t value;
	asm volatile("in AX, DX" : "=a" (value) : "d" (port));
	return value;
}

static inline uint32_t inportd(uint16_t port)
{
	uint32_t value;
	asm volatile("in EAX, DX" : "=a" (value) : "d" (port));
	return value;
}

static inline void cpuSaveFpuContext(void* context)
{
	asm volatile("fxsave [%0] "::"r"(context) : "memory");
}

static inline void cpuRestoreFpuContext(const void *context)
{
	asm volatile("fxrstor [%0] "::"r"(context) : "memory");
}

static inline uintptr_t cpuGetLocalData(uintptr_t offset)
{
	uintptr_t result;
	asm volatile ("movq %0, FS:[%1]" : "=r"(result) : "ri"(offset));
	return result;
}

static inline void cpuSetLocalData(uintptr_t offset, uintptr_t value)
{
	asm volatile ("movq FS:[%0], %1"::"ri"(offset), "ri"(value));
}

static inline uintptr_t threadGetLocalData(uintptr_t offset)
{
	uintptr_t result;
	asm volatile ("movq %0, GS:[%1]" : "=r"(result) : "ri"(offset));
	return result;
}

static inline void threadSetLocalData(uintptr_t offset, uintptr_t value)
{
	asm volatile ("movq GS:[%0], %1"::"ri"(offset), "ri"(value));
}

static inline uintptr_t cpuLocalAddExcahnge(uintptr_t offset, uintptr_t value)
{
	asm volatile ("xadd GS:[%1], %0" : "=r"(value) : "ri"(offset) : "cc");
	return value;
}

static inline uintptr_t cpuLocalSubExcahnge(uintptr_t offset, uintptr_t value)
{
	return cpuLocalAddExcahnge(offset, -value);
}

static inline void cpuLocalInc(uintptr_t offset)
{
	asm volatile ("incq FS:[%0]"::"ri"(offset) : "cc");
}

static inline void cpuDecLocalData(uintptr_t offset)
{
	asm volatile ("dec FS:[%0]"::"ri"(offset) : "cc");
}

static inline void cpuInvalidateCache()
{
	asm volatile("wbinvd");
}

template<unsigned int vector>
static inline void cpuCallInterrupt()
{
	asm volatile("int %0"::"i"(vector));
}

uint64_t cpuGetFlagsRegister();
void cpuFullFlushTLB(uintptr_t addr);
void cpuLoadCS(uint16_t selector);
void cpuStop();
void cpuSetTsFlag();
uintptr_t cpuMaxPhysAddr();

void dumpControlRegisters();
void cpuTestCommands();
bool cpuSupport1GbPages();
void fixFrameBufferMtrr();

void cpuSetTsFlag();
void cpuClearTsFlag();

unsigned int cpuCurrentId();
unsigned int cpuLogicalCount();


extern "C" void cpuFastEio();

struct CpuMtrrItem
{
	uintptr_t m_addr;
	uintptr_t m_mask;
};

kvector<CpuMtrrItem> cpuStoreMtrr();
void cpuLoadMtrr(const kvector<CpuMtrrItem>& mtrr);

static inline void cpuSetLocalPtr(uintptr_t offset, const void* ptr)
{
	cpuSetLocalData(offset, reinterpret_cast<uintptr_t>(ptr));
}

static inline void* cpuGetLocalPtr(uintptr_t offset)
{
	return reinterpret_cast<void*>(cpuGetLocalData(offset));
}

static inline void threadSetLocalPtr(uintptr_t offset, const void* ptr)
{
	threadSetLocalData(offset, reinterpret_cast<uintptr_t>(ptr));
}

static inline void* threadGetLocalPtr(uintptr_t offset)
{
	return reinterpret_cast<void*>(threadGetLocalData(offset));
}

static inline void cpuSetDefaultControlRegisters()
{
	cpuSetCR0((cpuGetCR0() & ~(CR0_CACHE_DISABLE | CR0_NOT_WRITE_TROUGHT | CR0_EMULATION)) | CR0_WRITE_PROTECT | CR0_NUMERIC_ERROR);
	cpuSetCR4(cpuGetCR4() | CR4_PAGE_GLOBAL_ENABLE | CR4_OSFXSR | CR4_OSXMMEXCPT | CR4_OSXSAVE);
}

class CpuInterruptLock
{
public:
	CpuInterruptLock()
	{
		cpuDisableInterrupts();
	}

	~CpuInterruptLock()
	{
		cpuEnableInterrupts();
	}
};

class CpuInterruptLockSave
{
public:
	CpuInterruptLockSave()
	{
		if (m_needLock)
			cpuDisableInterrupts();
	}

	~CpuInterruptLockSave()
	{
		if (m_needLock)
			cpuEnableInterrupts();
	}
	
private:
	const bool m_needLock = ((cpuGetFlagsRegister() & 0x200) != 0);
};


#pragma pack(push, 1)
struct InterruptFrame
{
	uint64_t m_rip;
	uint64_t m_cs;
	uint64_t m_rflags;
	uint64_t m_rsp;
	uint64_t m_ss;
};

struct InterruptVolatileState
{
	struct
	{
		uint64_t m_r11;
		uint64_t m_r10;
		uint64_t m_r9;
		uint64_t m_r8;
		uint64_t m_rdx;
		uint64_t m_rcx;
		uint64_t m_rax;
	} m_regs;

	union
	{
		struct
		{
			uint64_t m_errorCode;
			InterruptFrame m_frame;
		} m_frameEC;
		InterruptFrame m_frame;
	};
	uint64_t m_padding;
};

struct InterruptFullState
{
	struct
	{
		uint64_t m_r15;
		uint64_t m_r14;
		uint64_t m_r13;
		uint64_t m_r12;
		uint64_t m_rsi;
		uint64_t m_rdi;
		uint64_t m_rbp;
		uint64_t m_rbx;
	} m_nonvolatileRegs;
	InterruptVolatileState m_volatile;
};
#pragma pack(pop)
static_assert(sizeof (InterruptVolatileState) % 16 == 0);
static_assert(sizeof (InterruptFullState) % 16 == 0);

#define INTERRUPT_SAVE_VOLATILE_REGS \
    "push RAX\n"\
    "push RCX\n"\
    "push RDX\n"\
    "push R8\n"\
    "push R9\n"\
    "push R10\n"\
    "push R11\n"

#define INTERRUPT_RESTORE_VOLATILE_REGS \
    "pop R11\n"\
    "pop R10\n"\
    "pop R9\n"\
    "pop R8\n"\
    "pop RDX\n"\
    "pop RCX\n"\
    "pop RAX\n"

#define INTERRUPT_SAVE_NONVOLATILE_REGS \
    "push RBX\n"\
    "push RBP\n"\
    "push RDI\n"\
    "push RSI\n"\
    "push R12\n"\
    "push R13\n"\
    "push R14\n"\
    "push R15\n"

#define INTERRUPT_RESTORE_NONVOLATILE_REGS \
    "pop R15\n"\
    "pop R14\n"\
    "pop R13\n"\
    "pop R12\n"\
    "pop RSI\n"\
    "pop RDI\n"\
    "pop RBP\n"\
    "pop RBX\n"

#define INTERRUPT_HANDLER(procName, retRoutine)\
    asm volatile(   ".globl " #procName "\n"\
                    #procName ":\n"\
                    INTERRUPT_SAVE_VOLATILE_REGS \
                    "mov RCX, RSP\n"\
                    "call _"#procName "\n"\
                    INTERRUPT_RESTORE_VOLATILE_REGS \
                    retRoutine"\n"\
                    "iretq\n");\
    extern "C" void procName();\
    extern "C" void _##procName(InterruptVolatileState* state)

#define EXCEPTION_HANDLER(procName)\
    INTERRUPT_HANDLER(procName, "")

#define EXCEPTION_HANDLER_EC(procName)\
    INTERRUPT_HANDLER(procName, "ADD RSP, 8")

#define IRQ_HANDLER_BEGIN_ASM_ROUTINE\
    "call tackManagerBeginInterrupt\n"\
    "sti\n"

#define IRQ_HANDLER_END_ASM_ROUTINE\
    "1:\n"\
    "mov RCX, RSP\n"\
    "call tackManagerEndInterrupt\n"\
    "test RAX, RAX\n"\
    "jz 2f\n"\
    INTERRUPT_SAVE_NONVOLATILE_REGS\
    "mov RSP, RAX\n"\
    "call tackManagerEndTaskSwitch\n"\
    INTERRUPT_RESTORE_NONVOLATILE_REGS\
    "test RAX, RAX\n"\
    "jz 1b \n"\
    "2:\n"

