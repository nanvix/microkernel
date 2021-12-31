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

/**
 * @defgroup kernel- Thread System
 * @ingroup kernel
 *
 * @brief Thread System
 */

#ifndef NANVIX_THREAD_H_
#define NANVIX_THREAD_H_

	/* Must come first. */
	#define __NEED_RESOURCE

	/* External dependencies. */
	#include <nanvix/hal.h>
	#include <nanvix/hal/resource.h>
	#include <nanvix/kernel/mm.h>
	#include <nanvix/kernel/config.h>
	#include <nanvix/const.h>

/*============================================================================*
 *                                Thread System                               *
 *============================================================================*/

	/**
	 * @name Execution Parameters.
	 */
	/**@{*/
	#ifndef __NANVIX_MICROKERNEL_DYNAMIC_SCHED
	#define __NANVIX_MICROKERNEL_DYNAMIC_SCHED 0
	#endif
	#ifndef __NANVIX_MICROKERNEL_THREAD_STATS
	#define __NANVIX_MICROKERNEL_THREAD_STATS 0
	#endif
	/**@}*/

	/**
	 * @brief Kernel thread dedicated to kernel services.
	 */
	#if __NANVIX_USE_TASKS
		#define KTHREAD_SERVICE_MAX (2) /**< Master + Dispatcher thread. */
	#else
		#define KTHREAD_SERVICE_MAX (1) /**< Master thread.              */
	#endif

	/**
	 * @brief Idle thread dedicated to occupy the idle core.
	 *
	 * @details One master thread to respond syscall requests plus
	 * (CORES_NUM - 1) idle threads to occupy the core idle time.
	 */
	#if CORE_SUPPORTS_MULTITHREADING
		#define KTHREAD_IDLE_MAX (CORES_NUM) /**< CORES_NUM - Master thread. */
	#else
		#define KTHREAD_IDLE_MAX (0)         /**< There is not idle thread.  */
	#endif

	/**
	 * @brief Maximum number of system threads.
	 */
	#define SYS_THREAD_MAX (KTHREAD_SERVICE_MAX + KTHREAD_IDLE_MAX)

	/**
 	* @brief Size of the buffer with exiting values.
 	*/
	#define KTHREAD_EXIT_VALUE_NUM (32)

	/**
	 * @brief Maximum number of user threads.
	 */
	#if CORE_SUPPORTS_MULTITHREADING && defined(__mppa256__)
		#ifdef __k1bio__
			#define THREAD_MAX (8 - KTHREAD_SERVICE_MAX)  /**< Reserved 8 kernel pages. */
		#else
			#define THREAD_MAX ((NUM_KPAGES / 2) - KTHREAD_SERVICE_MAX) /**< Reserved 24 kernel pages.*/
		#endif
	#elif CORE_SUPPORTS_MULTITHREADING && !defined(__mppa256__)
		#define THREAD_MAX (2 * (SYS_THREAD_MAX - 1))
	#else
		#define THREAD_MAX (CORES_NUM - SYS_THREAD_MAX)
	#endif

	/**
	 * @brief Maximum number of system threads.
	 */
	#define KTHREAD_MAX (SYS_THREAD_MAX + THREAD_MAX)

	/**
	 * @brief Number of clock cycles per thread.
	 */
	#define THREAD_QUANTUM (128)

	/**
	 * @name Thread States
	 */
	/**@{*/
	#define THREAD_NOT_STARTED 0 /**< Not Started */
	#define THREAD_READY       1 /**< Started     */
	#define THREAD_RUNNING     2 /**< Running     */
	#define THREAD_SLEEPING    3 /**< Sleeping    */
	#define THREAD_STOPPED     4 /**< Stopped     */
	#define THREAD_TERMINATED  5 /**< Terminated  */
	#define THREAD_ZOMBIE      6 /**< Zombie      */
	/**@}*/

	/**
	 * @name Affinity.
	 */
	/**@{*/
	#define KTHREAD_AFFINITY_SET           ((1 << CORES_NUM) - 1)                            /**< Mask of cores set.             */
	#define KTHREAD_AFFINITY_MASTER        (1 << COREID_MASTER)                              /**< Master thread affinity.        */
	#define KTHREAD_AFFINITY_DEFAULT       (KTHREAD_AFFINITY_SET & ~KTHREAD_AFFINITY_MASTER) /**< Default user affinity.         */
	#define KTHREAD_AFFINITY_IS_VALID(aff) (aff & KTHREAD_AFFINITY_SET)                      /**< Valid affinity checker.        */
	#define KTHREAD_AFFINITY_FIXED(coreid) (1 << coreid)                                     /**< Affinity related to a coreid.  */
	#define KTHREAD_AFFINITY_MATCH(a, b)   (a & b)                                           /**< Similarity between affinities. */
	/**@}*/

	/**
	 * @name Features
	 */
	/**@{*/
	#define KERNEL_THREAD_BAD_START 0 /**< Check for bad thread start routine? */
	#define KERNEL_THREAD_BAD_ARG   0 /**< Check for bad thread argument?      */
	#define KERNEL_THREAD_BAD_JOIN  0 /**< Check for bad thread join?          */
	#define KERNEL_THREAD_BAD_EXIT  0 /**< Check for bad thread exit?          */
	/**@}*/

	/**
	 * @name Stats options.
	 */
	/**@{*/
	#define KTHREAD_STATS_EXEC_TIME 0
	/**@}*/

