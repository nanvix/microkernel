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

#if CLUSTER_IS_MULTICORE && CORE_SUPPORTS_MULTITHREADING

/**
 * @brief Indicates for idle threads to exit.
 */
PRIVATE volatile int tm_shutdown = 0;

/**
 * @brief Maximum of stacks.
 */
#define KSTACK_MAX (THREAD_MAX + KTHREAD_SERVICE_MAX)

/**
 * @name Stacks.
 */
/**@{*/
PRIVATE struct stack *ustacks[KSTACK_MAX];
PRIVATE struct stack *kstacks[KSTACK_MAX];
/**@}*/

/**
 * @brief Schedule queues.
 */
PRIVATE struct resource_arrangement scheduling;

/*============================================================================*
 * Thread Allocation/Release                                                  *
 *============================================================================*/

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
PUBLIC void __thread_free(struct thread *t)
{
	int utid;
	KASSERT(t->state == THREAD_ZOMBIE);

	utid = KTHREAD_USER_ID(t);
	kpage_put((void *) ustacks[utid]);
	kpage_put((void *) kstacks[utid]);
	ustacks[utid] = NULL;
	kstacks[utid] = NULL;
}

/*============================================================================*
 * thread_set_affinity()                                                      *
 *============================================================================*/

/**
 * @brief Sets a new
 *
 * @param new_affinity New affinity value.
 *
 * @returns Old affinity value.
 *
 * This function is thread-safe.
 *
 * @author João Vicente Souto
 */
PUBLIC int thread_set_affinity(struct thread * t, int new_affinity)
{
	int old_affinity;           /* Old affinity.   */
	struct section_guard guard; /* Section guard.  */

	/* Sanity check. */
	KASSERT(KTHREAD_AFFINITY_IS_VALID(new_affinity));

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_curr_tm, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);
		old_affinity = t->affinity;
		t->affinity  = new_affinity;
	section_guard_exit(&guard);

	return (old_affinity);
}

/*============================================================================*
 * Scheduling functions                                                       *
 *============================================================================*/

/*============================================================================*
 * thread_switch_to()                                                         *
 *============================================================================*/

/**
 * @brief Switch between contexts.
 *
 * @param previous Previous thread context where the current context will be
 * stored.
 * @param next     Next thread context from which the new context will be
 * consumed.
 *
 * @todo We must verify if the @p previous and @p next are pointing to kernel
 * pages and not to user pages.
 */
PRIVATE void thread_switch_to(struct context ** previous, struct context ** next)
{
	/* Does a thread try to switch to itself? */
	KASSERT(previous != next);

	/* Invalid previous. */
	KASSERT(previous != NULL && *previous == NULL);

	/* Invalid next. */
	KASSERT(next != NULL && *next != NULL);

	/* Switch with success. */
	KASSERT(context_switch_to(previous, next) == 0);
}

/*============================================================================*
 * thread_schedule_next()                                                     *
 *============================================================================*/

/**
 * @brief Helper to store the desired affinity used by the thread_choose().
 */
PRIVATE int thread_desired_affinity;

/**
 * @brief Looks if the @p r thread has a similary to a desired affinity. In
 * general, it will be used to select a thread that has an affinity to a specific
 * core.
 *
 * @returns True if the thread has at least one bit similary with the desired
 * affinity, false otherwise.
 */
PRIVATE bool thread_choose(struct resource * r)
{
	struct thread * t = (struct thread *) r;

	return (KTHREAD_AFFINITY_MATCH(thread_desired_affinity, t->affinity) != 0);
}

/**
 * @brief Gets the next user thread to be scheduled.
 *
 * @returns Valid thread pointer if there is user threads ready to be scheduled,
 * NULL otherwise.
 */
PRIVATE struct thread * thread_schedule_next(void)
{
	/* Sets the desired affinity to the underlying core. */
	thread_desired_affinity = KTHREAD_AFFINITY_FIXED(core_get_id());

	return ((struct thread *) resource_remove_verify(&scheduling, thread_choose));
}

