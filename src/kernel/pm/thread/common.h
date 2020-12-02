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

#ifndef KERNEL_THREAD_COMMON_H_
#define KERNEL_THREAD_COMMON_H_

	#include <nanvix/kernel/thread.h>
	#include <nanvix/kernel/mm.h>
	#include <nanvix/const.h>
	#include <posix/errno.h>

	/**
	 * @brief Number of thread_create trials.
	 */
	#define THREAD_CREATE_NTRIALS 5

	/**
	 * @brief Kernel thread ID.
	 */
	#define KERNEL_THREAD_ID(_t) (_t - threads)

	/**
	 * @brief Thread table.
	 */
	EXTENSION EXTERN struct thread threads[KTHREAD_MAX];

	/**
	 * @brief Running Threads.
	 */
	EXTENSION EXTERN struct thread * curr_threads[CORES_NUM];

	/**
	 * @brief Returns the target thread.
	 *
	 * The thread_get() function performs a linear search in the table of
	 * threads for the thread whose ID equals to @p tid.
	 *
	 * @param tid ID of the target thread.
	 *
	 * @returns Upon successful completion, a pointer to the target thread
	 * is returned. Upon failure, a null pointed is returned instead.
	 *
	 * @note This function is NOT thread safe.
	 */
#if CLUSTER_IS_MULTICORE
	EXTERN struct thread * thread_get(int tid);
#else
	static struct thread * thread_get(int tid)
	{
		if (tid != 0)
			return (NULL);

		return (KTHREAD_MASTER);
	}
#endif

#if CLUSTER_IS_MULTICORE

	/**
	 * @brief Thread join conditions.
	 */
	EXTENSION EXTERN struct condvar joincond[KTHREAD_MAX];

	/**
	 * @brief Number of running threads.
	 */
	EXTERN int nthreads;

	/**
	 * @brief Next thread ID.
	 */
	EXTERN int next_tid;

	/**
	 * @brief Thread manager lock.
	 */
	EXTERN spinlock_t lock_tm;

	/**
	 * @brief Saves the retval of the leaving thread in the exit values array.
	 *
	 * @param retval return value of the target thread
	 * @param leaving_thread theard of the retval we're saving
	 */
    EXTERN void thread_save_retval(void *retval, struct thread *curr);

	/**
	 * @brief Searches the exit_values array for the target return value of
	 * a thread.
	 *
	 * @param retval return value of the target thread
	 * @param tid Thread's ID
	 */
    EXTERN void thread_search_retval(void **retval, int tid);

	/**
	 * @brief Allocates a thread.
	 *
	 * The thread_alloc() function allocates a new thread structure in the
	 * table of threads.
	 *
	 * @returns Upon successful completion, a pointer to the newly
	 * allocated thread is returned. Upon failure, a NULL pointer is
	 * returned instead.
	 *
	 * @note This function is NOT thread-safe.
	 *
	 * @author Pedro Henrique Penna and João Vicente Souto
	 */
	EXTERN struct thread * thread_alloc(void);

	/**
	 * @brief Releases a thread.
	 *
	 * The thread_free() function releases the thread entry pointed to by
	 * @p t in the table of threads.
	 *
	 * @note This function is NOT thread-safe.
	 *
	 * @author Pedro Henrique Penna
	 */
	EXTERN void thread_free(struct thread *t);

	/**
	 * @brief Underlying releases a thread.
	 *
	 * The thread_free() function releases the thread entry pointed to by
	 * @p t in the table of threads.
	 *
	 * @note This function is NOT thread-safe.
	 *
	 * @author João Vicente Souto
	 */
	EXTERN void __thread_free(struct thread *t);

	/*============================================================================*
	 * thread_start()                                                             *
	 *============================================================================*/

	/**
	 * @brief Starts a thread.
	 *
	 * The thread_start function is a wrapper routine for the user-level
	 * thread start routine. Overall, it does some basic, kernel level
	 * setup and calls the registered user-level function.
	 *
	 * @note This function does not return.
	 *
	 * @author Pedro Henrique Penna
	 */
	EXTERN NORETURN void thread_start(void);

	/**
	 * @brief Initialize thread system.
	 */
	EXTERN void __thread_init(void);

#endif /* CLUSTER_IS_MULTICORE */
#endif /* KERNEL_THREAD_COMMON_H_ */
