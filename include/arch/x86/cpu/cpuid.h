/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef ARCH_X86_CPU_CPUID_H_
#define ARCH_X86_CPU_CPUID_H_

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#ifndef _ASM_FILE_
#include <nanvix/cc.h>
#include <stdbool.h>
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @brief Operations for CPUID instruction.
 */
/**@{*/
#define CPUID_GET_VENDOR 0x00   /** Get vendor. */
#define CPUID_GET_FEATURES 0x01 /** Get features.  */
#define CPUID_GET_CACHE 0x02    /** Get cache.     */
#define CPUID_GET_SERIAL 0x03   /** Get serial.    */
/**@}*/

/**
 * @brief CPUID Feature EDX Flags
 */
/**@{*/
#define CPUID_ECX_SSE3 (1 << 0)     /* Streaming SIMD Extensions 3         */
#define CPUID_ECX_PCLMUL (1 << 1)   /* Carry-less Multiplication           */
#define CPUID_ECX_DTES64 (1 << 2)   /* 64-bit DS Area                      */
#define CPUID_ECX_MONITOR (1 << 3)  /* MONITOR/MWAIT                       */
#define CPUID_ECX_DS_CPL (1 << 4)   /* CPL Qualified Debug Store           */
#define CPUID_ECX_VMX (1 << 5)      /* Virtual Machine Extensions          */
#define CPUID_ECX_SMX (1 << 6)      /* Safer Mode Extensions               */
#define CPUID_ECX_EST (1 << 7)      /* Enhanced Intel SpeedStep Technology */
#define CPUID_ECX_TM2 (1 << 8)      /* Thermal Monitor 2                   */
#define CPUID_ECX_SSSE3 (1 << 9)    /* Supplemental SIMD Extensions 3      */
#define CPUID_ECX_CID (1 << 10)     /* L1 Context ID                       */
#define CPUID_ECX_SDBG (1 << 11)    /* Silicon Debug                       */
#define CPUID_ECX_FMA (1 << 12)     /* Fused Multiply Add                  */
#define CPUID_ECX_CX16 (1 << 13)    /* CMPXCHG16B                          */
#define CPUID_ECX_XTPR (1 << 14)    /* xTPR Update Control                 */
#define CPUID_ECX_PDCM (1 << 15)    /* Perf/Debug Capability MSR           */
#define CPUID_ECX_PCID (1 << 17)    /* Process-context identifiers         */
#define CPUID_ECX_DCA (1 << 18)     /* Direct Cache Access                 */
#define CPUID_ECX_SSE4_1 (1 << 19)  /* Streaming SIMD Extensions 4.1       */
#define CPUID_ECX_SSE4_2 (1 << 20)  /* Streaming SIMD Extensions 4.2       */
#define CPUID_ECX_X2APIC (1 << 21)  /* x2APIC                              */
#define CPUID_ECX_MOVBE (1 << 22)   /* MOVBE Instruction                   */
#define CPUID_ECX_POPCNT (1 << 23)  /* POPCNT Instruction                  */
#define CPUID_ECX_TSC (1 << 24)     /* Time Stamp Counter                  */
#define CPUID_ECX_AES (1 << 25)     /* AES Instruction Set                 */
#define CPUID_ECX_XSAVE (1 << 26)   /* XSAVE/XRSTOR/XSETBV/XGETBV          */
#define CPUID_ECX_OSXSAVE (1 << 27) /* XSAVE enabled by OS                 */
#define CPUID_ECX_AVX (1 << 28)     /* Advanced Vector Extensions          */
#define CPUID_ECX_F16C (1 << 29)    /* 16-bit FP conversion instructions   */
#define CPUID_ECX_RDRAND (1 << 30)  /* RDRAND Instruction                  */
/**@}*/

/**
 * @brief CPUID Feature EDX Flags
 */
/**@{*/
#define CPUID_EDX_FPU (1 << 0)      /* On-chip Floating-Point Unit      */
#define CPUID_EDX_VME (1 << 1)      /* Virtual 8086 Mode Enhancements   */
#define CPUID_EDX_DE (1 << 2)       /* Debugging Extensions             */
#define CPUID_EDX_PSE (1 << 3)      /* Page Size Extensions             */
#define CPUID_EDX_TSC (1 << 4)      /* Time Stamp Counter               */
#define CPUID_EDX_MSR (1 << 5)      /* Model Specific Registers         */
#define CPUID_EDX_PAE (1 << 6)      /* Physical Address Extension       */
#define CPUID_EDX_MCE (1 << 7)      /* Machine Check Exception          */
#define CPUID_EDX_CX8 (1 << 8)      /* CMPXCHG8 Instruction             */
#define CPUID_EDX_APIC (1 << 9)     /* On-chip APIC                     */
#define CPUID_EDX_SEP (1 << 11)     /* SYSENTER/SYSEXIT                 */
#define CPUID_EDX_MTRR (1 << 12)    /* Memory Type Range Registers      */
#define CPUID_EDX_PGE (1 << 13)     /* Page Global Enable               */
#define CPUID_EDX_MCA (1 << 14)     /* Machine Check Architecture       */
#define CPUID_EDX_CMOV (1 << 15)    /* Conditional Move Instructions    */
#define CPUID_EDX_PAT (1 << 16)     /* Page Attribute Table             */
#define CPUID_EDX_PSE36 (1 << 17)   /* 36-bit Page Size Extension       */
#define CPUID_EDX_PSN (1 << 18)     /* Processor Serial Number          */
#define CPUID_EDX_CLFLUSH (1 << 19) /* CLFLUSH Instruction              */
#define CPUID_EDX_DS (1 << 21)      /* Debug Store                      */
#define CPUID_EDX_ACPI (1 << 22)    /* Advanced Control Power Interface */
#define CPUID_EDX_MMX (1 << 23)     /* MMX Technology                   */
#define CPUID_EDX_FXSR (1 << 24)    /* FXSAVE and FXRSTOR Instructions  */
#define CPUID_EDX_SSE (1 << 25)     /* Streaming SIMD Extensions        */
#define CPUID_EDX_SSE2 (1 << 26)    /* Streaming SIMD Extensions 2      */
#define CPUID_EDX_SS (1 << 27)      /* Self Snoop                       */
#define CPUID_EDX_HTT (1 << 28)     /* Hyper-Threading Technology       */
#define CPUID_EDX_TM (1 << 29)      /* Thermal Monitor                  */
#define CPUID_EDX_IA64 (1 << 30)    /* IA64 Instructions                */
#define CPUID_EDX_PBE (1 << 31)     /* Pending Break Enable             */
/**@}*/