/*============================================================================*
 * thread_schedule()                                                          *
 *============================================================================*/

/**
* @brief Insert a new thread into the scheduling queue.
*
* @param t Target thread.
*/
PUBLIC void thread_schedule(struct thread * t)
{
	/* Valid thread. */
	KASSERT(t->state != THREAD_RUNNING);

	/* Schedulable state. */
	t->state = THREAD_READY;

	if (!WITHIN(t, &idle_threads[0], &idle_threads[KTHREAD_IDLE_MAX]))
	{
		/* Reset age. */
		t->age = 0ULL;

		/* Schedule thread. */
		resource_enqueue(&scheduling, &t->resource);
	}

	/**
	 * Idle thread only reset its age to the limit and not comes to
	 * the schedule list.
	 */
	else
		t->age = THREAD_QUANTUM;
}

/*============================================================================*
 * __thread_prolog_config()                                                   *
 *============================================================================*/

/**
 * @brief Configurates the prolog of the next thread.
 *
 * @details The prolog is used to schedule the previous thread because it cannot
 * schedule/free itself.
 *
 * @param curr Thread that is exiting the core.
 * @param next Thread that will be scheduled.
 */
PRIVATE void __thread_prolog_config(struct thread * curr, struct thread * next)
{
	/* Avoid to set itself. */
	if (curr == next)
		return;

#if __NANVIX_MICROKERNEL_THREAD_STATS

	/* Save execution time. */
	if (curr->stats.exec_start != 0ULL)
	{
		curr->stats.exec_total = (clock_read() - curr->stats.exec_start);
		curr->stats.exec_start = 0ULL;
	}

#endif

	/* Get current core status mode. */
	curr->core_mode = core_status_get_mode();

	/* Current is not in an arrangement, so we need to schedule it. */
	next->resource.next = curr->state != THREAD_SLEEPING ? &curr->resource : NULL;

	/**
	 * Current thread must be freed, so we sinalize that the thread
	 * is in a zombie state.
	 */
	if (UNLIKELY(curr->state == THREAD_TERMINATED))
	{
		KASSERT(curr != next);
		curr->state = THREAD_ZOMBIE;
	}
}

/*============================================================================*
 * __thread_prolog()                                                          *
 *============================================================================*/

/**
 * @brief Configurates the prolog of the next thread.
 *
 * @details The prolog is used to schedule the previous thread because it cannot
 * schedule/free itself.
 *
 * @param curr Newly scheduled thread.
 */
PUBLIC void __thread_prolog(struct thread * curr)
{
	struct section_guard guard; /* Section guard.    */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		/* Verifies if the next thread is zombie */
		struct thread * t = (struct thread *) curr->resource.next;

		/* There is a configurated thread. */
		if (LIKELY(t != NULL))
		{
			/* Schedules the stopped thread. */
			if (t->state == THREAD_STOPPED)
				thread_schedule(t);

			/* Releases the thread resources. */
			else if (t->state == THREAD_ZOMBIE)
				thread_free(t);

			curr->resource.next = NULL;
		}

		/* Get current core status mode. */
		core_status_set_mode(curr->core_mode);

#if __NANVIX_MICROKERNEL_THREAD_STATS

		/* Start measures the execution time. */
		curr->stats.exec_start = clock_read();

#endif

	thread_unlock_tm(&guard);
}

/*============================================================================*
 * thread_yield()                                                             *
 *============================================================================*/