#if __NANVIX_MICROKERNEL_THREAD_STATS

	/**
	 * @brief Thread statistics.
	 */
	struct thread_stats
	{
		uint64_t exec_start;
		uint64_t exec_total;
	};

#endif

	/**
	 * @brief Thread.
	 */
	struct thread
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource; /**< Generic resource information. */

		/**
		 * @name Control variables.
		 */
		/**@{*/
		int tid;                   /**< Thread ID.                   */
		short coreid;              /**< Core ID.                     */
		short state;               /**< State.                       */
		int affinity;              /**< Affinity.                    */
		uint64_t age;              /**< Age.                         */
		/**@}*/

		/**
		 * @name Core status.
		 *
		 * @see struct core_status
		 */
		/**@{*/
		short core_mode;           /**< Mode.                        */
		/**@}*/

		/**
		 * @name Arguments and functions.
		 */
		/**@{*/
		void *arg;                 /**< Argument.                    */
		void *(*start)(void*);     /**< Starting routine.            */
		/**@}*/

		/**
		 * @name Scheduling variables.
		 *
		 * @details Resource attribute is used by the scheduler.
		 */
		/**@{*/
		struct context * ctx;      /**< Preempted context.           */
		/**@}*/

#if __NANVIX_MICROKERNEL_THREAD_STATS
		/**
		 * @name Statistics variables.
		 */
		/**@{*/
		struct thread_stats stats; /**< Statistics.                  */
		/**@}*/
#endif

	} ALIGN(CACHE_LINE_SIZE);

	/**
	 * @brief Thread's exit value
	 */
	struct exit_value
	{
		int tid;
		void * retval;
	};

	/**
	 * @brief Thread table.
	 */
	EXTERN struct thread *threads[KTHREAD_MAX];

	/**
	 * @brief System threads table
	 */
	EXTERN struct thread threads_sys[SYS_THREAD_MAX];

	/**
	 * @name Thread IDs.
	 */
	/**@{*/
	#define KTHREAD_NULL_TID                   (-1) /**< ID of NULL thread.       */
	#define KTHREAD_MASTER_TID                  (0) /**< ID of master thread.     */
#if __NANVIX_USE_TASKS
	#define KTHREAD_DISPATCHER_TID              (1) /**< ID of dispatcher thread. */
#else
	#define KTHREAD_DISPATCHER_TID KTHREAD_NULL_TID /**< ID of dispatcher thread. */
#endif
	#define KTHREAD_LEADER_TID     (SYS_THREAD_MAX) /**< ID of leader thread.     */
	/**@}*/

	/**
	 * @brief Master thread.
	 */
	#define KTHREAD_MASTER (threads[0])

	/**
	 * @brief Dispatcher thread.
	 */
#if __NANVIX_USE_TASKS
	#define KTHREAD_DISPATCHER (threads[1])
#else
	#define KTHREAD_DISPATCHER (NULL)
#endif

	/**
	 * @brief Dispatcher core.
	 */
	#define KTHREAD_DISPATCHER_CORE (COREID_MASTER)

	/**
	 * @brief Gets the currently running thread.
	 *
	 * The thread_get() function returns a pointer to the thread
	 * that is running in the underlying core.
	 *
	 * @returns A pointer to the thread that is running in the
	 * underlying core.
	 */
	#if CLUSTER_IS_MULTICORE
	EXTERN struct thread *thread_get_curr(void);
	#else
	static inline struct thread *thread_get_curr(void)
	{
		return (KTHREAD_MASTER);
	}
	#endif

	/**
	 * @brief Gets the core ID of a thread.
	 *
	 * @param t Target thread.
	 *
	 * The thread_get_coreid() function returns the ID of the core
	 * that the thread pointed to by @p t is running.
	 *
	 * @returns The ID of the core that the target thread is running.
	 */
	static inline int thread_get_coreid(const struct thread *t)
	{
		return (t->coreid);
	}

	/**
	 * @brief Gets the ID of a thread.
	 *
	 * @param t Target thread.
	 *
	 * The thread_get_id() function returns the ID of the thread
	 * pointed to by @p t.
	 *
	 * @returns The ID of the target thread.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int thread_get_id(const struct thread *t)
	{
		return (t->tid);
	}

	/**
	 * @brief Gets the current running thread id
	 */
	static inline int thread_get_curr_id(void)
	{
		return (thread_get_id(thread_get_curr()));
	}

	/**
	 * @brief Gets the core set of affinity of a thread.
	 *
	 * @param t Target thread.
	 *
	 * @returns The core set of affinity.
	 */
	static inline int thread_get_affinity(const struct thread *t)
	{
		return (t->affinity);
	}

	/**
	 * @brief Sets a new affinity to a thread.
	 *
	 * @param new_affinity New affinity value.
	 *
	 * This function is thread-safe.
	 *
	 * @returns Old affinity value.
	 */
