/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/hal.h>
#include <stdint.h>

/**
 * @brief Initializes the local xAPIC.
 *
 */
extern void xapic_init(void);

void apic_init(void)
{
    if (is_x2apic_supported()) {
        kpanic("x2apic detected\n");

    } else if (is_apic_supported()) {
        kprintf("xapic detected\n");
        xapic_init();

    } else {
        kprintf("not apic detected\n");
    }

    while (1)
        ;
}