/**
* @brief Release the core to another thread.
*
* @returns Zero if it is a user thread, Non-zero if it is the master.
*/
PUBLIC int thread_yield(void)
{
	int coreid;                 /* Core ID.        */
	struct thread * idle;       /* Schedule queue. */
	struct thread * curr;       /* Current Thread. */
	struct thread * next;       /* Next Thread.    */
	struct section_guard guard; /* Section guard.  */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		curr   = thread_get_curr();
		coreid = core_get_id();
		idle   = KTHREAD_IDLE(coreid);

	/**
	 * Gets next thread.
	 */

		/* Is there any user thread to schedule? */
		if ((next = thread_schedule_next()) != NULL)
		{
			/* Thread not exit the core yet? */
			if (UNLIKELY(next->ctx == NULL))
			{
				thread_schedule(next);
				next = NULL;
			}
		}

		/* Valid next? */
		if  (next)
		{
			if (curr->state == THREAD_RUNNING)
				curr->state = THREAD_STOPPED;
		}

		/* There are no other threads and I can continue. */
		else if (curr->state == THREAD_RUNNING)
		{
			/* Does curr still have an affinity to the underlying core? */
			if (KTHREAD_AFFINITY_MATCH(curr->affinity, (1 << coreid)))
				next = curr;

			/* Lose affinity. */
			else
			{
				curr->state = THREAD_STOPPED;
				next = idle;
			}
		}

		/* I finish and there are no other threads. Switch to idle. */
		else
			next = idle;

	/**
	 * Configure the next thread.
	 */

		__thread_prolog_config(curr, next);
		next->coreid = core_get_id();
		next->state  = THREAD_RUNNING;
		thread_set_curr(next);

	thread_unlock_tm(&guard);

	/* Current context must be NULL before switch to another. */
	KASSERT(curr->ctx == NULL);

		/* Switch context to the new thread. */
		if (curr != next)
			thread_switch_to(&curr->ctx, &next->ctx);

	/* Restore context function must clean ctx variable. */
	KASSERT(curr->ctx == NULL);

	__thread_prolog(curr);

	return (0);
}

/*============================================================================*
 * thread_handler()                                                           *
 *============================================================================*/

/**
 * @brief Handle scheduling kernel events.
 */
PRIVATE void thread_handler(int evnum)
{
	KASSERT(evnum == KEVENT_SCHED);

	thread_yield();
}

/*============================================================================*
 * thread_manager()                                                           *
 *============================================================================*/

/**
 * @brief Node to order threads.
 */
struct tnode {
	struct resource resource;
	struct thread * thread;
};

/**
 * @brief Insert ordered on an arrangement.
 */
PRIVATE int thread_compare_age(struct resource * a, struct resource * b)
{
	uint64_t ta, tb;

	KASSERT(a && b);

	ta = ((struct tnode *) a)->thread->age;
	tb = ((struct tnode *) b)->thread->age;

	if (ta == tb)
		return (0);

	return (ta < tb) ? (1) : (-1);
}

/**
 * @brief Execute schedule algorithm.
 */
