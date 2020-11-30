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
 * @name stacks.
 */
/**@{*/
PRIVATE struct stack *ustacks[THREAD_MAX];
PRIVATE struct stack *kstacks[THREAD_MAX];
/**@}*/

/*============================================================================*
 * Scheduler variables                                                        *
 *============================================================================*/

/**
 * @name Gets idle thread based on coreid.
 */
/**@{*/
#define IDLE_THREAD_ID(_t)   (_t - idle_threads)
#define USER_THREAD_ID(_t)   (_t - user_threads)
/**@}*/

/**
 * @brief Number of idle threads.
 *
 * @details Master + Idles == SYS_THREAD_MAX.
 */
#define IDLE_THREAD_MAX (SYS_THREAD_MAX - 1)

/**
 * @brief Scheduler queue per core/idle thread.
 *
 * @details The next pointer of the idle thread is used
 * to store the schedule queue of each core.
 * Id 0 is not an idle thread but is the initial thread
 * because the correspondence of the core ID with that of
 * the thread.
 */
PRIVATE struct thread * idle_threads = (KTHREAD_MASTER + 1);
PRIVATE struct thread * user_threads = (KTHREAD_MASTER + SYS_THREAD_MAX);

/**
 * @brief Schedule queues.
 */
PRIVATE struct resource_arrangement schedules;

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

	utid = USER_THREAD_ID(t);
	kpage_put((void *) ustacks[utid]);
	kpage_put((void *) kstacks[utid]);
	ustacks[utid] = NULL;
	kstacks[utid] = NULL;
}

/*============================================================================*
 * Scheduling functions                                                       *
 *============================================================================*/

/*============================================================================*
 * thread_switch_to()                                                         *
 *============================================================================*/

/**
 * Switch between contexts.
 */
PRIVATE int thread_switch_to(struct context ** previous, struct context ** next)
{
	/* Invalid thread. */
	if (thread_get_curr_id() == KTHREAD_MASTER_TID)
		return (-EINVAL);

	/* Does a thread try to switch to itself? */
	if (previous == next)
		return (-EINVAL);

	/* Invalid previous. */
	if ((previous == NULL) || (!mm_is_kaddr(VADDR(previous))))
		return (-EINVAL);

	/* Will previous be overwritten? */
	if (*previous != NULL)
		return (-EINVAL);

	/* Invalid next. */
	if ((next == NULL) || (!mm_is_kaddr(VADDR(next))))
		return (-EINVAL);

	/* Invalid next context. */
	if ((*next == NULL) || (!mm_is_kaddr(VADDR(*next))))
		return (-EINVAL);

	return (context_switch_to(previous, next));
}

/*============================================================================*
 * thread_schedule()                                                          *
 *============================================================================*/

/**
* @brief Insert a new thread into a schedule queue.
*
* @param t New thread to insert into @idle queue.
*/
PUBLIC void thread_schedule(struct thread * t)
{
	/* Valid user thread. */
	KASSERT(WITHIN(t, &user_threads[0], &user_threads[THREAD_MAX]));

	/* Reconfigure the thread. */
	t->age    = 0ULL;
	t->state  = THREAD_READY;
	t->coreid = -1;

	/* Enqueue new thread. */
	resource_enqueue(&schedules, &t->resource);
}

/*============================================================================*
 * thread_schedule_next()                                                     *
 *============================================================================*/

/**
* @brief Gets the next user thread to be scheduled.
*
* @returns Valid thread pointer if there is user threads ready to be scheduled,
* NULL otherwise.
*/
PRIVATE struct thread * thread_schedule_next(void)
{
	return ((struct thread *) (resource_dequeue(&schedules)));
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
	struct thread * idle;       /* Schedule queue. */
	struct thread * curr;       /* Current Thread. */
	struct thread * next;       /* Next Thread.    */
	struct section_guard guard; /* Section guard.  */

	curr = thread_get_curr();

	/* Kernel thread do not yield. */
	if (thread_get_id(curr) == KTHREAD_MASTER_TID)
		return (-EINVAL);

	idle = IDLE_THREAD(thread_get_coreid(curr));

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

	/**
	 * Gets next thread.
	 */

		/* Is there any user thread to schedule? */
		if  ((next = thread_schedule_next()) != NULL)
		{
			if (curr->state == THREAD_RUNNING)
			{
				/* Update thread states. */
				curr->state = THREAD_STOPPED;

				/* Make user thread schedulable. */
				if (curr != idle)
					thread_schedule(curr);
			}
		}

		/* There are no other threads and I can continue. */
		else if (curr->state == THREAD_RUNNING)
			next = curr;

		/* I finish and there are no other threads. Switch to idle. */
		else
			next = idle;

	/**
	 * Release terminated thread (@see thread_exit).
	 */

		if (curr->state == THREAD_TERMINATED)
		{
			/* Sanity check. */
			KASSERT(curr != idle && curr != next);

			/* Put the thread to zombie. */
			curr->state         = THREAD_ZOMBIE;
			next->resource.next = &curr->resource;
		}

	/**
	 * Set next thread to running state.
	 */

		next->coreid               = core_get_id();
		next->state                = THREAD_RUNNING;
		curr_threads[next->coreid] = next;

	thread_unlock_tm(&guard);

	/* Current context must be NULL before switch to another. */
	KASSERT(curr->ctx == NULL);

		/* Switch context to the new thread. */
		if (curr != next)
			thread_switch_to(&curr->ctx, &next->ctx);

	/* Restore context function must clean ctx variable. */
	KASSERT(curr->ctx == NULL);

	thread_lock_tm(&guard);

		/* Verifies if the next thread is zombie */
		struct thread * zombie = (struct thread *) curr->resource.next;

		if (zombie && zombie->state == THREAD_ZOMBIE)
			thread_free(zombie);

		curr->resource.next = NULL;

	thread_unlock_tm(&guard);

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

	return (ta < tb) ? (-1) : (1);
}

