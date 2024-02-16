/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/log.h>
#include <stddef.h>
#include <stdint.h>

/*============================================================================*
 * Structures                                                                 *
 *============================================================================*/

/**
 * @brief MADT entry types.
 */
enum MADT_ENTRY_TYPE {
    MADT_ENTRY_TYPE_LOCAL_APIC = 0,
    MADT_ENTRY_TYPE_IO_APIC = 1,
    MADT_ENTRY_TYPE_SOURCE_OVERRIDE = 2,
    MADT_ENTRY_TYPE_IO_APIC_NMI = 3,
    MADT_ENTRY_TYPE_LOCAL_APIC_NMI = 4,
    MADT_ENTRY_TYPE_LOCAL_X2APIC = 9,
};

/**
 * @brief MADT entry header.
 */
struct madt_entry_header {
    uint8_t entry_type;
    /* length include this header + body */
    uint8_t entry_len;
};

/**
 * @brief Processor Local APIC.
 *
 * @details MADT type 0. It represents a single physical processor and its
 * local interrupt controller.
 */
struct madt_entry_processor_local_apic {
    struct madt_entry_header h;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    /* bit 0 = Processor Enabled, bit 1 = Online Capable */
    uint32_t flags;
};

/**
 * @brief I/O APIC.
 *
 * @details MADT type 1. This type represents a I/O APIC. The global system
 * interrupt base is the first interrupt number that this I/O APIC handles.
 */
struct madt_entry_io_apic {
    struct madt_entry_header h;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_addr;
    /* First first IRQ that this IOAPIC handles */
    uint32_t global_system_interrupt_base;
};

/**
 * @brief I/O APIC Interrupt Source Override.
 *
 * @details MADT type 2. This type explains how IRQ sources are mapped to
 * global system interrupts.
 */
struct madt_interrupt_source_override {
    struct madt_entry_header h;
    uint8_t bus_source;
    uint8_t irq_source;
    /*
     * Look for I/O APIC with base below this number, then make redirection
     * entry (interrupt - base) to be the interrupt.
     */
    uint32_t global_system_interrupt;
    uint16_t flags;
};

/**
 * @brief I/O APIC NMI.
 *
 * @details MADT type 3. This type represents a non-maskable interrupt (NMI)
 * source that is connected to an I/O APIC.
 */
struct madt_io_apic_nmi {
    struct madt_entry_header h;
    uint8_t io_apic_id;
    uint16_t flags;
    uint32_t global_system_interrupt;
};

/**
 * @brief Local APIC NMI.
 *
 * @details MADT type 4. This type represents a non-maskable interrupt (NMI)
 * source that is connected to the local APIC.
 */
struct madt_nonmaskable_interrupts {
    struct madt_entry_header h;
    uint8_t acpi_processor_id; // 0xFF means all processors
    uint16_t flags;
    uint8_t lint_no; // 0 or 1
};

/**
 * @brief Local APIC Address Override.
 *
 * @details MADT type 5. This type provides 64 bit systems with an override of
 * the physical address of the Local APIC. If this structure is defined, the
 * 64-bit Local APIC address stored within it should be used instead of the
 * 32-bit Local APIC address stored in the MADT header.
 */
struct madt_local_apic_addr_override {
    struct madt_entry_header h;
    uint16_t reserved;
    uint64_t local_apic_phys_addr;
};

/**
 * @brief Processor Local x2APIC.
 *
 * @details MADT type 9. This type represents a single physical processor and
 * its local interrupt controller.
 */
struct madt_entry_processor_local_x2apic {
    struct madt_entry_header h;
    uint16_t reserved;
    uint32_t acpi_processor_id;
    /* bit 0 = Processor Enabled, bit 1 = Online Capable */
    uint32_t flags;
    uint32_t apic_id;
};

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/**
 * @brief Parses a MADT local APIC entry.
 *
 * @param local_apic MADT local APIC entry.
 */
static void madt_entry_processor_local_apic_parse(
    struct madt_entry_processor_local_apic *local_apic)
{
    KASSERT(local_apic != NULL);

    log(INFO,
        "ProcessorID [%d], APIC ID[%d], "
        "flags[%d]",
        local_apic->acpi_processor_id,
        local_apic->apic_id,
        local_apic->flags);
}

/**
 * @brief Parses a MADT local x2APIC entry.
 *
 * @param local_x2apic MADT local x2APIC entry.
 */
static void madt_entry_processor_local_x2apic_parse(
    struct madt_entry_processor_local_x2apic *local_x2apic)
{
    KASSERT(local_x2apic != NULL);

    log(INFO,
        "ProcessorID [%d], APIC ID[%d], "
        "flags[%d]",
        local_x2apic->acpi_processor_id,
        local_x2apic->apic_id,
        local_x2apic->flags);
}

/**
 * @brief Parses a MADT I/O APIC entry.
 *
 * @param io_apic MADT I/O APIC entry.
 */
static void madt_entry_io_apic_parse(struct madt_entry_io_apic *io_apic)
{
    KASSERT(io_apic != NULL);

    log(INFO,
        "IO APIC ID[%d], "
        "IO APIC Address[%x], "
        "Global System Interrupt Base[%d]",
        io_apic->io_apic_id,
        io_apic->io_apic_addr,
        io_apic->global_system_interrupt_base);
}

/**
 * @brief Parses a MADT I/O APIC interrupt source override entry.
 *
 * @param source_override MADT I/O APIC interrupt source override entry.
 */
static void madt_interrupt_source_override_parse(
    struct madt_interrupt_source_override *source_override)
{
    KASSERT(source_override != NULL);

    uint8_t src = source_override->irq_source;
    uint32_t mapped = source_override->global_system_interrupt;
    log(INFO,
        "Bus Source[%d], IRQ Source[%d], "
        "Global System Interrupt[%d]",
        source_override->bus_source,
        src,
        mapped);
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Parses the MADT table.
 */
int parse_madt(struct madt_t *madt)
{
    KASSERT(madt != NULL);

    struct madt_entry_header *entry = (struct madt_entry_header *)madt->entries;
    uint32_t madt_len = madt->h.length;
    log(INFO,
        "local_apic_addr=%lx, entry=%lx, madt=%lx, madt_len=%d",
        madt->local_apic_addr,
        ((char *)entry),
        ((char *)madt),
        madt_len);

    while ((char *)entry + entry->entry_len <= (char *)madt + madt_len) {
        log(INFO,
            "entry->entry_type=%d, entry->entry_len=%d",
            entry->entry_type,
            entry->entry_len);
        if (entry->entry_len == 0)
            break;

        switch (entry->entry_type) {
            case MADT_ENTRY_TYPE_LOCAL_APIC: {
                madt_entry_processor_local_apic_parse(
                    (struct madt_entry_processor_local_apic *)entry);
            } break;

            case MADT_ENTRY_TYPE_LOCAL_X2APIC: {
                madt_entry_processor_local_x2apic_parse(
                    (struct madt_entry_processor_local_x2apic *)entry);
            } break;

            case MADT_ENTRY_TYPE_IO_APIC: {
                madt_entry_io_apic_parse((struct madt_entry_io_apic *)entry);
            } break;

            case MADT_ENTRY_TYPE_SOURCE_OVERRIDE: {
                madt_interrupt_source_override_parse(
                    (struct madt_interrupt_source_override *)entry);
                break;
            }

            default:
                break;
        }
        entry = (struct madt_entry_header *)((char *)entry + entry->entry_len);
    }

    return (0);
}