PUBLIC void do_thread_schedule(bool is_aging)
{
	int coreid;                         /* Current core ID.    */
	int nodeid;                         /* Current helper ID.  */
	int mycoreid;                       /* Current core ID.    */
	bool do_schedule;                   /* Need to schedule?   */
	struct tnode * older;               /* Older node pointer. */
	struct tnode nodes[CORES_NUM];      /* List Helpers.       */
	struct resource_arrangement olders; /* List of olders.     */

	/* Initialize priority queue. */
	olders   = RESOURCE_ARRANGEMENT_INITIALIZER;
	nodeid   = 0;
	mycoreid = core_get_id();

	do_schedule = (scheduling.size != 0);

	/* Manually call and has no thread to schedule? */
	if (!is_aging && !do_schedule)
		return;

	/* Find the older thread per coreid. */
	for (int i = 0; i < CORES_NUM; ++i)
	{
		/* Update thread age. */
		if (is_aging)
			curr_threads[i]->age++;

		/* Avoid schedule or Young thread. */
		if (!do_schedule || curr_threads[i]->age < THREAD_QUANTUM)
			continue;

		/* Configure the node. */
		nodes[nodeid].resource = RESOURCE_INITIALIZER;
		nodes[nodeid].thread   = curr_threads[i];

		/* Insert ordered. */
		KASSERT(resource_insert_ordered(
			&olders,
			&nodes[nodeid].resource,
			thread_compare_age
		) >= 0);

		/* Next node. */
		nodeid++;
	}

	/* Has any thread waiting? */
	if (do_schedule)
	{
		/* Notify a scheduling event to the older thread. */
		while ((older = (struct tnode *) resource_dequeue(&olders)) != NULL)
		{
#if 0
			/**
			 * If has many syscall request, maybe the master can leave
			 * the dispatcher on starvation.
			 */
			if (older->thread == KTHREAD_MASTER)
				continue;
#endif

			/* Gets the target coreid. */
			coreid = thread_get_coreid(older->thread);

			/* Sets the desired affinity to the target core. */
			thread_desired_affinity = KTHREAD_AFFINITY_FIXED(coreid);

			/* Did find any thread that fit into the target core? */
			if (resource_search_verify(&scheduling, thread_choose) >= 0)
			{
				/* Do not notify itself. */
				if (UNLIKELY(coreid == mycoreid))
				{
					spinlock_unlock(&lock_tm);
						KASSERT(thread_yield() == 0);
					spinlock_lock(&lock_tm);
				}

				/* Notify slave core. */
				else
				{
					KASSERT(kevent_notify(KEVENT_SCHED, coreid) == 0);
				}

				break;
			}
		}
	}
}

/**
 * @brief Manage the thread system.
 */
PUBLIC void thread_manager(void)
{
	/* Lock thread system. */
	spinlock_lock(&lock_tm);

		do_thread_schedule(true);

	/* Release the thread system. */
	spinlock_unlock(&lock_tm);
}

/*============================================================================*
 * Idle Threads Responsibility                                                *
 *============================================================================*/

/*============================================================================*
 * thread_idle()                                                              *
 *============================================================================*/

