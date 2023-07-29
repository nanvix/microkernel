/*
 * Copyright(c) 2011-2023 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_HAL_CPU_H_
#define NANVIX_KERNEL_HAL_CPU_H_

#include <nanvix/kernel/hal/arch.h>
#include <nanvix/kernel/hal/cpu/context.h>
#include <nanvix/kernel/hal/cpu/exception.h>

#ifndef _ASM_FILE_

/**
 * @brief Initializes the CPU.
 */
extern void cpu_init(void);

/**
 * @brief Disables interrupts.
 *
 * @todo Implement this function (TODO).
 */
static inline void disable_interrupts(void)
{
    /* NOOP */
}

#endif /* !_ASM_FILE_ */

#endif /* NANVIX_KERNEL_HAL_CPU_H_ */
