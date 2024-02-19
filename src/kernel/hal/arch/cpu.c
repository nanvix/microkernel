/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/config.h>
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Initializes the CPU.
 */
void cpu_init(void)
{
    kprintf("[hal] initializing cpu...");

    // Ensure that size of types match what we expect.
    KASSERT_SIZE(BYTE_BIT, 8 * BYTE_SIZE);
    KASSERT_SIZE(HWORD_BIT, 8 * HWORD_SIZE);
    KASSERT_SIZE(WORD_BIT, 8 * WORD_SIZE);
    KASSERT_SIZE(DWORD_BIT, 8 * DWORD_SIZE);
    KASSERT_SIZE(sizeof(byte_t), BYTE_SIZE);
    KASSERT_SIZE(sizeof(hword_t), HWORD_SIZE);
    KASSERT_SIZE(sizeof(word_t), WORD_SIZE);
    KASSERT_SIZE(sizeof(dword_t), DWORD_SIZE);

    kprintf("[hal] has cpuid? %s", __has_cpuid() ? "yes" : "no");

    gdt_init();
    const unsigned kernel_cs = gdt_kernel_cs();
    const unsigned hwint_off = idt_init(kernel_cs);
    lpic_init(hwint_off);
    timer_init(KERNEL_TIMER_FREQUENCY);
}
