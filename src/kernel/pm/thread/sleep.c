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

#include <nanvix/hal.h>
#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>

#include "common.h"

/*============================================================================*
 * thread_asleep()                                                            *
 *============================================================================*/

/**
 * The thread_asleep() function atomically puts the calling thread to
 * sleep.  Before sleeping, the spinlock pointed to by @p lock is
 * released.  The calling thread resumes its execution when another
 * thread invokes thread_wakeup() on it. When the thread wakes up, the
 * spinlock @p lock is re-acquired.
 *
 * @note This function is NOT thread safe.
 *
 * @see thread_wakeup().
 *
 * @author Pedro Henrique Penna
 */
PUBLIC void thread_asleep(spinlock_t *lock)
{
	spinlock_unlock(lock);
		core_sleep();
	spinlock_lock(lock);
}

/*============================================================================*
 * thread_wakeup()                                                            *
 *============================================================================*/

/**
 * The thread_wakeup() function wakes up the thread pointed to by @p
 * thread.
 *
 * @note This function is NOT thread safe.
 *
 * @see thread_asleep().
 *
 * @author Pedro Henrique Penna
 */
PUBLIC void thread_wakeup(struct thread *t)
{
	core_wakeup(thread_get_coreid(t));
}
