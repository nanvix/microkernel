/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/errno.h>
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/log.h>
#include <nanvix/libcore.h>

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/**
 * @brief Checks if the checksum of a given ACPI table is valid.
 *
 * @param start Start address of the ACPI table.
 * @param len  Length of the ACPI table.
 *
 * @return Upon successful completion, non-zero is returned. Upon failure, zero
 * is returned instead.
 */
static int do_checksum(const char *start, size_t len)
{
    char sum = 0;

    for (size_t i = 0; i < len; i++) {
        sum += start[i];
    }

    return (sum == 0);
}

/**
 * @brief Finds an APIC table by its signature.
 *
 * @param rsdt Root System Description Table.
 * @param sig Signature of the table.
 *
 * @return Upon successful completion, a pointer to the table is returned.
 * Upon failure, NULL is returned instead.
 */
static void *find_table_by_sig(const struct rsdt_t *rsdt, char *sig)
{
    int entries;

    /* Determine which SDT is being used */
    entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;

    kprintf("entries: %d", entries);

    for (int i = 0; i < entries; i++) {
        struct acpi_sdt_header *h = (struct acpi_sdt_header *)(rsdt->others[i]);

        // Print signature.
        char signature[5];
        __memcpy(signature, h->signature, 4);
        signature[4] = '\0';
        log(INFO, "Signature Found: %s", signature);

        // Check signature.
        if (!__strncmp(h->signature, sig, 4)) {
            // Validate checksum.
            if (!do_checksum((char *)h, h->length)) {
                continue;
            }

            return (void *)h;
        }
    }

    // Table not found.
    return (NULL);
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @brief Parses ACPI info.
 *
 * @param info ACPI info.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
int acpi_info_parse(const void *info)
{
    struct acpi_sdt_header *h = NULL;
    struct rsdt_t *rsdt = NULL;
    struct rsdp_t *rsdp = (struct rsdp_t *)info;

    // Validate RSDP.
    if (!do_checksum((char *)rsdp, sizeof(*rsdp))) {
        log(ERROR, "RSDP checksum invalid");
        return (-EINVAL);
    }

    // Print RSDP info
    char signature[9];
    __memcpy(signature, rsdp->signature, 8);
    signature[8] = '\0';
    log(INFO, "RSDP: %s", signature);
    char oemid[7];
    __memcpy(oemid, rsdp->oemid, 6);
    oemid[6] = '\0';
    log(INFO, "OEMID: %s", oemid);
    log(INFO, "Revision: %d", rsdp->revision);
    log(INFO, "RSDT Address: %x", rsdp->rsdt_addr);

    // Get RSDT.
    if (rsdp->revision == 0) {
        // ACPI version 1.0.
        log(INFO, "ACPI version 1.0");
        h = (struct acpi_sdt_header *)(rsdp->rsdt_addr);
        rsdt = (struct rsdt_t *)h;
    } else {
        // ACPI version >= 2.0.
        log(ERROR, "ACPI version >= 2.0 not supported");
        return (-EINVAL);
    }

    // Validate SDT.
    if (!do_checksum((char *)h, h->length)) {
        log(ERROR, "SDT checksum invalid");
        return (-EINVAL);
    }

    // Print SDT header info.
    __memcpy(signature, h->signature, 4);
    signature[4] = '\0';
    log(INFO, "SDT: %s", signature);
    log(INFO, "Length: %d", h->length);
    log(INFO, "Revision: %d", h->revision);
    log(INFO, "OEMID: %s", h->oem_id);
    log(INFO, "OEM Table ID: %s", h->oem_table_id);
    log(INFO, "OEM Revision: %d", h->oem_revision);
    log(INFO, "Creator ID: %d", h->creator_id);
    log(INFO, "Creator Revision: %d", h->creator_rev);
    log(INFO, "SDT Address: %x", rsdt);

    struct madt_t *madt = (struct madt_t *)find_table_by_sig(rsdt, "APIC");
    if (madt == NULL) {
        kpanic("MADT not found");
    }
    madt = (struct madt_t *)(madt);

    parse_madt(madt);

    return (0);
}
