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
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>
#include <posix/errno.h>
#include "common.h"

#if !CORE_SUPPORTS_MULTITHREADING

/**
 * @brief Number of thread_create trials.
 */
#define THREAD_CREATE_NTRIALS 5

/*
 * Import definitions.
 */
EXTERN void kmain(int argc, const char *argv[]);

/**
 * @brief Thread table.
 */
EXTENSION PUBLIC struct thread threads[KTHREAD_MAX] = {
	[0] = {
		.tid = KTHREAD_MASTER_TID,
		.coreid = 0,
		.state = THREAD_RUNNING,
		.arg = NULL,
		.start = (void *) kmain,
		.next = NULL
	},
#if CLUSTER_IS_MULTICORE
	[1 ... (KTHREAD_MAX - 1)] = {.tid = KTHREAD_NULL_TID, .state = THREAD_NOT_STARTED}
#endif
} ;

#if CLUSTER_IS_MULTICORE

/**
 * @brief Thread join conditions.
 */
EXTENSION PRIVATE struct condvar joincond[KTHREAD_MAX] = {
	[0 ... (KTHREAD_MAX - 1)] = COND_INITIALIZER
};

/**
 * @brief Number of running threads.
 */
PRIVATE int nthreads = 1;

/**
 * @brief Next thread ID.
 */
PRIVATE int next_tid = (KTHREAD_MASTER_TID + 1);

/**
 * @brief Thread manager lock.
 */
PRIVATE spinlock_t lock_tm = SPINLOCK_UNLOCKED;

/*============================================================================*
 * thread_get_curr()                                                          *
 *============================================================================*/

/**
* @brief Gets the currently running thread.
*
* The thread_get() function returns a pointer to the thread
* that is running in the underlying core.
*
* @returns A pointer to the thread that is running in the
* underlying core.
*/
PUBLIC struct thread *thread_get_curr(void)
{
	int mycoreid; /* Core ID. */

	mycoreid = core_get_id();

	for (int i = 0; i < KTHREAD_MAX; i++)
	{
		/* Found. */
		if (threads[i].coreid == mycoreid)
			return (&threads[i]);
	}

	/* Should not happen. */
	return (NULL);
}

/*============================================================================*
 * thread_alloc()                                                             *
 *============================================================================*/

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
 * @author Pedro Henrique Penna
 */
PRIVATE struct thread *thread_alloc(void)
{
	for (int i = 1; i < KTHREAD_MAX; i++)
	{
		/* Found. */
		if (threads[i].state == THREAD_NOT_STARTED)
		{
			struct thread *new_thread;
			new_thread = &threads[i];
			new_thread->coreid = i;
			new_thread->state = THREAD_STARTED;
			nthreads++;

			return (new_thread);
		}
	}

	return (NULL);
}

/*============================================================================*
 * thread_free()                                                              *
 *============================================================================*/

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
PRIVATE void thread_free(struct thread *t)
{
	KASSERT(t >= &threads[0]);
	KASSERT(t <= &threads[KTHREAD_MAX - 1]);

	t->coreid = -1;
	t->state = THREAD_NOT_STARTED;
	t->tid = KTHREAD_NULL_TID;
	nthreads--;
}

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
	struct thread *curr_thread;

	/* Indicates that the underlying core will be reset. */
	KASSERT(core_release() == 0);

		curr_thread = thread_get_curr();
		mycoreid = thread_get_coreid(curr_thread);

		spinlock_lock(&lock_tm);

			/* Saves the retval of current thread. */
			thread_save_retval(retval, curr_thread);

			/* Release thread structure. */
			thread_free(curr_thread);

			/* Notify waiting threads. */
			cond_broadcast(&joincond[mycoreid]);

		spinlock_unlock(&lock_tm);

	/* No rollback after this point. */
	/* Resets the underlying core. */
	core_reset();

	/* Never gets here. */
	UNREACHABLE();
}

/*============================================================================*
 * thread_get()                                                               *
 *============================================================================*/

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
PRIVATE struct thread *thread_get(int tid)
{
	/* Sanity check. */
	KASSERT(tid > KTHREAD_NULL_TID);

	/* Search for target thread. */
	for (int i = 0; i < KTHREAD_MAX; i++)
	{
		/* Found. */
		if (threads[i].tid == tid)
			return (&threads[i]);
	}

	return (NULL);
}

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
PRIVATE NORETURN void thread_start(void)
{
	void *retval;               /* Return value.   */
	struct thread *curr_thread; /* Current thread. */

	curr_thread = thread_get_curr();

	retval = curr_thread->start(curr_thread->arg);

	thread_exit(retval);

	/* Never gets here. */
	UNREACHABLE();
}

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
	int ret;                   /* Return value.             */
	int _tid;                  /* Unique thread identifier. */
	struct thread *new_thread; /* New thread.               */
	int ntrials = 0;           /* Trials realized.          */

	/* Sanity check. */
	KASSERT(start != NULL);

	spinlock_lock(&lock_tm);

		/* Allocate thread. */
		new_thread = thread_alloc();
		if (new_thread == NULL)
		{
			kprintf("[pm] cannot create thread");
			spinlock_unlock(&lock_tm);
			return (-EAGAIN);
		}

		/* Get thread ID. */
		_tid = next_tid++;

		/* Initialize thread structure. */
		new_thread->tid   = _tid;
		new_thread->state = THREAD_RUNNING;
		new_thread->arg   = arg;
		new_thread->start = start;
		new_thread->next  = NULL;

	spinlock_unlock(&lock_tm);

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
		spinlock_lock(&lock_tm);
			thread_free(new_thread);
		spinlock_unlock(&lock_tm);
	}

	return (ret);
}

/*============================================================================*
 * thread_join()                                                              *
 *============================================================================*/

/**
 * The thread_join() function causes the calling thread to block until
 * the thread whose ID equals to @p tid its terminates execution, by
 * calling thread_exit(). If @p retval is not a null pointed, then the
 * return value of the terminated thread is stored in the location
 * pointed to by @p retval.
 *
 * @retval -EINVAL Invalid thread ID.
 *
 * @see thread_exit().
 *
 * @todo Retrieve return value.
 */
PUBLIC int thread_join(int tid, void **retval)
{
	int ret;
	struct thread *t;

	/* Sanity check. */
	KASSERT(tid > KTHREAD_NULL_TID);
	KASSERT(tid != thread_get_curr_id());
	KASSERT(tid != KTHREAD_MASTER_TID);

	spinlock_lock(&lock_tm);

		/* Wait for thread termination. */
		if ((t = thread_get(tid)) != NULL)
		{
			int coreid;

			coreid = thread_get_coreid(t);

			/*
			 * The target thread is still running,
			 * so we have to block and wait for it.
			 */
			if (t->state == THREAD_RUNNING)
				cond_wait(&joincond[coreid], &lock_tm);
		}

		/**
		 * Thread IDs are incremented by next_id. So, if we want to know
		 * if the @p tid is valid and has already left, just check if it
		 * is less than the next_tid.
		 */
		ret = (tid < next_tid) ? 0 : (-EINVAL);

		/**
		 * This prevents the thread from returning an invalid value.
		 * This if is used guarante that the the @p retval is valid
		 */
		if (ret == 0)
			thread_search_retval(retval, tid);

	spinlock_unlock(&lock_tm);

	return (ret);
}

#endif /* CLUSTER_IS_MULTICORE */

/*============================================================================*
 * thread_init()                                                              *
 *============================================================================*/

/**
 * @brief Dummy Initialize thread system.
 */
PUBLIC void thread_init(void)
{

}

#endif /* "CORE_SUPPORTS_MULTITHREADING */
