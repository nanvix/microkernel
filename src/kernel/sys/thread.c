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

#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>
#include <posix/errno.h>

/*============================================================================*
 * kernel_thread_get_id()                                                     *
 *============================================================================*/

/**
 * @see thread_get_id().
 */
PUBLIC int kernel_thread_get_id(void)
{
	return (thread_get_curr_id());
}

#if (THREAD_MAX > 1)

/*============================================================================*
 * kernel_thread_create()                                                     *
 *============================================================================*/

/**
 * @see thread_create().
 */
PUBLIC int kernel_thread_create(int *tid, void *(*start)(void*), void *arg)
{
	/* Invalid start routine. */
	if (start == NULL)
		return (-EINVAL);

	/* Bad start routine. */
#if (defined(KERNEL_THREAD_BAD_START) && (KERNEL_THREAD_BAD_START == 1))
	if (mm_is_kaddr(VADDR(start)))
		return (-EINVAL);
#endif

	/* Bad argument. */
#if (defined(KERNEL_THREAD_BAD_ARG) && (KERNEL_THREAD_BAD_ARG == 1))
	if ((arg != NULL) && (mm_is_kaddr(VADDR(arg))))
		return (-EINVAL);
#endif

	return (thread_create(tid, start, arg));
}

/*============================================================================*
 * kernel_thread_exit()                                                       *
 *============================================================================*/

/**
 * @see thread_exit().
 */
PUBLIC int kernel_thread_exit(void *retval)
{
	/* Bad exit. */
#if (defined(KERNEL_THREAD_BAD_EXIT) && (KERNEL_THREAD_BAD_EXIT == 1))
	if ((retval != NULL) && (mm_is_kaddr(VADDR(retval))))
		return (-EINVAL);
#endif

	thread_exit(retval);

	return (-EAGAIN);
}

/*============================================================================*
 * kernel_thread_join()                                                       *
 *============================================================================*/

/**
 * @see thread_join().
 *
 * @retval -EINVAL Cannot join itself.
 * @retval -EINVAL Cannot join master thread.
 */
PUBLIC int kernel_thread_join(int tid, void **retval)
{
	/* Invalid thread ID. */
	if (tid < 0)
		return (-EINVAL);

	/* Bad join. */
#if (defined(KERNEL_THREAD_BAD_JOIN) && (KERNEL_THREAD_BAD_JOIN == 1))
	if ((retval != NULL) && (mm_is_kaddr(VADDR(retval))))
		return (-EINVAL);
#endif

	/* Cannot join itself. */
	if (tid == thread_get_curr_id())
		return (-EINVAL);

	/* Cannot join master thread. */
	if (WITHIN(tid, 0, SYS_THREAD_MAX))
		return (-EINVAL);

	return (thread_join(tid, retval));
}

/*============================================================================*
 * kernel_thread_yield()                                                      *
 *============================================================================*/

/**
 * @see thread_yield().
 *
 * @retval -EINVAL Failed to release the core.
 */
PUBLIC int kernel_thread_yield(void)
{
	/* Invalid thread. */
	if (thread_get_curr_id() == KTHREAD_MASTER_TID)
		return (-EINVAL);

#if CORE_SUPPORTS_MULTITHREADING
	return (thread_yield());
#else
	return (-ENOSYS);
#endif
}

/**
 * Gets performance statistics of the kernel. Retrived statistics are
 * stored into the userspace buffer pointed to by @p buffer. If @p
 * buffer is a null pointer, then performance counters are started to
 * watch the performance event @p stat.
 */
PUBLIC int kernel_thread_stats(int tid, uint64_t * buffer, int stat)
{
	return (thread_stats(tid, buffer, stat));
}

#endif