#if CORE_SUPPORTS_MULTITHREADING
	EXTERN int thread_set_affinity(struct thread * t, int new_affinity);
#else
	static int thread_set_affinity(struct thread * t, int new_affinity)
	{
		UNUSED(t);
		UNUSED(new_affinity);

		return (-ENOSYS);
	}
#endif

	/**
	 * @brief Sets a new affinity to a thread.
	 *
	 * @param new_affinity New affinity value.
	 *
	 * This function is thread-safe.
	 *
	 * @returns Old affinity value.
	 */
	static int thread_set_curr_affinity(int new_affinity)
	{
		return (thread_set_affinity(thread_get_curr(), new_affinity));
	}

	/**
	 * @brief Creates a thread.
	 *
	 * @param tid   Place to store the ID of the thread.
	 * @param start Thread start routine.
	 * @param arg   Argument for thread start routine.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int thread_create(int *tid, void*(*start)(void*), void *arg);

	/**
	 * @brief Terminates the calling thread.
	 *
	 * @param retval Return value.
	 */
	EXTERN NORETURN void thread_exit(void *retval);

	/**
	 * @brief Waits for a thread to terminate.
	 *
	 * @param tid    Target thread to wait for.
	 * @param retval Target location to store return value.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int thread_join(int tid, void **retval);

	/**
	 * @brief Atomically puts the calling thread to sleep.
	 *
	 * @param queue      Arrangement where the thread will be enqueued.
	 * @param queue_lock Spinlock to protect the queue.
	 * @param user_lock  Spinlock of the critical region of the user.
	 */
	EXTERN void thread_asleep(
		struct resource_arrangement * queue,
		spinlock_t * queue_lock,
		spinlock_t * user_lock
	);

	/**
	 * @brief Wakes up a thread.
	 *
	 * @param t Target thread.
	 */
	EXTERN void thread_wakeup(struct thread *t);

	/**
	 * @brief Release the core to another thread.
	 */
	EXTERN int thread_yield(void);

	/**
	 * @brief Manage the thread system.
	 */
	EXTERN void thread_manager(void);

	/**
	 * @brief Perform thread statistics.
	 */
	EXTERN int thread_stats(int tid, uint64_t *buffer, int stat);

	/**
	 * @brief Initialize thread system.
	 */
	EXTERN void thread_init(void);

