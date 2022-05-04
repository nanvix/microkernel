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
	#ifndef __NANVIX_MICROKERNEL_STATIC_SCHED
	#define __NANVIX_MICROKERNEL_STATIC_SCHED 0
	#endif
	#ifndef __NANVIX_MICROKERNEL_THREAD_STATS
	#define __NANVIX_MICROKERNEL_THREAD_STATS 0
	#endif
	/**@}*/

	/**
	 * @brief Kernel thread dedicated to kernel services.
	 */
	#define KTHREAD_SERVICE_MAX (1) /**< Master thread.              */

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
	 * @brief Thread table.
	 */
	EXTERN struct thread threads[KTHREAD_MAX];

	/**
	 * @name Thread IDs.
	 */
	/**@{*/
	#define KTHREAD_NULL_TID                   (-1) /**< ID of NULL thread.       */
	#define KTHREAD_MASTER_TID                  (0) /**< ID of master thread.     */
	#define KTHREAD_LEADER_TID     (SYS_THREAD_MAX) /**< ID of leader thread.     */
	/**@}*/

	/**
	 * @brief Master thread.
	 */
	#define KTHREAD_MASTER (&threads[0])

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
 *                            Thread Keys Facility                            *
 *============================================================================*/

	/**
	 * @name Control variables.
	 */
	/**@{*/
	#define THREAD_KEY_MAX 4                                                 /**< Maximum number of keys.           */
	#define THREAD_KEY_VALUE_PER_KEY (THREAD_MAX)                            /**< Maximum number of values per key. */
	#define THREAD_KEY_VALUE_MAX (THREAD_KEY_MAX * THREAD_KEY_VALUE_PER_KEY) /**< Maximum number of key values.     */
	/**@}*/

	EXTERN int thread_key_create(int * key, void (* destructor)(void *));

	EXTERN int thread_key_delete(int key);

	EXTERN int thread_getspecific(int tid, int key, void ** value);

	EXTERN int thread_setspecific(int tid, int key, void * value);

	EXTERN void thread_key_exit(int tid, int * retv);

#endif /* NANVIX_THREAD_H_ */

/**@}*/
