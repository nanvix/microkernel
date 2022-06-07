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
 * Global Variables                                                           *
 *============================================================================*/

/**
 * @name Imported definitions.
 */
/**@{*/
EXTERN void kmain(int argc, const char *argv[]);
EXTERN void * task_loop(void * args);
/**@}*/

/**
 * @brief Thread table.
 */
EXTENSION PUBLIC struct thread *threads[KTHREAD_MAX];

/**
 * @brief System thread table.
 */
EXTENSION PUBLIC struct thread threads_sys[SYS_THREAD_MAX] = {
	[0] = {
		.resource = RESOURCE_STATIC_INITIALIZER,
		.tid      = KTHREAD_MASTER_TID,
		.coreid   = COREID_MASTER,
		.state    = THREAD_READY,
		.affinity = KTHREAD_AFFINITY_MASTER,
		.age      = 0ULL,
		.arg      = NULL,
		.start    = (void *) kmain,
		.ctx      = NULL,
	},
#if __NANVIX_USE_TASKS
	[1] = {
		.resource = RESOURCE_STATIC_INITIALIZER,
		.tid      = KTHREAD_DISPATCHER_TID,
		.coreid   = KTHREAD_DISPATCHER_CORE,
		.state    = THREAD_READY,
		.affinity = KTHREAD_AFFINITY_FIXED(KTHREAD_DISPATCHER_CORE),
		.age      = 0ULL,
		.arg      = NULL,
		.start    = (void *) task_loop,
		.ctx      = NULL,
	}
#endif
};

/**
 * @brief Running Threads.
 */
EXTENSION PUBLIC struct thread * curr_threads[CORES_NUM];

#if CLUSTER_IS_MULTICORE

/**
 * @brief Thread join conditions for system threads.
 */
PUBLIC struct condvar joincond_sys[SYS_THREAD_MAX];

/**
 * @brief Thread join conditions.
 */
EXTENSION PUBLIC struct condvar *joincond[KTHREAD_MAX];

/**
 * @brief User area
 */
PUBLIC struct uarea uarea;

/**
 * @brief Number of running threads.
 */
PUBLIC int *nthreads;

/**
 * @brief Next thread ID.
 */
PUBLIC int *next_tid;

/**
 * @brief Is the cluster freezing?
 */
PUBLIC int *freezing = &uarea.freezing

/**
 * @brief Thread manager lock.
 */
PUBLIC spinlock_t lock_tm;

/**
 * @brief Thread manager lock to protect the current thread array.
 */
PUBLIC spinlock_t lock_curr_tm;

/*============================================================================*
 * Return values                                                              *
 *============================================================================*/

/*============================================================================*
 * Variables                                                                  *
 *============================================================================*/

/**
 * @brief Thread's exit value
 */
PRIVATE struct exit_value *retvals;

/**
 * @brief Global counter for saving exit values.
 */
PRIVATE int *retval_curr_slot;

/*============================================================================*
 * thread_save_retval                                                         *
 *============================================================================*/

/**
 * @brief Saves the retval of the leaving thread in the exit values array.
 *
 * @param retval return value of the target thread
 * @param leaving_thread theard of the retval we're saving
 */
PUBLIC void thread_save_retval(void* retval, struct thread *leaving_thread)
{
	/* Check if retval is a valid pointer*/
	if (retval)
	{
		retvals[(*retval_curr_slot)].tid    = thread_get_id(leaving_thread);
		retvals[(*retval_curr_slot)].retval = retval;

		*retval_curr_slot = ((*retval_curr_slot) + 1) % KTHREAD_EXIT_VALUE_NUM;
	}
}

/*============================================================================*
 * thread_search_retval                                                       *
 *============================================================================*/

/**
 * @brief Searches the retvals array for the target return value of
 * a thread.
 *
 * @param retval return value of the target thread
 * @param tid Thread's ID
 */
PUBLIC void thread_search_retval(void **retval, int tid)
{
	/* We need the adress of retval to be different than NULL to use it. */
	if (retval)
	{
		/**
		 * We set the value of retval as NULL in case
		 * we dont find a corresponding TID in the search.
		 */
		*retval = NULL;

		/* Search. */
		for (int i = 0; i < KTHREAD_EXIT_VALUE_NUM; i++)
		{
			/* Found? */
			if (retvals[i].tid == tid)
			{
				*retval = retvals[i].retval;
				break;
			}
		}
	}
}

/*============================================================================*
 * Getters                                                                    *
 *============================================================================*/

/*============================================================================*
 * thread_set_curr()                                                          *
 *============================================================================*/