/*============================================================================*
 * Functions                                                                  *
 *============================================================================*/

#ifndef _ASM_FILE_

/**
 * @brief Issues the CPUID instructions.
 *
 * @param op Operation.
 * @param eax Storage location for value read from EAX.
 * @param ebx Storage location for value read from EBX.
 * @param ecx Storage location for value read from ECX.
 * @param edx Storage location for value read from EDX.
 */
static inline void cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx)
{
    *eax = op;
    *ecx = 0;
    __asm__ __volatile__("cpuid"
                         : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                         : "a"(*eax), "c"(*ecx)
                         : "memory");
}

/**
 * @brief Gets the vendor string of the CPU.
 *
 * @param vendor Storage location for vendor string.
 */
extern void cpu_get_vendor(char vendor[13]);

static inline bool is_sse3_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SSE3;
}

static inline bool is_pclmul_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_PCLMUL;
}

static inline bool is_dtes64_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_DTES64;
}

static inline bool is_monitor_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_MONITOR;
}

static inline bool is_ds_cpl_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_DS_CPL;
}

static inline bool is_vmx_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_VMX;
}

static inline bool is_smx_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SMX;
}

static inline bool is_est_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_EST;
}

static inline bool is_tm2_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_TM2;
}

static inline bool is_ssse3_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SSSE3;
}

static inline bool is_cid_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_CID;
}

static inline bool is_sdbg_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SDBG;
}

static inline bool is_fma_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_FMA;
}

static inline bool is_cx16_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_CX16;
}

static inline bool is_xtpr_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_XTPR;
}

static inline bool is_pdcm_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_PDCM;
}

static inline bool is_pcid_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_PCID;
}

static inline bool is_dca_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_DCA;
}

static inline bool is_sse4_1_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SSE4_1;
}

static inline bool is_sse4_2_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_SSE4_2;
}

static inline bool is_x2apic_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_X2APIC;
}

static inline bool is_movbe_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_MOVBE;
}

static inline bool is_popcnt_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_POPCNT;
}

static inline bool is_tsc_deadline_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_TSC;
}

static inline bool is_aes_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_AES;
}

static inline bool is_xsave_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_XSAVE;
}

static inline bool is_osxsave_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_OSXSAVE;
}

static inline bool is_avx_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_AVX;
}

static inline bool is_f16c_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_F16C;
}

static inline bool is_rdrand_supported()
{
    int ecx;
    asm volatile("cpuid"
                 : "=c"(ecx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%edx");
    return ecx & CPUID_ECX_RDRAND;
}

static inline bool is_fpu_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_FPU;
}

static inline bool is_vme_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_VME;
}

static inline bool is_de_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_DE;
}

static inline bool is_pse_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PSE;
}

static inline bool is_tsc_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_TSC;
}

static inline bool is_msr_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_MSR;
}

static inline bool is_pae_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PAE;
}

static inline bool is_mce_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_MCE;
}

static inline bool is_cx8_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_CX8;
}

static inline bool is_apic_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_APIC;
}

static inline bool is_sep_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_SEP;
}

static inline bool is_mtrr_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_MTRR;
}

static inline bool is_pge_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PGE;
}

static inline bool is_mca_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_MCA;
}

static inline bool is_cmov_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_CMOV;
}

static inline bool is_pat_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PAT;
}

static inline bool is_pse36_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PSE36;
}

static inline bool is_psn_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PSN;
}

static inline bool is_clflush_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_CLFLUSH;
}

static inline bool is_ds_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_DS;
}

static inline bool is_acpi_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_ACPI;
}

static inline bool is_mmx_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_MMX;
}

static inline bool is_fxsr_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_FXSR;
}

static inline bool is_sse_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_SSE;
}

static inline bool is_sse2_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_SSE2;
}

static inline bool is_ss_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_SS;
}

static inline bool is_htt_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_HTT;
}

static inline bool is_tm_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_TM;
}

static inline bool is_ia64_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_IA64;
}

static inline bool is_pbe_supported()
{
    int edx;
    asm volatile("cpuid"
                 : "=d"(edx)
                 : "a"(CPUID_GET_FEATURES)
                 : "%ebx", "%ecx");
    return edx & CPUID_EDX_PBE;
}

#endif /* !_ASM_FILE_ */

/*============================================================================*/

#endif /* !ARCH_X86_CPU_CPUID_H_ */