/**
 * @brief Manage the thread system.
 */
PUBLIC void thread_manager(void)
{
	int coreid;
	int nodeid;
	struct tnode * older;
	struct tnode nodes[CORES_NUM];
	struct resource_arrangement olders;

	/* Initialize priority queue. */
	olders = RESOURCE_ARRANGEMENT_INITIALIZER;
	nodeid = 0;

	spinlock_lock(&lock_tm);

		/* Find the older thread per coreid. */
		for (int i = 1; i < CORES_NUM; ++i)
		{
			/* Update thread age. */
			curr_threads[i]->age++;

			/* Young thread. */
			if (curr_threads[i]->age < THREAD_QUANTUM)
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

		/* Get older thread. */
		while ((older = (struct tnode *) resource_dequeue(&olders)) != NULL)
		{
			coreid = thread_get_coreid(older->thread);

			/* Has any thread waiting? */
			if (schedules[coreid].size)
			{
				/* Notify scheduling event. */
				KASSERT(kevent_notify(KEVENT_SCHED, coreid) == 0);

				/* Schedule only one thread per management. */
				break;
			}
		}

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
PRIVATE NORETURN void thread_idle(void)
{
	struct thread * idle;       /* Idle Thread.   */
	struct section_guard guard; /* Section guard. */

	idle = thread_get_curr();

	KASSERT(WITHIN(idle, &idle_threads[0], &idle_threads[IDLE_THREAD_MAX]));

	interrupts_enable();

	if (LIKELY(thread_get_coreid(idle) != COREID_MASTER))
		interrupt_mask(INTERRUPT_TIMER);

	section_guard_init(&guard, &lock_tm, INTERRUPT_LEVEL_NONE);

	thread_lock_tm(&guard);

		/* Lifecycle of idle thread. */
		while (UNLIKELY(!tm_shutdown))
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
		utid = USER_THREAD_ID(new_thread);

		/* Initialize thread structure. */
		new_thread->tid   = _tid;
		new_thread->arg   = arg;
		new_thread->start = start;

		/* Static put the thread on a coreid. */
		KASSERT((new_thread->coreid = (utid % IDLE_THREAD_MAX) + 1) > 0);

		/* Store reference to the stacks of the thread. */
		ustacks[utid] = ustack;
		kstacks[utid] = kstack;

		/* Create initial context of the thread. */
		KASSERT((new_thread->ctx = context_create(thread_start, ustack, kstack)) != NULL);

		/* Puts thread in the schedule queue. */
		thread_schedule(new_thread);

		/* Is the Idle thread running? */
		idle = IDLE_THREAD(new_thread->coreid);
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
	KASSERT(IDLE_THREAD_MAX == (SYS_THREAD_MAX - 1));
	KASSERT(IDLE_THREAD_MAX == (CORES_NUM - 1));
	KASSERT(THREAD_MAX == THREAD_MAX);
	KASSERT(nthreads == 1);

	/* Initialize the schedule queue. */
	schedules = RESOURCE_ARRANGEMENT_INITIALIZER;

	/* Set schedule handler. */
	KASSERT(kevent_set_handler(KEVENT_SCHED, thread_handler) == 0);

	/* Spawn idle threads. */
	for (int coreid = 1; coreid <= IDLE_THREAD_MAX; coreid++)
	{
		KASSERT((idle = thread_alloc()) != NULL);

		/* Get thread ID. */
		KASSERT((idle->tid = next_tid++) == coreid);

		/* Initialize thread structure. */
		idle->coreid        = coreid;
		idle->state         = THREAD_RUNNING;
		idle->age           = THREAD_QUANTUM;
		idle->arg           = NULL;
		idle->start         = (void *(*)(void*)) thread_idle;
		idle->resource.next = NULL;

		/* Sets running thread. */
		curr_threads[coreid] = idle;

		/* Thread id must be the same of coreid. */
		KASSERT(KERNEL_THREAD_ID(idle) == thread_get_coreid(idle));

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
}

#endif /* CLUSTER_IS_MULTICORE && CORE_SUPPORTS_MULTITHREADING */

