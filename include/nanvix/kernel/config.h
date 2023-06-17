/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
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

/**
 * @defgroup kernel-config Kconfig
 * @ingroup kernel
 *
 * @brief Kernel Configuration
 */

#ifndef NANVIX_KERNEL_CONFIG_H_
#define NANVIX_KERNEL_CONFIG_H_

	/**
	 * @name Synchronization Primitives
	 */
	/**@{*/
	#if defined(__unix64__)
		#define __NANVIX_MUTEX_SLEEP     0 /**< Blocking Mutexes?    */
		#define __NANVIX_SEMAPHORE_SLEEP 0 /**< Blocking Semaphores? */
		#define __NANVIX_CONDVAR_SLEEP   0 /**< Blocking Condition Variables? */
	#else
		#define __NANVIX_MUTEX_SLEEP     1 /**< Blocking Mutexes?    */
		#define __NANVIX_SEMAPHORE_SLEEP 1 /**< Blocking Semaphores? */
		#define __NANVIX_CONDVAR_SLEEP   1 /**< Blocking Condition Variables? */
	#endif
	/**@}*/

	/**
	 * @name Task
	 */
	/**@{*/
	#if defined(__mppa256__)
		#define __NANVIX_USE_TASKS 1 /**< Use an dedicated thread to perform tasks? */
	#else
		#error "Currently, only MPPA-256 supports the task engine"
	#endif
	/**@}*/

	/**
	 * @brief Network Capabilities
	 */
	#if defined(__qemu_x86__) || defined(__qemu_openrisc__)
		#define __NANVIX_HAS_NETWORK 1
	#else
		#define __NANVIX_HAS_NETWORK 0
	#endif

/**@}*/

#endif /* NANVIX_KERNEL_CONFIG_H_ */
