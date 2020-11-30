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

#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>
#include <posix/errno.h>

/**
 * @brief Lock to sleeping queues.
 */
spinlock_t lock = SPINLOCK_UNLOCKED;

/**
 * brief Sleeping queues.
 */
PRIVATE struct condvar sleeping = COND_STATIC_INITIALIZER;

/*============================================================================*
 * kernel_sleep()                                                             *
 *============================================================================*/

/**
 * The kernel_sleep() function causes the calling thread to block
 * until it receives a wakeup signal from another thread.
 */
PUBLIC int kernel_sleep(void)
{
	spinlock_lock(&lock);
		cond_wait(&sleeping, &lock);
	spinlock_unlock(&lock);

	return (0);
}

/**
 * The kernel_wakeup() function wakes up the thread whose identifier
 * matchers @p tid and it is asleep.
 *
 * @retval -EINVAL Invalid thread ID.
 */
PUBLIC int kernel_wakeup(int tid)
{
	int ret;

	/* Invalid thread ID. */
	if (tid < 0)
		return (-EINVAL);

	/* Search for sleeping thread. */
	spinlock_lock(&lock);
		ret = cond_unicast(&sleeping, tid);
	spinlock_unlock(&lock);

	return (ret);
}