/**
 * @brief Sets the currently running thread.
 *
 * @param curr New current thread.
 */
PUBLIC void thread_set_curr(struct thread * curr)
{
	struct section_guard guard; /* Section guard.  */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_curr_tm, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);
		curr_threads[core_get_id()] = curr;
	section_guard_exit(&guard);
}

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
PUBLIC struct thread * thread_get_curr(void)
{
	struct thread * curr;       /* Current thread. */
	struct section_guard guard; /* Section guard.  */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_curr_tm, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);
		curr = curr_threads[core_get_id()];
	section_guard_exit(&guard);

	/* NULL pointer should not happen. */
	return (curr);
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
PUBLIC struct thread * thread_get(int tid)
{
	/* Sanity check. */
	KASSERT(tid > KTHREAD_NULL_TID);

	/* Search for target thread. */
	for (int i = 0; i < KTHREAD_MAX; i++)
	{
		/* Found. */
		if (threads[i]->tid == tid)
			return (threads[i]);
	}

	return (NULL);
}

/*============================================================================*
 * Thread Allocation/Release                                                  *
 *============================================================================*/

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
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC struct thread * thread_alloc(void)
{
	for (int i = SYS_THREAD_MAX; i < KTHREAD_MAX; i++)
	{
		/* Verify the state of the thread. */
		switch (threads[i]->state)
		{
			/* Found a free thread. */
			case THREAD_NOT_STARTED:
				(*nthreads)++;
#if !CORE_SUPPORTS_MULTITHREADING
				threads[i]->coreid = i;
				curr_threads[i]   = threads[i];
				break;
#else
				break;

			/* Found a zombie thread (frees used kpages). */
			case THREAD_ZOMBIE:
				thread_free(threads[i]);
				break;

#endif

			/* Skip busy thread. */
			default:
				continue;
		}

		/* Initializes chosen thread. */
		threads[i]->state    = THREAD_READY;
		threads[i]->resource = RESOURCE_INITIALIZER;

		return (threads[i]);
	}

	/* All threads are in use. */
	return (NULL);
}

/*============================================================================*
 * __thread_free()                                                            *
 *============================================================================*/

#if !CORE_SUPPORTS_MULTITHREADING

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
PUBLIC void __thread_free(struct thread *t)
{
	struct section_guard guard; /* Section guard.  */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_curr_tm, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);
		curr_threads[t->coreid] = NULL;
	section_guard_exit(&guard);
}

#endif

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
PUBLIC void thread_free(struct thread *t)
{
	KASSERT(WITHIN(KERNEL_THREAD_ID(t), 0, KTHREAD_MAX));

	__thread_free(t);

	t->coreid = -1;
	t->state  = THREAD_NOT_STARTED;
	t->tid    = KTHREAD_NULL_TID;
	(*nthreads)--;
}

/*============================================================================*
 * User Threads Responsibility                                                *
 *============================================================================*/

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
PUBLIC NORETURN void thread_start(void)
{
	void *retval;         /* Return value.   */
	struct thread * curr; /* Current thread. */

	curr = thread_get_curr();

	__thread_prolog(curr);

	interrupts_enable();
	interrupts_set_level(INTERRUPT_LEVEL_LOW);
	interrupt_mask(INTERRUPT_TIMER);

		retval = curr->start(curr->arg);

	interrupts_disable();

	thread_exit(retval);

	/* Never gets here. */
	UNREACHABLE();
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
	struct section_guard guard; /* Section guard.    */

	/* Sanity check. */
	KASSERT(tid > KTHREAD_NULL_TID);
	KASSERT(tid != thread_get_curr_id());
	KASSERT(tid != KTHREAD_MASTER_TID);

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		/* Wait for thread termination. */
		if ((t = thread_get(tid)) != NULL)
		{
			/*
			 * The target thread is still running,
			 * so we have to block and wait for it.
			 */
			if (t->state != THREAD_NOT_STARTED &&
				t->state != THREAD_TERMINATED &&
				t->state != THREAD_ZOMBIE)
				cond_wait(joincond[KERNEL_THREAD_ID(t)], &lock_tm);
		}

		/**
		 * Thread IDs are incremented by next_id. So, if we want to know
		 * if the @p tid is valid and has already left, just check if it
		 * is less than the next_tid.
		 */
		ret = (tid < (*next_tid)) ? 0 : (-EINVAL);

		/**
		 * This prevents the thread from returning an invalid value.
		 * This if is used guarante that the the @p ret is valid
		 */
		if (ret == 0)
			thread_search_retval(retval, tid);

	thread_unlock_tm(&guard);

	return (ret);
}

