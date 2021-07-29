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

#include "common.h"

/*============================================================================*
 * Sleep/Wake up functions                                                    *
 *============================================================================*/

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
 * @see thread_wakeup().
 *
 * @author João Vicente Souto
 */
PUBLIC void thread_asleep(
	struct resource_arrangement * queue,
	spinlock_t * queue_lock,
	spinlock_t * user_lock
)
{
#if CLUSTER_IS_MULTICORE
	int intlvl;                 /* Previous interrupt level.        */
	int status;                 /* Current core status.             */
	bool changed;               /* Must change the interrupt level? */
	struct thread * curr;       /* Current thread.                  */
	struct section_guard guard; /* Section guard.                   */

	status = core_status_get_mode();

	/* Do not be interrupted because you are going to exit the core. */
	if (UNLIKELY((status == CORE_STATUS_MODE_NORMAL) || (status == CORE_STATUS_MODE_MASKED)))
	{
		/* Does the current level has a lower priority than the new one? */
		changed = (INTERRUPT_LEVEL_NONE > interrupts_get_level());

		/* Sets new interrupt level and keeps the old level. */
		if (changed)
			KASSERT((intlvl = interrupts_set_level(INTERRUPT_LEVEL_NONE)) >= 0);
	}

	spinlock_lock(queue_lock);

		if (user_lock != &lock_tm)
		{
			/* Prevent this call be preempted by any maskable interrupt. */
			section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);


			/* Asleep was called from outside the thread system. */
			thread_lock_tm(&guard);
		}

			/* Stop current thread. */
			curr        = thread_get_curr();
			curr->state = THREAD_SLEEPING;

			/* Sleeps on queue. */
			resource_enqueue(queue, &curr->resource);

		if (user_lock != &lock_tm)
			thread_unlock_tm(&guard);

	spinlock_unlock(queue_lock);

	/* Release user critical region. */
	spinlock_unlock(user_lock);

#if !CORE_SUPPORTS_MULTITHREADING

		/* Waits wake up. */
		core_sleep();

#else /* CORE_SUPPORTS_MULTITHREADING */

		/* Leave the core. */
		thread_yield();

#endif /* CORE_SUPPORTS_MULTITHREADING */

	/* Lock the user critical region. */
	spinlock_lock(user_lock);

	/* Restore interrupt level. */
	if (changed)
		KASSERT((interrupts_set_level(intlvl)) >= 0);

#else
	UNUSED(queue);
	UNUSED(queue_lock);
	UNUSED(user_lock);
#endif
}

/*============================================================================*
 * thread_wakeup()                                                            *
 *============================================================================*/

/**
 * The thread_wakeup() function wakes up the thread pointed to by @t
 * thread. The @t thread will be inserted into scheduler queue.
 *
 * @param t Thread to wakeup.
 *
 * @see thread_asleep().
 *
 * @author João Vicente Souto
 */
PUBLIC void thread_wakeup(struct thread *t)
{
#if CLUSTER_IS_MULTICORE

#if !CORE_SUPPORTS_MULTITHREADING

	/* Wake up thread. */
	core_wakeup(thread_get_coreid(t));

#else /* CORE_SUPPORTS_MULTITHREADING */

	int state;                  /* Current thread state. */
	struct section_guard guard; /* Section guard.        */

	state = thread_get_curr()->state;

	if (state != THREAD_TERMINATED)
	{
		/* Prevent this call be preempted by any maskable interrupt. */
		section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

		thread_lock_tm(&guard);
	}

		/* Insert the thread on the scheduling queue. */
		thread_schedule(t);

#if 0
		/* Verify if it can be already scheduled. */
		do_thread_schedule(false);
#endif

	if (state != THREAD_TERMINATED)
		thread_unlock_tm(&guard);

#endif /* CORE_SUPPORTS_MULTITHREADING */

#else
	UNUSED(t);
#endif
}
