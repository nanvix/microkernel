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
#include <nanvix/hlib.h>

/*============================================================================*
 * mutex_down()                                                               *
 *============================================================================*/

/**
 * The mutex_lock() function performs a lock operation in the
 * mutex pointed to by @p sem. It atomically allocates a ticket.
 * If it is equal to the current ticket of the mutex, the calling
 * thread continue its execution, flow as usual. Otherwise, the
 * calling thread sleeps until another thread performs a call to
 * mutex_unlock() on this mutex. The unlock only wake up the first
 * thread stopped into the condition variable queue.
 *
 * @see mutex_init(), mutex_unlock()
 */
PUBLIC void mutex_lock(struct mutex *m)
{
	int ticket;

	KASSERT(m != NULL);

	spinlock_lock(&m->lock);

		/* Allocates a ticket. */
		ticket = m->next_ticket++;

		/* The ticket never will be less then curr_ticket. */
		KASSERT(m->curr_ticket <= ticket);

		/* While another thread holds the lock. */
		while (m->curr_ticket < ticket)
			cond_wait(&m->cond, &m->lock);

		/* Local ticket must be the same of current ticket. */
		KASSERT(m->curr_ticket == ticket);

		/* Old owner clean up the mutex. */
		KASSERT(m->curr_owner == KTHREAD_NULL_TID);

		m->curr_owner = thread_get_curr_id();

	spinlock_unlock(&m->lock);
}

/*============================================================================*
 * mutex_unlock()                                                             *
 *============================================================================*/

/**
 * The mutex_unlock() function performs an unlock a mutex
 * pointed to by @p m. This wakeup the first thread waiting on the mutex queue.
 *
 * @see mutex_init(), mutex_lock()
 */
PUBLIC void mutex_unlock(struct mutex *m)
{
	KASSERT(m != NULL);

	spinlock_lock(&m->lock);

		/* Calling thread holds the lock. */
		KASSERT(m->curr_owner == thread_get_curr_id());

		/* Update the current ticket. */
		m->curr_ticket++;
		m->curr_owner = KTHREAD_NULL_TID;

		/* Unlock must be done in an locked mutex. */
		KASSERT(m->curr_ticket <= m->next_ticket);

		/* Wake up the first sleeping thread. */
		cond_anycast(&m->cond);

	spinlock_unlock(&m->lock);
}