/**
* @brief Idle thread algorithm.
*
* @details This thread is scheduled when there is no user thread available.
* The idle thread finish when it is wake up but there is no user thread to
* schedule.
*/
PUBLIC void thread_idle(void)
{
	struct thread * idle;       /* Idle Thread.   */
	struct section_guard guard; /* Section guard. */

	idle = thread_get_curr();

	KASSERT(WITHIN(idle, &idle_threads[0], &idle_threads[KTHREAD_IDLE_MAX]));

	interrupts_enable();

	if (LIKELY(thread_get_coreid(idle) != COREID_MASTER))
		interrupt_mask(INTERRUPT_TIMER);

	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		/* Lifecycle of idle thread. */
		while (LIKELY(!tm_shutdown))
		{
			thread_unlock_tm(&guard);
				kevent_wait(KEVENT_WAKEUP);
			thread_lock_tm(&guard);
		}

	thread_unlock_tm(&guard);

	/* Indicates that the underlying core will be reset. */
	KASSERT(core_release() == 0);

		thread_lock_tm(&guard);
			thread_free(idle);
			cond_broadcast(&joincond[KERNEL_THREAD_ID(idle)]);
		thread_unlock_tm(&guard);

	/* No rollback after this point. */
	/* Resets the underlying core. */
	core_reset();

	/* Never gets here. */
	UNREACHABLE();
}

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
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC NORETURN void thread_exit(void *retval)
{
	struct thread * curr;
	struct section_guard guard; /* Section guard.    */
	int * ret = NULL;
	/* Gets current thread information. */
	curr = thread_get_curr();

	/* Do not get scheduled because it will exit the core. */
	interrupts_disable();

	/* Valid thread. */
	/* @TODO Do we need sure that only user thread will call thread_exit? */
	KASSERT(WITHIN(curr, &user_threads[0], &user_threads[THREAD_MAX]));

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	/* Notifies thread exit. */
	thread_lock_tm(&guard);

		/* Saves the retval of current thread. */
		thread_save_retval(retval, curr);
		
		/* Release thread key related values */
		
		thread_key_exit(curr->tid, ret);

		/**
		 * To schedule another user thread without use idle thread has
		 * intermediate, we need to indicate that the current thread will be
		 * finished. So, setting the state to THREAD_TERMINATED, we can release
		 * the thread structure inside the thread_yield where we schedule
		 * another thread directly.
		 */
		curr->state = THREAD_TERMINATED;

		/* Notifies thread exit. */
		cond_broadcast(&joincond[KERNEL_THREAD_ID(curr)]);

	thread_unlock_tm(&guard);

	/* Switch to another thread. */
	thread_yield();

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
	int _tid;                   /* Unique thread identifier. */
	int utid;                   /* Kernel thread ID.         */
	struct stack * ustack;      /* User stack pointer.       */
	struct stack * kstack;      /* Kernel stack pointer.     */
	struct thread * idle;       /* Idle thread.              */
	struct thread * new_thread; /* New thread.               */
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
			goto error0;
		}

		/* Allocate stacks to the thread. */
		if ((kstack = (struct stack *) kpage_get(1)) == NULL)
		{
			kprintf("[pm] cannot create kernel stack");
			goto error1;
		}

		if ((ustack = (struct stack *) kpage_get(1)) == NULL)
		{
			kprintf("[pm] cannot create user stack");
			kpage_put((void *) kstack);
			goto error1;
		}

		/* Get thread ID. */
		_tid = next_tid++;
		utid = KTHREAD_USER_ID(new_thread);

		/* Initialize thread structure. */
		new_thread->tid      = _tid;
		new_thread->arg      = arg;
		new_thread->start    = start;

		/**
		 * Indicate the first core based on the offset of thread user thread.
		 *
		 * Range of cores: [1, (CORES_NUM - 1)]
		 *
		 * @details Its is greater than 0 because the core 0 is reserved to the
		 * master thread.
		 */
		KASSERT((new_thread->coreid = (utid % (CORES_NUM - 1)) + 1) > 0);

		/* Sets default affinity. */
#if __NANVIX_MICROKERNEL_STATIC_SCHED
		new_thread->affinity = KTHREAD_AFFINITY_FIXED(new_thread->coreid);
#else
		new_thread->affinity = KTHREAD_AFFINITY_DEFAULT;
#endif

#if __NANVIX_MICROKERNEL_THREAD_STATS
		new_thread->stats.exec_start = 0ULL;
		new_thread->stats.exec_total = 0ULL;
#endif

		/* Store reference to the stacks of the thread. */
		ustacks[utid] = ustack;
		kstacks[utid] = kstack;

		/* Create initial context of the thread. */
		KASSERT((new_thread->ctx = context_create(thread_start, ustack, kstack)) != NULL);

		/* Puts thread in the schedule queue. */
		thread_schedule(new_thread);

		/* Is the Idle thread running? */
		idle = KTHREAD_IDLE(new_thread->coreid);
		if (idle->state == THREAD_RUNNING)
			kevent_notify(KEVENT_SCHED, idle->coreid);

	thread_unlock_tm(&guard);

	/* Save thread ID. */
	if (tid != NULL)
	{
		*tid = _tid;
		dcache_invalidate();
	}

	return (0);

error1:
		new_thread->state = THREAD_ZOMBIE;
		thread_free(new_thread);
error0:
	thread_unlock_tm(&guard);

	return (-EAGAIN);
}

/*============================================================================*
 * Thread Manager Initialization                                              *
 *============================================================================*/

/**
 * @name Imported definitions.
 */
/**@{*/
EXTERN void _kmain(void);
/**@}*/

/**
 * @brief Main wrapper.
 *
 * @details We must execute the prolog after yield.
 */
