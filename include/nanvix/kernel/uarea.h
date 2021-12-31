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


#ifndef NANVIX_UAREA_H_
#define NANVIX_UAREA_H_

	#include <nanvix/kernel/thread.h>

	/**
	 * @brief User area
	 */
	struct uarea
	{
		int nthreads;                /* Number of running threads.             */
		int next_tid;                /* Next thread ID.                        */
		int retval_curr_slot;        /* Global counter for saving exit values. */

		struct thread threads[THREAD_MAX];                 /* User threads.           */
		struct condvar joincond[THREAD_MAX];               /* Thread join conditions. */
		struct exit_value retvals[KTHREAD_EXIT_VALUE_NUM]; /* Thread's exit value.    */

#if CLUSTER_IS_MULTICORE && CORE_SUPPORTS_MULTITHREADING

		struct resource_arrangement scheduling; /* Schedule queues. */
		struct stack *ustacks[THREAD_MAX];      /* User stacks.     */
		struct stack *kstacks[THREAD_MAX];      /* Kernel stacks.   */

#endif
	};

	/**
	 * @brief User area
	 */
	EXTERN struct uarea uarea;

	/**
	 * @brief Initialize the user area
	 */
	EXTERN void uarea_init(void);

#endif