/**
 * Gets performance statistics of the thread. Retrived statistics are
 * stored into the userspace buffer pointed to by @p buffer. If @p
 * buffer is a null pointer, then performance counters are started to
 * watch the performance event @p perf.
 */
PUBLIC int thread_stats(int tid, uint64_t * buffer, int stat)
{
#if __NANVIX_MICROKERNEL_THREAD_STATS

	struct thread * t;

	if (tid < 0 || (t = thread_get(tid)) == NULL)
		return (-EINVAL);

	/* Start counter. */
	if (buffer == NULL)
	{
		switch (stat)
		{
			case KTHREAD_STATS_EXEC_TIME:
				t->stats.exec_total = 0ULL;
				break;

			default:
				return (-EFAULT);
		}
	}

	/* Stop counter. */
	else
	{
		/* Bad buffer. */
		if (!mm_check_area(VADDR(buffer), sizeof(uint64_t), UMEM_AREA))
			return (-EFAULT);

		/* Save statistic. */
		switch (stat)
		{
			case KTHREAD_STATS_EXEC_TIME:
				*buffer = t->stats.exec_total;
				break;

			default:
				return (-EFAULT);
		}
	}

	return (0);

#else

	UNUSED(tid);
	UNUSED(buffer);
	UNUSED(stat);

	return (-ENOSYS);

#endif
}

#endif /* CLUSTER_IS_MULTICORE */

/**
 * @brief Initialize user area.
 */
PUBLIC void uarea_init(void)
{
	for (int i = 0; i < SYS_THREAD_MAX; i++)
	{
		threads[i] = &threads_sys[i];
		joincond[i] = &joincond_sys[i];
	}

	for (int i = SYS_THREAD_MAX; i < KTHREAD_MAX; i++)
	{
		threads[i] = &uarea.threads[i - SYS_THREAD_MAX];
		joincond[i] = &uarea.joincond[i - SYS_THREAD_MAX];
	}

	nthreads = &uarea.nthreads;
	next_tid = &uarea.next_tid;
	retval_curr_slot = &uarea.retval_curr_slot;
	retvals = uarea.retvals;
	uarea.freezing = 0;

	__uarea_init();
}


/*============================================================================*
 * Thread Manager Initialization                                              *
 *============================================================================*/

/*============================================================================*
 * thread_init()                                                              *
 *============================================================================*/

/**
 * @brief Initialize thread system.
 */
PUBLIC void thread_init(void)
{
	uarea_init();
	
	/**
	 * Initializes global variables.
	 */
	curr_threads[0] = threads[0];

	/* Initializes user threads. */
	for (int i = KTHREAD_SERVICE_MAX; i < KTHREAD_MAX; ++i)
	{
		threads[i]->resource  = RESOURCE_INITIALIZER;
		threads[i]->tid       = KTHREAD_NULL_TID;
		threads[i]->coreid    = -1;
		threads[i]->state     = THREAD_NOT_STARTED;
		threads[i]->affinity  = 0;
		threads[i]->age       = 0ULL;
		threads[i]->core_mode = 0ULL;
		threads[i]->arg       = NULL;
		threads[i]->start     = NULL;
		threads[i]->ctx       = NULL;

#if __NANVIX_MICROKERNEL_THREAD_STATS
		threads[i]->stats.exec_start = 0ULL;
		threads[i]->stats.exec_total = 0ULL;
#endif
	}

	for (int i = 1; i < CORES_NUM; ++i)
		curr_threads[i] = NULL;

#if CLUSTER_IS_MULTICORE

	/* Init join conditions. */
	for (int i = 0; i < KTHREAD_MAX; ++i)
		cond_init(joincond[i]);

	/** Init number of running threads. */
	*nthreads = KTHREAD_SERVICE_MAX;

	/* Init next thread ID. */
	*next_tid = KTHREAD_SERVICE_MAX;

	/* Init thread manager locks. */
	spinlock_init(&lock_tm);
	spinlock_init(&lock_curr_tm);

	/* Init return values structure. */
	for (int i = 0; i < KTHREAD_EXIT_VALUE_NUM; ++i)
	{
		retvals[i].tid = KTHREAD_NULL_TID;
		retvals[i].retval = NULL;
	}

	/* Current slot on retval array. */
	*retval_curr_slot = 0;

	/* Init dependent args. */
	__thread_init();

#endif /* CLUSTER_IS_MULTICORE */
}

