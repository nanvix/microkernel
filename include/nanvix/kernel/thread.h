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
	#include <nanvix/kernel/config.h>
	#include <nanvix/const.h>

/*============================================================================*
 *                                Thread System                               *
 *============================================================================*/

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
		#define KTHREAD_IDLE_MAX (CORES_NUM - 1) /**< CORES_NUM - Master thread. */
	#else
		#define KTHREAD_IDLE_MAX (0)             /**< There is not idle thread.  */
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
			#define THREAD_MAX (24 - KTHREAD_SERVICE_MAX) /**< Reserved 24 kernel pages.*/
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
	 * @brief Thread.
	 */
	struct thread
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource; /**< Generic resource information. */

		int tid;                  /**< Thread ID.                    */
		short coreid;             /**< Core ID.                      */
		short state;              /**< State.                        */
		int affinity;             /**< Affinity.                     */
		uint64_t age;             /**< Age.                          */
		void *arg;                /**< Argument.                     */
		void *(*start)(void*);    /**< Starting routine.             */
		struct context *ctx;      /**< Preempted context.            */
	};

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
	#define KTHREAD_MASTER (&threads[0])

	/**
	 * @brief Dispatcher thread.
	 */
#if __NANVIX_USE_TASKS
	#define KTHREAD_DISPATCHER (&threads[1])
#else
	#define KTHREAD_DISPATCHER (NULL)
#endif

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

	/**
	 * @name States of a task.
	 */
	/**@{*/
	#define TASK_STATE_ERROR       (-1) /**< Task exit with error.  */
	#define TASK_STATE_NOT_STARTED  (0) /**< Task not dispatched.   */
	#define TASK_STATE_READY        (1) /**< Task ready to execute. */
	#define TASK_STATE_RUNNING      (2) /**< Task running.          */
	#define TASK_STATE_STOPPED      (3) /**< Task Stopped.          */
	#define TASK_STATE_COMPLETED    (4) /**< Task completed.        */
	/**@}*/

	/**
	 * @name Return behaviors of a task.
	 *
	 * @details The return value of the operation function indicates which
	 * manager actions to perform over the current task.
	 */
	/**@{*/
	#define TASK_RET_ERROR   (-1) /**< Release the task with error.    */
	#define TASK_RET_SUCCESS  (0) /**< Release the task with success.  */
	#define TASK_RET_AGAIN    (1) /**< Reschedule the task.            */
	#define TASK_RET_STOP     (3) /**< Move the task to stopped state. */
	/**@}*/

	/**
	 * @brief Invalid Task ID
	 */
	#define TASK_NULL_ID (~0ULL)

	/**
	 * @brief Task arguments and return.
	 */
	struct task_args
	{
		word_t arg0; /**< Argument slot.    */
		word_t arg1; /**< Argument slot.    */
		word_t arg2; /**< Argument slot.    */
		word_t arg3; /**< Argument slot.    */
		word_t arg4; /**< Argument slot.    */
		word_t arg5; /**< Argument slot.    */
		int ret;     /**< Return value.     */
	};

	/**
	 * @brief Task function type.
	 *
	 * @param args Argument pointer.
	 *
	 * @return Zero to task completed. Non-zero, an error ocurred.
	 * If the task can be rescheduled, return value must be the -EAGAIN.
	 */
	typedef int (*task_fn)(struct task_args *);

	/**
	 * @brief Task.
	 */
	struct task
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource;             /**< Resource struct.            */

		/**
		 * @name Period.
		 */
		/**@{*/
		int period;                           /**< Current period value.       */
		int reload;                           /**< Reload value of the period. */
		/**@}*/

		/**
		 * @name Task parameters.
		 */
		/**@{*/
		uint64_t id;                          /**< Identification.             */
		char state;                           /**< State.                      */
		/**@}*/

		/**
		 * @name Dependency graph.
		 */
		/**@{*/
		char parents;                         /**< Number of tracked parents.  */
		struct resource_arrangement children; /**< List of successors.         */
		/**@}*/

		/**
		 * @name Task parameters.
		 */
		/**@{*/
		task_fn fn;                           /**< Function pointer.           */
		struct task_args args;                /**< Arguments.                  */
		/**@}*/

		/**
		 * @brief Waiting control.
		 */
		struct semaphore sem;                 /**< Semaphore.                  */
	};

	/**
	 * @brief Create a task.
	 *
	 * Sets the struct parameters and initializes the mutex for the waiting
	 * control. The @p arg pointer can be a NULL pointer.
	 *
	 * @param task   Task pointer.
	 * @param fn     Function pointer.
	 * @param arg    Arguments pointer.
	 * @param period Period of the task (0 is meaning no periodic).
	 *
	 * @return Zero if successfully create the task, non-zero otherwise.
	 */
	EXTERN int task_create(
		struct task * task,
		task_fn fn,
		struct task_args * args,
		int period
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
	 * @brief Create a dependency on the @p child task to the @p parent task.
	 *
	 * @param parent Independent task.
	 * @param child  Dependent task.
	 *
	 * @return Zero if successfully create the dependency, non-zero otherwise.
	 */
	EXTERN int task_connect(struct task * parent, struct task * child);

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
	 * @brief Gets the return value of a task.
	 *
	 * @param task Task pointer.
	 */
	static inline uint64_t task_get_id(struct task * task)
	{
		return (task->id);
	}

	/**
	 * @brief Gets the return value of a task.
	 *
	 * @param task Task pointer.
	 */
	static inline int task_get_return(struct task * task)
	{
		return (task->args.ret);
	}

	/**
	 * @brief Enqueue a task to the dispatcher thread operate.
	 *
	 * @param task Task pointer.
	 *
	 * @return Zero if successfully dispatch a task, non-zero otherwise.
	 */
	EXTERN int task_dispatch(struct task * task);

	/**
	 * @brief Wait for a task to complete.
	 *
	 * @param task Task pointer.
	 *
	 * @return Zero if successfully wait for a task, non-zero otherwise.
	 */
	EXTERN int task_wait(struct task * task);

	/**
	 * @brief Continue a blocked task.
	 *
	 * @param target Task pointer.
	 *
	 * @returns Zero if successfully wakeup the task, non-zero otherwise.
	 */
	EXTERN int task_continue(struct task * task);

	/**
	 * @brief Complete a task.
	 *
	 * @param target Task pointer.
	 *
	 * @returns Zero if successfully complete the task, non-zero otherwise.
	 */
	EXTERN int task_complete(struct task * task);

	/**
	 * @brief Get current task.
	 *
	 * @returns Current thread running. NULL if the dispatcher is not executing
	 * a task.
	 */
	EXTERN struct task * task_current(void);

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
