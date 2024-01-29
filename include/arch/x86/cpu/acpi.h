/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef ARCH_X86_CPU_ACPI_H_
#define ARCH_X86_CPU_ACPI_H_

/**
 * @addtogroup x86-cpu-acpi ACPI
 * @ingroup x86
 *
 * @brief Advanced Control Power Interface (ACPI)
 */
/**@{*/

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#ifndef _ASM_FILE_
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/*============================================================================*
 * Structures                                                                 *
 *============================================================================*/

#ifndef _ASM_FILE_

/**
 * @brief System Description Table (SDT) header.
 */
struct acpi_sdt_header {
    char signature[4];     /** ASCII string representation of the table ID.  */
    uint32_t length;       /** Length of the table.                          */
    uint8_t revision;      /** Revision of the structure.                    */
    uint8_t checksum;      /** Checksum of the table.                        */
    char oem_id[6];        /** ASCII string that identifies the OEM.         */
    char oem_table_id[8];  /** ASCII string that identifies the OEM table.   */
    uint32_t oem_revision; /** OEM revision number.                          */
    uint32_t creator_id;   /** Vendor ID of the util that created the table. */
    uint32_t creator_rev;  /** Revision of the util that created the table.  */
} __attribute__((packed));

/**
 * @brief Root System Description Table (RSDT).
 */
struct rsdt_t {
    struct acpi_sdt_header h; /** SDT header. */
    uint32_t others[];        /** Other SDTs. */
} __attribute__((packed));

/**
 * @brief Root System Description Pointer (RSDP).
 */
struct rsdp_t {
    uint8_t signature[8]; /** ASCII string representation of the signature. */
    uint8_t checksum;     /** Checksum of the table.                        */
    uint8_t oemid[6];     /** ASCII string that identifies the OEM.         */
    uint8_t revision;     /** Revision of the structure.                    */
    uint32_t rsdt_addr;   /** Address of the RSDT.                          */
} __attribute__((packed));

#endif /* !_ASM_FILE_ */

/*============================================================================*/

/**@}*/

#endif /* !ARCH_X86_CPU_ACPI_H_ */