/*============================================================================*
 *                        Condition Variables Facility                        *
 *============================================================================*/

	/**
	 * @brief Condition variable.
	 */
	struct condvar
	{
		spinlock_t lock;                   /**< Lock for sleeping queue. */
		struct resource_arrangement queue; /**< Sleeping queue.          */
	};

	/**
	 * @brief Static initializer for condition variables.
	 *
	 * The @p COND_INITIALIZER macro statically initiallizes a
	 * conditional variable.
	 */
	#define COND_STATIC_INITIALIZER                      \
	{                                                    \
		.lock  = SPINLOCK_UNLOCKED,                      \
		.queue = RESOURCE_ARRANGEMENT_STATIC_INITIALIZER \
	}

	/**
	 * @brief initializer for condition variables.
	 *
	 * The @p COND_INITIALIZER macro initiallizes a
	 * conditional variable.
	 */
	#define COND_INITIALIZER                      \
	(struct condvar){                             \
		.lock  = SPINLOCK_UNLOCKED,               \
		.queue = RESOURCE_ARRANGEMENT_INITIALIZER \
	}

	/**
	 * @brief Initializes a condition variable.
	 *
	 * @param cond Target condition variable.
	 */
	static inline void cond_init(struct condvar *cond)
	{
		spinlock_init(&cond->lock);
		cond->queue = RESOURCE_ARRANGEMENT_INITIALIZER;
	}

	/**
	 * @brief Waits on a condition variable.
	 *
	 * @param cond Target condition variable.
	 * @param lock Target spinlock to acquire.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int cond_wait(struct condvar *cond, spinlock_t *lock);

	/**
	 * @brief Unlocks a specific thread that is waiting on a condition
	 * variable.
	 *
	 * @param cond Target condition variable.
	 * @param tid  Target thread ID.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int cond_unicast(struct condvar *cond, int tid);

	/**
	 * @brief Unlocks first thread waiting on a condition variable.
	 *
	 * @param cond Target condition variable.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int cond_anycast(struct condvar *cond);

	/**
	 * @brief Unlocks all threads waiting on a condition variable.
	 *
	 * @param cond Target condition variable.
	 *
	 * @returns Upon successful completion zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int cond_broadcast(struct condvar *cond);

/*============================================================================*
 *                            Semaphores Facility                             *
 *============================================================================*/

	/**
	 * @brief Semahore
	 */
	struct semaphore
	{
		int count;           /**< Semaphore counter.  */
		spinlock_t lock;     /**< Semaphore lock.     */
		struct condvar cond; /**< Condition variable. */
	};

	/**
	 * @brief Static initializer for semaphores.
	 *
	 * The @p SEMAPHORE_INIT macro statically initializes the fields of
	 * a semaphore. The initial value of the semaphore is set to @p x
	 * in the initialization.
	 *
	 * @param x Initial value for semaphore.
	 */
	#define SEMAPHORE_STATIC_INITIALIZER(x) \
	{                                       \
		.count = (x),                       \
		.lock  = SPINLOCK_UNLOCKED,         \
		.cond  = COND_STATIC_INITIALIZER,   \
	}

	/**
	 * @brief Initializer for semaphores.
	 *
	 * The @p SEMAPHORE_INIT macro initializes the fields of
	 * a semaphore. The initial value of the semaphore is set to @p x
	 * in the initialization.
	 *
	 * @param x Initial value for semaphore.
	 */
	#define SEMAPHORE_INITIALIZER(x)        \
	(struct semaphore) {                    \
		.count = (x),                       \
		.lock  = SPINLOCK_UNLOCKED,         \
		.cond  = COND_INITIALIZER,          \
	}

	/**
	 * @brief Initializes a semaphore.
	 *
	 * The semaphore_init() function dynamically initializes the
	 * fields of the semaphore pointed to by @p sem. The initial value
	 * of the semaphore is set to @p x in the initialization.
	 *
	 * @param sem Target semaphore.
	 * @param x   Initial semaphore value.
	 */
	static inline void semaphore_init(struct semaphore *sem, int x)
	{
		KASSERT(x >= 0);
		KASSERT(sem != NULL);

		sem->count = x;
		spinlock_init(&sem->lock);
		cond_init(&sem->cond);
	}

	/**
	 * @brief Performs a down operation in a semaphore.
	 *
	 * @param sem Target semaphore.
	 */
	EXTERN void semaphore_down(struct semaphore *sem);

	/**
	 * @brief Tries a down operation in a semaphore.
	 *
	 * @param sem Target semaphore.
	 *
	 * @return Zero if successfully down the @p sem, non-zero otherwize.
	 */
	EXTERN int semaphore_trydown(struct semaphore *sem);

	/**
	 * @brief Performs an up operation in a semaphore.
	 *
	 * @param sem target semaphore.
	 */
	EXTERN void semaphore_up(struct semaphore *sem);

/*============================================================================*
 *                               Mutex Facility                               *
 *============================================================================*/

	/**
	 * @brief Mutex
	 */
	struct mutex
	{
		int curr_ticket;     /**< Current ticket.                 */
		int next_ticket;     /**< Next ticket available.          */
		int curr_owner;      /**< Thread ID that holds the mutex. */
		spinlock_t lock;     /**< Mutex lock.                     */
		struct condvar cond; /**< Condition variable.             */
	};

	/**
	 * @brief Static initializer for mutex.
	 *
	 * The @p MUTEX_STATIC_INITIALIZER macro statically initializes
	 * the fields of a mutex.
	 */
	#define MUTEX_STATIC_INITIALIZER            \
	{                                           \
		.curr_ticket = 0,                       \
		.next_ticket = 0,                       \
		.curr_owner  = KTHREAD_NULL_TID,        \
		.lock        = SPINLOCK_UNLOCKED,       \
		.cond        = COND_STATIC_INITIALIZER, \
	}

	/**
	 * @brief Initializer for mutex.
	 *
	 * The @p MUTEX_INITIALIZER macro initializes the fields of
	 * a mutex.
	 */
	#define MUTEX_INITIALIZER             \
	(struct mutex){                       \
		.curr_ticket = 0,                 \
		.next_ticket = 0,                 \
		.curr_owner  = KTHREAD_NULL_TID,  \
		.lock        = SPINLOCK_UNLOCKED, \
		.cond        = COND_INITIALIZER,  \
	}

	/**
	 * @brief Initializes a mutex.
	 *
	 * The mutex_init() function dynamically initializes the
	 * fields of the mutex pointed to by @p m.
	 *
	 * @param m Target mutex.
	 */
	static inline void mutex_init(struct mutex *m)
	{
		KASSERT(m != NULL);

		m->curr_ticket = 0;
		m->next_ticket = 0;
		m->curr_owner  = KTHREAD_NULL_TID;
		spinlock_init(&m->lock);
		cond_init(&m->cond);
	}

	/**
	 * @brief Performs a lock operation in a mutex.
	 *
	 * @param m Target mutex.
	 */
	EXTERN void mutex_lock(struct mutex *m);

	/**
	 * @brief Performs an unlock operation in a mutex.
	 *
	 * @param m target mutex.
	 */
	EXTERN void mutex_unlock(struct mutex *m);

