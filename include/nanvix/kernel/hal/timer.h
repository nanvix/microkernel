/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_HAL_TIMER_H_
#define NANVIX_KERNEL_HAL_TIMER_H_

/**
 * @addtogroup hal-timer HAL TImer
 * @ingroup hal
 *
 * @brief HAL Timer
 */
/**@{*/

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#ifndef _ASM_FILE_
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/*============================================================================*
 * Functions                                                                  *
 *============================================================================*/

#ifndef _ASM_FILE_

/**
 * @brief Initializes the timer device.
 *
 * @param freq Target frequency for the timer device.
 */
extern void timer_init(unsigned freq);

/**
 * @brief Gets the frequency of the CPU.
 *
 * @return The frequency of the CPU in hertz.
 */
uint64_t get_cpu_freq(void);

#endif /* _ASM_FILE_ */

/*============================================================================*/

/**@}*/

#endif /* NANVIX_KERNEL_HAL_TIMER_H_ */
