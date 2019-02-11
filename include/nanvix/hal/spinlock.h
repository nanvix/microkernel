/*
 * MIT License
 *
 * Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef NANVIX_HAL_SPINLOCK_H_
#define NANVIX_HAL_SPINLOCK_H_

/**
 * @addtogroup kernel-hal-spinlock Spinlock
 * @ingroup kernel-hal-cpu
 *
 * @brief Spinlock Interface
 */
/**@{*/

	#include <nanvix/const.h>
	#include <nanvix/hal/target.h>

#ifdef HAL_SMP

	/**
	 * @brief Initializes a spinlock.
	 *
	 * @param lock Target spinlock.
	 */
	EXTERN void spinlock_init(spinlock_t *lock);

	/**
	 * @brief Locks a spinlock.
	 *
	 * @param lock Target spinlock.
	 */
	EXTERN void spinlock_lock(spinlock_t *lock);

	/**
	 * @brief Attempts to lock a spinlock.
	 *
	 * @param lock Target spinlock.
	 *
	 * @returns Upon successful completion, the spinlock pointed to by
	 * @p lock is locked and non-zero is returned. Upon failure, zero
	 * is returned instead, and the lock is not acquired by the
	 * caller.
	 */
	EXTERN int spinlock_trylock(spinlock_t *lock);

	/**
	 * @brief Unlocks a spinlock.
	 *
	 * @param lock Target spinlock.
	 */
	EXTERN void spinlock_unlock(spinlock_t *lock);

#endif

/**@}*/

#endif /* NANVIX_HAL_SPINLOCK_H_ */