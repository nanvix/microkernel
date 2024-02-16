/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef ARCH_X86_CPU_MDAT_H_
#define ARCH_X86_CPU_MDAT_H_

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#ifndef _ASM_FILE_
#include <arch/x86/cpu/acpi.h>
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/*============================================================================*
 * Structures                                                                 *
 *============================================================================*/

#ifndef _ASM_FILE_

/**
 * @brief Multiple APIC Description Table (MADT).
 */
struct madt_t {
    struct acpi_sdt_header h;
    uint32_t local_apic_addr;
    /* 1 = Dual 8259 Legacy PICs Installed */
    uint32_t flags;
    uint32_t entries[];
};

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @brief Parses the Multiple APIC Description Table (MADT).
 *
 * @param madt Target MADT.
 *
 * @returns Upon successful completion, 0 is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int parse_madt(struct madt_t *madt);

#endif /* _ASM_FILE_ */

/*============================================================================*/

/**@}*/

#endif /* ARCH_X86_CPU_MDAT_H_ */
