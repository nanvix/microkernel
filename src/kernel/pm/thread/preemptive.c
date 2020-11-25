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

#if CORE_SUPPORTS_MULTITHREADING

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
		.ctx = NULL,
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
 * @brief Indicates for idle threads to exit.
 */
PRIVATE volatile int tm_shutdown = 0;

/**
 * @brief Thread manager lock.
 */
PRIVATE spinlock_t lock_tm = SPINLOCK_UNLOCKED;

/*============================================================================*
 * Scheduler variables                                                        *
 *============================================================================*/

/**
 * @name Gets idle thread based on coreid.
 */
/**@{*/
#define KERNEL_THREAD_ID(_t) (_t - threads)
#define IDLE_THREAD_ID(_t)   (_t - idle_threads)
#define USER_THREAD_ID(_t)   (_t - user_threads)
/**@}*/

/**
 * @brief Gets idle thread pointer.
 *
 * @warning Not use core 0 because it is not a idle thread.
 */
#define IDLE_THREAD(coreid) (&threads[coreid])

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
 * @name stacks.
 */
/**@{*/
PRIVATE struct stack *ustacks[THREAD_MAX];
PRIVATE struct stack *kstacks[THREAD_MAX];
/**@}*/

/**
 * @brief Schedule queue.
 */
PRIVATE struct schedule_queue
{
	size_t size;
	struct thread * head;
	struct thread * tail;
	struct semaphore sem;
} sched_queue = {
	.size = 0,
	.head = NULL,
	.tail = NULL,
	.sem  = SEMAPHORE_INITIALIZER(0)
};

/**
 * @brief Fence to synchronize the idle thread in the initialization.
 */
PRIVATE spinlock_t idle_fence;

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
	int mycoreid;         /* Core ID.        */
	struct thread * curr; /* Current Thread. */

	curr     = NULL;
	mycoreid = core_get_id();

	for (int i = 0; i < KTHREAD_MAX; i++)
	{
		if (threads[i].coreid != mycoreid)
			continue;

		/* Found. */
		if (threads[i].state == THREAD_RUNNING)
			return (&threads[i]);

		/* Possible. */
		if (threads[i].state == THREAD_TERMINATED)
			curr = (&threads[i]);
	}

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
PRIVATE struct thread * thread_get(int tid)
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
	int utid;
	KASSERT(WITHIN(t, &threads[0], &threads[KTHREAD_MAX]));
	KASSERT(t->state == THREAD_ZOMBIE);
	utid = USER_THREAD_ID(t);

	kpage_put((void *)ustacks[utid]);
	kpage_put((void *)kstacks[utid]);
	ustacks[utid] = NULL;
	kstacks[utid] = NULL;

	t->coreid = -1;
	t->state  = THREAD_NOT_STARTED;
	t->tid    = KTHREAD_NULL_TID;
	nthreads--;
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
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PRIVATE struct thread * thread_alloc(void)
{
	for (int i = 1; i < KTHREAD_MAX; i++)
	{
		/* Verify the state of the thread. */
		switch (threads[i].state)
		{
			/* Found a free thread. */
			case THREAD_NOT_STARTED:
				nthreads++;
				break;

			/* Found a zombie thread (frees used kpages). */
			case THREAD_ZOMBIE:
				thread_free(&threads[i]);
				break;

			/* Skip busy thread. */
			default:
				continue;
		}

		/* Initializes chosen thread. */
		threads[i].state = THREAD_STARTED;

		return (&threads[i]);
	}

	/* All threads are in use. */
	return (NULL);
}

/*============================================================================*
 * thread_schedule()                                                          *
 *============================================================================*/

/**
* @brief Insert a new thread into a schedule queue.
*
* @param new_thread New thread to insert into @idle queue.
*/
PRIVATE void thread_schedule(struct thread * new_thread)
{
	/* Valid user thread. */
	KASSERT(WITHIN(new_thread, &user_threads[0], &user_threads[THREAD_MAX]));

	/* Is the queue empty? Uodate the head pointer. */
	if (sched_queue.size == 0)
		sched_queue.head = new_thread;

	/* Update next pointer of the tail. */
	else
		sched_queue.tail->next = new_thread;

	/* Update tail pointer. */
	sched_queue.tail   = new_thread;
	new_thread->next   = NULL;
	new_thread->coreid = -1;

	/* Counts the new thread. */
	sched_queue.size++;

	/* Notifies that is a new thread available. */
	semaphore_up(&sched_queue.sem);
}

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
 * thread_yield()                                                             *
 *============================================================================*/