/*============================================================================*
 *                               Tasks Facility                               *
 *============================================================================*/

#if __NANVIX_USE_TASKS

/*----------------------------------------------------------------------------*
 *                               Task Constants                               *
 *----------------------------------------------------------------------------*/

	/**
	 * @brief Enable communication with tasks.
	 */
	#define __NANVIX_USE_COMM_WITH_TASKS (__NANVIX_USE_TASKS && 1)

	/**
	 * @brief Invalid Task ID.
	 */
	#define TASK_NULL_ID (-1)

	/**
	 * @brief Number of arguments passed to the task function.
	 */
	#define TASK_ARGS_NUM (5)

	/**
	 * @brief Maximum number of children.
	 */
	#define TASK_CHILDREN_MAX (10)

	/**
	 * @brief Maximum number of parents.
	 */
	#define TASK_PARENTS_MAX (sizeof(word_t) * 8)

	/**
	 * @name States of a task.
	 */
	/**@{*/
	#define TASK_STATE_NOT_STARTED (1 << 0) /**< Not dispatched.   */
	#define TASK_STATE_READY       (1 << 1) /**< Ready to execute. */
	#define TASK_STATE_RUNNING     (1 << 2) /**< Running.          */
	#define TASK_STATE_COMPLETED   (1 << 3) /**< Completed.        */
	#define TASK_STATE_STOPPED     (1 << 4) /**< Stopped.          */
	#define TASK_STATE_PERIODIC    (1 << 5) /**< Periodic stopped. */
	#define TASK_STATE_ERROR       (1 << 6) /**< Error.            */
	#define TASK_STATE_INVALID     (1 << 7) /**< Invalid.          */
	/**@}*/

	/**
	 * @name Reschedule's type.
	 */
	/**@{*/
	#define TASK_SCHEDULE_READY    (TASK_STATE_READY)    /**< Immediately scheduling.     */
	#define TASK_SCHEDULE_STOPPED  (TASK_STATE_STOPPED)  /**< External signal scheduling. */
	#define TASK_SCHEDULE_PERIODIC (TASK_STATE_PERIODIC) /**< Programmable scheduling.    */
	/**@}*/

	/**
	 * @name Priority level.
	 */
	/**@{*/
	#define TASK_PRIORITY_LOW  (0) /**< Low priority.  */
	#define TASK_PRIORITY_HIGH (1) /**< High priority. */
	/**@}*/

	/**
	 * @name Connection's trigger condition.
	 *
	 * @details When a connection is notified.
	 * We use one byte (8 bits : unsigned char) to hold the triggers.
	 */
	/**@{*/
	#define TASK_TRIGGER_USER0       (1 << 0) /**< At success.                                  */
	#define TASK_TRIGGER_USER1       (1 << 1) /**< At success.                                  */
	#define TASK_TRIGGER_USER2       (1 << 2) /**< At success.                                  */
	#define TASK_TRIGGER_AGAIN       (1 << 3) /**< At reschedule.                               */
	#define TASK_TRIGGER_STOP        (1 << 4) /**< At task stoppage.                            */
	#define TASK_TRIGGER_PERIODIC    (1 << 5) /**< At periodic reschedule.                      */
	#define TASK_TRIGGER_ERROR_THROW (1 << 6) /**< At error propagation (cannot dispatch child) */
	#define TASK_TRIGGER_ERROR_CATCH (1 << 7) /**< At error notification (can dispatch child).  */

	#define TASK_TRIGGER_ALL  ((1 << 8) - 1)  /**< All triggers.                                */
	#define TASK_TRIGGER_NONE (0)             /**< None trigger.                                */
	/**@}*/

	/**
	 * @name Management behaviors on a task.
	 *
	 * @details The management value indicates which action the Dispatcher must
	 * perform over the current task.
	 */
	/**@{*/
	#define TASK_MANAGEMENT_USER0    TASK_TRIGGER_USER0       /**< Release the task on success.                   */
	#define TASK_MANAGEMENT_USER1    TASK_TRIGGER_USER1       /**< Release the task on finalization.              */
	#define TASK_MANAGEMENT_USER2    TASK_TRIGGER_USER2       /**< Complete the task without releasing semaphore. */
	#define TASK_MANAGEMENT_AGAIN    TASK_TRIGGER_AGAIN       /**< Reschedule the task.                           */
	#define TASK_MANAGEMENT_STOP     TASK_TRIGGER_STOP        /**< Move the task to stopped state.                */
	#define TASK_MANAGEMENT_PERIODIC TASK_TRIGGER_PERIODIC    /**< Periodic reschedule the task.                  */
	#define TASK_MANAGEMENT_ERROR \
		(TASK_TRIGGER_ERROR_THROW | TASK_TRIGGER_ERROR_CATCH) /**< Release the task with error.                   */
	#define TASK_MANAGEMENT_INVALID  TASK_TRIGGER_NONE        /**< Invalid management.                            */
	/**@}*/

	/**
	 * @name Connection's type.
	 *
	 * @details Types' description:
	 * 0: Hard connections are lifetime connections and must be explicitly
	 *    disconnected.
	 * 1: Soft connections are (on-demand) temporary connections that cease
	 *    to exist when the parent completes.
	 * 2: Invalid connection is used internally to specify an invalid node.
	 */
	/**@{*/
	#define TASK_CONN_IS_FLOW        (0) /**< Flow.                 */
	#define TASK_CONN_IS_DEPENDENCY  (1) /**< Dependency.           */
	#define TASK_CONN_IS_PERSISTENT  (0) /**< Lifetime connection.  */
	#define TASK_CONN_IS_TEMPORARY   (1) /**< Temporary connection. */
	/**@}*/

	/**
	 * @name Default merge arguments's functions.
	 */
	/**@{*/
	#define TASK_MERGE_ARGS_FN_REPLACE (task_args_replace) /**< Replace arguments. */
	/**@}*/

	/**
	 * @name Default values used on task_exit.
	 */
	/**@{*/
	#define TASK_MERGE_ARGS_FN_DEFAULT TASK_MERGE_ARGS_FN_REPLACE /**< Replace arguments.                                  */
	#define TASK_MANAGEMENT_DEFAULT    TASK_MANAGEMENT_USER0      /**< Release the task with success.                      */
	#define TASK_TRIGGER_DEFAULT \
		(TASK_TRIGGER_USER0 | TASK_TRIGGER_ERROR_THROW)           /**< All conditions except finalization and error catch. */
	#define TASK_PRIORITY_DEFAULT TASK_PRIORITY_LOW               /**< Low priority.                                       */
	/**@}*/

