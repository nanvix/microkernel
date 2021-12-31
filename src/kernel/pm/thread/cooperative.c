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

#if !CORE_SUPPORTS_MULTITHREADING && CLUSTER_IS_MULTICORE

/*============================================================================*
 * User Threads Responsibility                                                *
 *============================================================================*/

/*============================================================================*
 * thread_exit()                                                              *
 *============================================================================*/

/**
 * The thread_exit() function terminates the calling thread. It first
 * releases all underlying kernel resources that are linked to the
 * thread and then resets the underlying core. The return value of the
 * thread, which is pointed to by @p retval, is made available for a
 * thread that joins this one.
 *
 * @note This function does not return.
 * @note This function is thread-safe.
 *
 * @see thread_join().
 *
 * @author Pedro Henrique Penna
 */
PUBLIC NORETURN void thread_exit(void *retval)
{
	int mycoreid;
	struct thread * curr;
	struct section_guard guard; /* Section guard.    */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	/* Indicates that the underlying core will be reset. */
	KASSERT(core_release() == 0);

		curr     = thread_get_curr();
		mycoreid = thread_get_coreid(curr);

		thread_lock_tm(&guard);

			/* Saves the retval of current thread. */
			thread_save_retval(retval, curr);

			/* Release thread structure. */
			thread_free(curr);

			/* Notify waiting threads. */
			cond_broadcast(joincond[mycoreid]);

		thread_unlock_tm(&guard);

	/* No rollback after this point. */
	/* Resets the underlying core. */
	core_reset();

	/* Never gets here. */
	UNREACHABLE();
}

/*============================================================================*
 * Master Thread Responsibility                                               *
 *============================================================================*/

/*============================================================================*
 * thread_create()                                                            *
 *============================================================================*/

/**
 * The thread_create() function create and starts a new thread. The
 * new thread executes the start routine pointed to by @p start, with
 * @p arg begin passed as argument to this function. If @p tid is not
 * a NULL pointed, the ID of the new thread is stored into the
 * location pointed to by @p tid.
 *
 * @retval -EAGAIN Not enough resources to allocate a new thread.
 *
 * @note This function is thread safe.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int thread_create(int *tid, void*(*start)(void*), void *arg)
{
	int ret;                    /* Return value.             */
	int _tid;                   /* Unique thread identifier. */
	int ntrials = 0;            /* Trials realized.          */
	struct thread *new_thread;  /* New thread.               */
	struct section_guard guard; /* Section guard.            */

	/* Sanity check. */
	KASSERT(start != NULL);

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		/* Allocate thread. */
		new_thread = thread_alloc();
		if (new_thread == NULL)
		{
			kprintf("[pm] cannot create thread");
			thread_unlock_tm(&guard);
			return (-EAGAIN);
		}

		/* Get thread ID. */
		_tid = (*next_tid)++;

		/* Initialize thread structure. */
		new_thread->tid   = _tid;
		new_thread->state = THREAD_RUNNING;
		new_thread->arg   = arg;
		new_thread->start = start;
		new_thread->age   = 0ULL;

	thread_unlock_tm(&guard);

	/* Save thread ID. */
	if (tid != NULL)
	{
		*tid = _tid;
		dcache_invalidate();
	}

	/*
	 * We should do some busy waitting here. When the kernel is under
	 * stress, there is a chance that we allocate a core that is in
	 * RUNNING state. That happens because a previous thread running
	 * on this core has existed and we have joined it, but the
	 * terminated thread hasn't had enough time to issue issue a
	 * core_reset().
	 */
	do
	{
		ret = core_start(thread_get_coreid(new_thread), thread_start);
		ntrials++;
	} while (ret == -EBUSY && ntrials < THREAD_CREATE_NTRIALS);


	/* Rollback. */
	if (ret != 0)
	{
		thread_lock_tm(&guard);
			thread_free(new_thread);
		thread_unlock_tm(&guard);
	}

	return (ret);
}

/**
 * @brief Dummy Initialize user area.
 */
PUBLIC void __uarea_init(void)
{

}

/*============================================================================*
 * Thread Manager Initialization                                              *
 *============================================================================*/

/*============================================================================*
 * __thread_init()                                                            *
 *============================================================================*/

/**
 * @brief Dummy Initialize thread system.
 */
PUBLIC void __thread_init(void)
{

}

#endif /* !CORE_SUPPORTS_MULTITHREADING && CLUSTER_IS_MULTICORE */
