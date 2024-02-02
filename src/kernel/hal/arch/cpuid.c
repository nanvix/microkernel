/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <arch/arch.h>
#include <stdint.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * Gets the vendor string of the CPU.
 */
void cpu_get_vendor(char vendor[13])
{
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpuid(CPUID_GET_VENDOR, &eax, &ebx, &ecx, &edx);
    *(uint32_t *)&vendor[0] = ebx;
    *(uint32_t *)&vendor[4] = edx;
    *(uint32_t *)&vendor[8] = ecx;
    vendor[12] = '\0';
}