/*----------------------------------------------------------------------------*
 *                           Task Types and Structures                        *
 *----------------------------------------------------------------------------*/

	/**
	 * @brief Task function type.
	 *
	 * @param arg0 Argument value.
	 * @param arg1 Argument value.
	 * @param arg2 Argument value.
	 * @param arg4 Argument value.
	 * @param arg5 Argument value.
	 *
	 * @return The return value in the context of the task.
	 */
	typedef int (*task_fn)(
		word_t arg0,
		word_t arg1,
		word_t arg2,
		word_t arg3,
		word_t arg4
	);

	/**
	 * @brief Function type to merge the argument values passed from a parent
	 * task to a child task.
	 *
	 * @param exit_args  Arguments passed from the task_exit call.
	 * @param child_args Pointer to the arguments of a child task.
	 */
	typedef void (*task_merge_args_fn)(
		const word_t exit_args[TASK_ARGS_NUM],
		word_t child_args[TASK_ARGS_NUM]
	);

	/**
	 * @brief Task prototype.
	 */
	struct task;

	/**
	 * @brief Node of a task on children list.
	 *
	 * @details This node is used to put a task into a dependency list.
	 */
	struct task_node
	{
		byte_t is_valid      : 1; /**< Connection is valid.        */
		byte_t is_dependency : 1; /**< Connection is a dependency. */
		byte_t is_temporary  : 1; /**< Connection is temporary.    */
		byte_t unused        : 5; /**< Unused                      */
		byte_t triggers;          /**< Connection triggers.        */
		struct task * child;      /**< Task struct.                */
	};

	/**
	 * @name Invalid value o a task node.
	 */
	/**@{*/
	#define TASK_NODE_INVALID (struct task_node) { 0 }
	/**@}*/

	/**
	 * @brief Task.
	 */
	struct task
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource;                     /**< Resource struct.                */

		/**
		 * @name Period.
		 */
		/**@{*/
		int delta_factor;                             /**< Factor used on a delta queue.   */
		int period;                                   /**< Reload value of the period.     */
		/**@}*/

		/**
		 * @name Task parameters.
		 */
		/**@{*/
		int id;                                       /**< Identification.                 */
		int color;                                    /**< Color.                          */
		byte_t state;                                 /**< State.                          */
		byte_t schedule_type;                         /**< Schedule type.                  */
		byte_t priority;                              /**< Priority level.                 */
		/**@}*/

		/**
		 * @name Dependency graph.
		 */
		/**@{*/
		char nparents;                                /**< Current number of parents.      */
		char rparents;                                /**< Reload value of active parents. */
		word_t parent_types;                          /**< Reload value of active parents. */
		char nchildren;                               /**< Current number of children.     */
		struct task_node children[TASK_CHILDREN_MAX]; /**< Children list.                  */
		/**@}*/

		/**
		 * @name Task parameters.
		 */
		/**@{*/
		task_fn fn;                                   /**< Function pointer.               */
		word_t args[TASK_ARGS_NUM];                   /**< Arguments.                      */
		int retval;                                   /**< Return value.                   */
		/**@}*/

		/**
		 * @name Waiting control.
		 */
		/**@{*/
		byte_t releases;                              /**< When the semaphore is released. */
		struct semaphore sem;                         /**< Semaphore.                      */
		/**@}*/
	};

