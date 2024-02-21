/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef ARCH_X86_CPU_MSR_H_
#define ARCH_X86_CPU_MSR_H_

/**
 * @addtogroup x86-cpu-msr x86 MSR
 * @ingroup x86
 *
 * @brief x86 Model-Specific Register (MSR)
 */
/**@{*/

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#ifndef _ASM_FILE_
#include <nanvix/cc.h>
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @brief APIC base register.
 *
 * @details This register holds the APIC base address, permitting the relocation
 * of the APIC memory map.
 */
#define MSR_IA32_APIC_BASE 0x0000001b

/**
 * @addtogroup x86-cpu-msr-ia32-apic-base IA32_APIC_BASE
 * @ingroup x86-cpu-msr
 */
/**@{*/
#define MSR_IA32_APIC_BASE_BSP (1 << 8)            /** BSP Flag (R/W)      */
#define MSR_IA32_APIC_BASE_X2APIC (1 << 10)        /** x2APIC Enable (R/W) */
#define MSR_IA32_APIC_BASE_ENABLE (1 << 11)        /** APIC Enable (R/W)   */
#define MSR_IA32_APIC_BASE_BASE 0x7ffffffffffff000 /** APIC Base Address   */
/**@}*/

/*============================================================================*
 * Functions                                                                  *
 *============================================================================*/

#ifndef _ASM_FILE_

/**
 * @brief Reads a model specific register.
 *
 * @param msr Model specific register to read.
 *
 * @return Returns the contents of the model specific register @p msr.
 */
static inline uint64_t rdmsr(const uint32_t msr)
{
    uint32_t eax = 0, edx = 0;

    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));

    return (((uint64_t)edx << 32) | eax);
}

/**
 * @brief Writes to a model specific register.
 *
 * @param msr Model specific register to write.
 * @param value Value to write.
 */
static inline void wrmsr(const uint32_t msr, const uint64_t value)
{
    const uint32_t eax = value & 0xffffffff;
    const uint32_t edx = value >> 32;

    asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr));
}

#endif /* !_ASM_FILE_ */

/*============================================================================*/

/**@}*/

#endif /* !ARCH_X86_CPU_MSR_H_ */