PRIVATE void _kmain_wrapper(void)
{
	__thread_prolog(thread_get_curr());

	_kmain();
}

/*============================================================================*
 * __thread_init()                                                            *
 *============================================================================*/

/**
 * @brief Initialize thread system.
 */
PUBLIC void __thread_init(void)
{
	int ret;
	int ntrials;
	struct thread * idle;

	/* Sanity checks. */
	KASSERT(KTHREAD_IDLE_MAX == CORES_NUM);
	KASSERT(nthreads == KTHREAD_SERVICE_MAX);

	/* Configure schedule queues. */
	scheduling = RESOURCE_ARRANGEMENT_INITIALIZER;

	/* Set schedule handler. */
	KASSERT(kevent_set_handler(KEVENT_SCHED, thread_handler) == 0);

	/* Spawn idle threads. */
	for (int coreid = 0; coreid < KTHREAD_IDLE_MAX; coreid++)
	{
		/* Thread should be the same of the getted by the macro. */
		KASSERT((idle = KTHREAD_IDLE(coreid)) != NULL);

		/* Sanity checks. */
		KASSERT(idle == &threads[KTHREAD_SERVICE_MAX + coreid]);
		KASSERT(idle == &idle_threads[coreid]);

		/* Initialize thread structure. */
		idle->tid           = next_tid++;
		idle->coreid        = coreid;
		idle->state         = THREAD_RUNNING;
		idle->affinity      = KTHREAD_AFFINITY_FIXED(coreid);
		idle->age           = THREAD_QUANTUM;
		idle->arg           = NULL;
		idle->start         = (void *(*)(void*)) thread_idle;
		idle->resource.next = NULL;

		/* Sets running thread. */
		curr_threads[coreid] = idle;

		/* Affinity should only be with the coreid and tid must be pre-known. */
		KASSERT(idle->affinity == (1 << coreid));
		KASSERT(idle->tid      == (KTHREAD_SERVICE_MAX + coreid));

		/* Core master is already running the idle thread. */
		if (coreid == COREID_MASTER)
			continue;

		/*
		 * We should do some busy waitting here. When the kernel is under
		 * stress, there is a chance that we allocate a core that is in
		 * RUNNING state. That happens because a previous thread running
		 * on this core has existed and we have joined it, but the
		 * terminated thread hasn't had enough time to issue issue a
		 * core_reset().
		 */
		ntrials = 0;
		do
		{
			ret = core_start(coreid, thread_idle);
			ntrials++;
		} while (ret == -EBUSY && ntrials < THREAD_CREATE_NTRIALS);

		/* Idle successfuly created. */
		KASSERT(ret == 0);
	}

	/**
	 * Create master thread.
	 */

		/* Gets the dispatcher thread. */
		struct thread * master = KTHREAD_MASTER;

#if __NANVIX_MICROKERNEL_THREAD_STATS
		master->stats.exec_start = 0ULL;
		master->stats.exec_total = 0ULL;
#endif

		/* Initialize thread structure. */
		KASSERT(master->coreid   == COREID_MASTER);
		KASSERT(master->affinity == KTHREAD_AFFINITY_MASTER);

		/* Allocate stacks to the thread. */
		KASSERT((ustacks[KSTACK_MAX - 1] = (struct stack *) kpage_get(1)) != NULL);
		KASSERT((kstacks[KSTACK_MAX - 2] = (struct stack *) kpage_get(1)) != NULL);

		/* Create initial context of the thread. */
		KASSERT((master->ctx =
			context_create(
				_kmain_wrapper,
				ustacks[KSTACK_MAX - 1],
				kstacks[KSTACK_MAX - 2]
			)
		) != NULL);

		/* Puts thread in the schedule queue. */
		thread_schedule(master);

}

#endif /* CLUSTER_IS_MULTICORE && CORE_SUPPORTS_MULTITHREADING */