/*----------------------------------------------------------------------------*
 *                           Task Auxiliary Functions                         *
 *----------------------------------------------------------------------------*/

	/**
	 * @brief Gets the return value of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return Number ID.
	 */
	static inline int task_get_id(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->id);
	}

	/**
	 * @brief Gets the return value of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return Return value.
	 */
	static inline int task_get_return(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->retval);
	}

	/**
	 * @brief Gets the priority of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return Priority.
	 */
	static inline int task_get_priority(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->priority);
	}

	/**
	 * @brief Sets the priority of a task.
	 *
	 * @param task Task pointer.
	 */
	static inline void task_set_priority(struct task * task, int priority)
	{
		KASSERT(task != NULL);
		task->priority = (byte_t) priority;
	}

	/**
	 * @brief Gets the number of parents of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return The number of parents.
	 */
	static inline int task_get_number_parents(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->rparents);
	}

	/**
	 * @brief Gets the number of children of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return The number of children.
	 */
	static inline int task_get_number_children(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->nchildren);
	}

	/**
	 * @brief Gets children of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return Pointer to the first children.
	 */
	static inline struct task * task_get_child(const struct task * task, int offset)
	{
		KASSERT(task != NULL);

		if (UNLIKELY(!WITHIN(offset, 0, task->nchildren)))
			return (NULL);

		KASSERT(task->children[offset].child != NULL);

		return (task->children[offset].child);
	}

	/**
	 * @brief Gets the period of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return The period of the task.
	 */
	static inline int task_get_period(const struct task * task)
	{
		KASSERT(task != NULL);
		return (task->period);
	}

	/**
	 * @brief Sets the period of a task.
	 *
	 * @param task   Task pointer.
	 * @param period Period value.
	 */
	static inline void task_set_period(struct task * task, int period)
	{
		KASSERT(task != NULL);
		task->period = period > 0 ? period : 0;
	}

	/**
	 * @brief Sets arguments of the task
	 *
	 * @param task Task pointer.
	 * @param arg0 Argument value.
	 * @param arg1 Argument value.
	 * @param arg2 Argument value.
	 */
	static inline void task_set_arguments(
		struct task * task,
		word_t arg0,
		word_t arg1,
		word_t arg2,
		word_t arg3,
		word_t arg4
	)
	{
		KASSERT(task != NULL);
		task->args[0] = arg0;
		task->args[1] = arg1;
		task->args[2] = arg2;
		task->args[3] = arg3;
		task->args[4] = arg4;
	}

	/**
	 * @brief Gets children of a task.
	 *
	 * @param task Task pointer.
	 *
	 * @return Pointer to the arguments.
	 */
	static inline const word_t * task_get_arguments(struct task * task)
	{
		KASSERT(task != NULL);
		return ((const word_t *) (task->args));
	}

	/**
	 * @brief Replate all child arguments.
	 *
	 * @param exit_args  Arguments passed from the task_exit call.
	 * @param child_args Pointer to the arguments of a child task.
	 */
	static inline void task_args_replace(
		const word_t exit_args[TASK_ARGS_NUM],
		word_t child_args[TASK_ARGS_NUM]
	)
	{
		for (int i = 0; i < TASK_ARGS_NUM; ++i)
			child_args[i] = exit_args[i];
	}

	/**
	 * @brief Get current task.
	 *
	 * @returns Current thread running. NULL if the dispatcher is not executing
	 * a task.
	 */
	EXTERN struct task * task_current(void);