/**
* @brief Release the core to another thread.
*
* @returns Zero if it is a user thread, Non-zero if it is the master.
*/
PUBLIC int thread_yield(void)
{
	struct thread * idle; /* Schedule queue. */
	struct thread * curr; /* Current Thread. */
	struct thread * next; /* Next Thread.    */

	curr = thread_get_curr();

	/* Kernel thread do not yield. */
	if (thread_get_id(curr) == KTHREAD_MASTER_TID)
		return (-EINVAL);

	idle = IDLE_THREAD(thread_get_coreid(curr));

	spinlock_lock(&lock_tm);

	/**
	 * Gets next thread.
	 */

		/* Is there any user thread to schedule? */
		if (sched_queue.size)
		{
			/* Gets next thread. */
			next = sched_queue.head;

			/* Consume user thread. */
			sched_queue.head = next->next;
			sched_queue.size--;

			/* Configure the new htread. */
			next->next   = NULL;
			next->coreid = core_get_id();

			if (curr->state != THREAD_TERMINATED)
			{
				/* Update thread states. */
				curr->state = THREAD_STOPPED;

				/* Make user thread schedulable. */
				if (curr != idle)
					thread_schedule(curr);
			}
		}

		/* There are no other threads and I can continue. */
		else if (curr->state != THREAD_TERMINATED)
			next = curr;

		/* I finish and the are no other threads. Switch to idle. */
		else
			next = idle;

		/* Make current a zombie thread */
		if (curr->state == THREAD_TERMINATED)
		{
			/* Sanity check. */
			KASSERT(curr != idle && curr != next);

			curr->state = THREAD_ZOMBIE;
			next->next = curr;
		}

	/**
	 * Set next thread to running state.
	 */

		next->state = THREAD_RUNNING;

	spinlock_unlock(&lock_tm);

	/* Current context must be NULL before switch to another. */
	KASSERT(curr->ctx == NULL);

		/* Switch context to the new thread. */
		if (curr != next)
			thread_switch_to(&curr->ctx, &next->ctx);

	/* Restore context function must clean ctx variable. */
	KASSERT(curr->ctx == NULL);

	spinlock_lock(&lock_tm);

		/* Verifies if the next thread is zombie */
		if (curr->next && curr->next->state == THREAD_ZOMBIE)
		{
			thread_free(curr->next);
			curr->next = NULL;
		}

	spinlock_unlock(&lock_tm);

	return (0);
}
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
	struct thread * idle; /* Idle Thread.    */

	idle = thread_get_curr();

	KASSERT(WITHIN(idle, &idle_threads[0], &idle_threads[IDLE_THREAD_MAX]));

	/* Ensure that the first thread will get the fisrt user thread. */
	if (idle == &idle_threads[0])
	{
		/* Waits a thread be available. */
		semaphore_down(&sched_queue.sem);

		/* Release fence.. */
		spinlock_unlock(&idle_fence);

		/* Schedule user thread. */
		KASSERT(thread_yield() == 0);
	}

	/* Waits first thread. */
	else
	{
		spinlock_lock(&idle_fence);
		spinlock_unlock(&idle_fence);
	}

	/* Lifecycle of idle thread. */
	while (!tm_shutdown)
	{
		/* Waits a thread be available. */
		semaphore_down(&sched_queue.sem);

		/* Schedule user thread. */
		KASSERT(thread_yield() == 0);
	}

	/* Indicates that the underlying core will be reset. */
	KASSERT(core_release() == 0);

		spinlock_lock(&lock_tm);
			idle->state = THREAD_ZOMBIE;
			cond_broadcast(&joincond[KERNEL_THREAD_ID(idle)]);
		spinlock_unlock(&lock_tm);

	/* No rollback after this point. */
	/* Resets the underlying core. */
	core_reset();

	/* Never gets here. */
	UNREACHABLE();
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
 * @author Pedro Henrique Penna and João Vicente Souto
 */
