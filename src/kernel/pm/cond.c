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

#include "thread/common.h"

/*============================================================================*
 * cond_wait()                                                                *
 *============================================================================*/

/**
 * The cond_wait() function causes the calling thread to block, until
 * the condition variable pointed to by @p cond is signaled and the
 * calling thread is chosen to run. If @p lock is unlocked before the
 * calling thread blocks, and when it wakes up the lock is
 * re-acquired.
 *
 * @see cond_broadcast()
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int cond_wait(struct condvar *cond, spinlock_t *lock)
{
	KASSERT(cond != NULL);
	KASSERT(lock != NULL);

	/* Put the calling thread to sleep. */
	thread_asleep(&cond->queue, &cond->lock, lock);

	return (0);
}

/*============================================================================*
 * cond_unicast()                                                             *
 *============================================================================*/

/**
 * The cond_unicast() function sends a wakeup signal to a specific
 * thread that are currently blocked waiting on the conditional
 * variable pointed to by @p cond.
 *
 * @see cond_wait().
 *
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC int cond_unicast(struct condvar *cond, int tid)
{
	int ret;
	struct thread * t;

	KASSERT(cond != NULL);

	if (tid < 0)
		return (-EINVAL);

	ret = (-EAGAIN);

	spinlock_lock(&cond->lock);

		if ((t = thread_get(tid)) == NULL)
			goto error;

		if ((resource_pop(&cond->queue, &t->resource)) < 0)
			goto error;

		thread_wakeup(t);
		ret = (0);

error:
	spinlock_unlock(&cond->lock);

	return (ret);
}

/*============================================================================*
 * cond_multicast()                                                           *
 *============================================================================*/

/**
 * The cond_multicast() function sends a wakeup signal to N first thread
 * that are currently blocked waiting on the conditional variable
 * pointed to by @p cond.
 *
 * @see cond_wait().
 *
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PRIVATE int cond_multicast(struct condvar *cond, int nwakeups)
{
	KASSERT(cond != NULL);

	spinlock_lock(&cond->lock);

		/* Wakeup threads. */
		while (UNLIKELY(cond->queue.size && nwakeups))
		{
			nwakeups--;
			thread_wakeup((struct thread *) resource_dequeue(&cond->queue));
		}

	spinlock_unlock(&cond->lock);

	return (0);
}

/*============================================================================*
 * cond_anycast()                                                             *
 *============================================================================*/

/**
 * The cond_anycast() function sends a wakeup signal to first thread
 * that are currently blocked waiting on the conditional variable
 * pointed to by @p cond.
 *
 * @see cond_wait().
 *
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC int cond_anycast(struct condvar *cond)
{
	return (cond_multicast(cond, 1));
}

/*============================================================================*
 * cond_broadcast()                                                           *
 *============================================================================*/

/**
 * The cond_broadcast() function sends a wakeup signal to all threads
 * that are currently blocked waiting on the conditional variable
 * pointed to by @p cond.
 *
 * @see cond_wait().
 *
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC int cond_broadcast(struct condvar *cond)
{
	return (cond_multicast(cond, KTHREAD_MAX));
}