/*----------------------------------------------------------------------------*
 *                             Task Functionalities                           *
 *----------------------------------------------------------------------------*/

	/**
	 * @brief Create a task.
	 *
	 * Sets the struct parameters and initializes the mutex for the waiting
	 * control. The @p arg pointer can be a NULL pointer.
	 *
	 * @param task     Task pointer.
	 * @param fn       Function pointer.
	 * @param priority Priority.
	 * @param period   Period of the task (0 is meaning no periodic).
	 * @param releases When the semaphore will be released based on the
	 *                 management value.
	 *
	 * @return Zero if successfully create the task, non-zero otherwise.
	 */
	EXTERN int task_create(
		struct task * task,
		task_fn fn,
		int priority,
		int period,
		char releases
	);

	/**
	 * @brief Destroy a task.
	 *
	 * Sets the struct parameters and initializes the mutex for the waiting
	 * control. The @p arg pointer can be a NULL pointer.
	 *
	 * @param task Task pointer.
	 *
	 * @return Zero if successfully create the task, non-zero otherwise.
	 */
	EXTERN int task_unlink(struct task * task);

	/**
	 * @brief Create a connection on the @p child task to the @p parent task.
	 *
	 * @param parent        Independent task.
	 * @param child         Dependent task.
	 * @param is_dependency Connection's type.
	 * @param is_temporary  Connection's lifetime.
	 * @param triggers      Connection's triggers.
	 *
	 * @return Zero if successfully create the dependency, non-zero otherwise.
	 */
	EXTERN int task_connect(
		struct task * parent,
		struct task * child,
		bool is_dependency,
		bool is_temporary,
		char triggers
	);

	/**
	 * @brief Destroy a dependency on the @p child task to the @p parent task.
	 *
	 * @param parent Independent task.
	 * @param child  Dependent task.
	 *
	 * @return Zero if successfully create the dependency, non-zero otherwise.
	 */
	EXTERN int task_disconnect(struct task * parent, struct task * child);

	/**
	 * @brief Enqueue a task to the dispatcher thread operate.
	 *
	 * @param task Task pointer.
	 * @param arg0 Argument value.
	 * @param arg1 Argument value.
	 * @param arg2 Argument value.
	 * @param arg3 Argument value.
	 * @param arg4 Argument value.
	 *
	 * @return Zero if successfully dispatch a task, non-zero otherwise.
	 */
	EXTERN int task_dispatch(
		struct task * task,
		word_t arg0,
		word_t arg1,
		word_t arg2,
		word_t arg3,
		word_t arg4
	);

	/**
	 * @brief Emit a task to the target core operate.
	 *
	 * @param task   Task pointer.
	 * @param coreid Core ID.
	 * @param arg0   Argument value.
	 * @param arg1   Argument value.
	 * @param arg2   Argument value.
	 * @param arg3   Argument value.
	 * @param arg4   Argument value.
	 *
	 * @returns Zero if successfully emit the task, non-zero otherwise.
	 */
	EXTERN int task_emit(
		struct task * task,
		int coreid,
		word_t arg0,
		word_t arg1,
		word_t arg2,
		word_t arg3,
		word_t arg4
	);

	/**
	 * @brief Emit a task to the target core operate.
	 *
	 * @param retval     Return value.
	 * @param management Management action.
	 * @param merge      Function to pass the arguments to the child task.
	 * @param arg0       Argument value.
	 * @param arg1       Argument value.
	 * @param arg2       Argument value.
	 * @param arg3       Argument value.
	 * @param arg4       Argument value.
	 */
	EXTERN void task_exit(
		int retval,
		int management,
		task_merge_args_fn merge,
		word_t arg0,
		word_t arg1,
		word_t arg2,
		word_t arg3,
		word_t arg4
	);

	/**
	 * @brief Wait for a task to complete.
	 *
	 * @param task Task pointer.
	 *
	 * @return Zero if successfully wait for a task, non-zero otherwise.
	 */
	EXTERN int task_wait(struct task * task);

	/**
	 * @brief Tries to wait for a task to complete.
	 *
	 * @param task Task pointer.
	 *
	 * @return Zero if successfully wait for a task, non-zero otherwise.
	 */
	EXTERN int task_trywait(struct task * task);

	/**
	 * @brief Stop a task.
	 *
	 * @param task Task pointer.
	 *
	 * @returns Zero if successfully stop the task, non-zero otherwise.
	 */
	EXTERN int task_stop(struct task * task);

	/**
	 * @brief Continue a blocked task.
	 *
	 * @param task Task pointer.
	 *
	 * @returns Zero if successfully wakeup the task, non-zero otherwise.
	 */
	EXTERN int task_continue(struct task * task);

	/**
	 * @brief Complete a task.
	 *
	 * @param task       Task pointer.
	 * @param management Management triggers (only user-defined management).
	 *
	 * @returns Zero if successfully complete the task, non-zero otherwise.
	 */
	EXTERN int task_complete(struct task * task, char management);

/*----------------------------------------------------------------------------*
 *                         Task System Functionalities                        *
 *----------------------------------------------------------------------------*/

	/**
	 * @brief Notify a system tick to the time-based queue (periodic queue).
	 */
	EXTERN void task_tick(void);

	/**
	 * @brief Initializes task system.
	 */
	EXTERN void task_init(void);

#endif /* __NANVIX_USE_TASKS */

#endif /* NANVIX_THREAD_H_ */

/**@}*/