PUBLIC NORETURN void thread_exit(void *retval)
{
	struct thread * curr;

	/* Gets current thread information. */
	curr = thread_get_curr();

	/* Valid thread. */
	/* @TODO Do we need sure that only user thread will call thread_exit? */
	KASSERT(WITHIN(curr, &user_threads[0], &user_threads[THREAD_MAX]));

	/* Notifies thread exit. */
	spinlock_lock(&lock_tm);

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

		/* Notify waiting threads. */
		cond_broadcast(&joincond[KERNEL_THREAD_ID(curr)]);

	spinlock_unlock(&lock_tm);

	/* Switch to thread. */
	thread_yield();

	/* Never gets here. */
	UNREACHABLE();
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

	spinlock_lock(&lock_tm);

		/* Verifies if the next thread is zombie */
		if (curr_thread->next && curr_thread->next->state == THREAD_ZOMBIE)
		{
			thread_free(curr_thread->next);
			curr_thread->next = NULL;
		}

	spinlock_unlock(&lock_tm);

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
	int _tid;                   /* Unique thread identifier. */
	int utid;                   /* Kernel thread ID.         */
	struct stack * ustack;      /* User stack pointer.       */
	struct stack * kstack;      /* Kernel stack pointer.     */
	struct thread *new_thread;  /* New thread.               */

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

		/* Allocate stacks to the thread. */
		if ((kstack = (struct stack *) kpage_get(1)) == NULL)
		{
			kprintf("[pm] cannot create kernel stack");
			spinlock_unlock(&lock_tm);
			return (-EAGAIN);
		}

		if ((ustack = (struct stack *) kpage_get(1)) == NULL)
		{
			kprintf("[pm] cannot create user stack");
			kpage_put((void *) kstack);
			spinlock_unlock(&lock_tm);
			return (-EAGAIN);
		}

		/* Get thread ID. */
		_tid = next_tid++;
		utid = USER_THREAD_ID(new_thread);

		/* Initialize thread structure. */
		new_thread->tid   = _tid;
		new_thread->arg   = arg;
		new_thread->start = start;
		new_thread->next  = NULL;

		ustacks[utid] = ustack;
		kstacks[utid] = kstack;
		/* Create initial context of the thread. */
		KASSERT((new_thread->ctx = context_create(thread_start, ustack, kstack)) != NULL);

		/* Puts thread in the schedule queue. */
		thread_schedule(new_thread);

	spinlock_unlock(&lock_tm);

	/* Save thread ID. */
	if (tid != NULL)
	{
		*tid = _tid;
		dcache_invalidate();
	}

	return (0);
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

	KASSERT(tid != KTHREAD_MASTER_TID); //! @TODO Can idle threads joinable?

	spinlock_lock(&lock_tm);

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
				cond_wait(&joincond[KERNEL_THREAD_ID(t)], &lock_tm);
		}

		/**
		 * Thread IDs are incremented by next_id. So, if we want to know
		 * if the @p tid is valid and has already left, just check if it
		 * is less than the next_tid.
		 */
		ret = (tid < next_tid) ? 0 : (-EINVAL);

		/**
		 * This prevents the thread from returning an invalid value.
		 * This if is used guarante that the the @p ret is valid
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
 * @brief Initialize thread system.
 */
PUBLIC void thread_init(void)
{
#if (CLUSTER_IS_MULTICORE)

	int ret;
	int ntrials;
	struct thread * idle;

	/* Sanity checks. */
	KASSERT(IDLE_THREAD_MAX == (SYS_THREAD_MAX - 1));
	KASSERT(IDLE_THREAD_MAX == (CORES_NUM - 1));
	KASSERT(THREAD_MAX == THREAD_MAX);
	KASSERT(nthreads == 1);

	/* initialize user and kernel stack pointers */
	for (int i = 0; i < THREAD_MAX; i++)
	{
		ustacks[i] = NULL;
		kstacks[i] = NULL;
	}

	/* Initialize the fence for idle threads. */
	spinlock_init(&idle_fence);
	spinlock_lock(&idle_fence);

	for (int coreid = 1; coreid <= IDLE_THREAD_MAX; coreid++)
	{
		KASSERT((idle = thread_alloc()) != NULL);

		/* Get thread ID. */
		KASSERT((idle->tid = next_tid++) == coreid);

		/* Initialize thread structure. */
		idle->coreid = coreid;
		idle->state  = THREAD_RUNNING;
		idle->arg    = NULL;
		idle->next   = NULL;
		idle->start  = (void *(*)(void*)) thread_idle;

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

#endif /* CLUSTER_IS_MULTICORE */
}

#endif /* CORE_SUPPORTS_MULTITHREADING */
