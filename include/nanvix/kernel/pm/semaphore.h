/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_PM_SEMAPHORE_H_
#define NANVIX_KERNEL_PM_SEMAPHORE_H_

#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/pm/cond.h>

/**
 * @brief Maximum Number of semaphores in the system.
 */
#define SEMAPHORE_MAX 128

/**
 * @brief Active semaphore?
 */
#define SEMAPHORE_ACTIVE 1

/**
 * @brief Inative semaphore?
 */
#define SEMAPHORE_INACTIVE 0

/**
 * @brief Semaphore
 */
struct semaphore {
    int count;                    /** Semaphore counter.  */
    struct condvar cond;          /** Condition variable. */
    int state;                    /** Semaphore state     */
    pid_t proc_owner;             /** Owner process.      */
    pid_t proc_user[PROCESS_MAX]; /** Users process.      */
    unsigned key;                 /** Semphore key.       */
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
#define SEMAPHORE_INITIALIZER(x)                                               \
    {                                                                          \
        .count = (x), .cond = COND_INITIALIZER,                                \
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
    cond_init(&sem->cond);
}

/**
 * @brief Set process as semaphore user.
 *
 * @param semid Semaphore id.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int semaphore_get(int semid);

/**
 * @brief Initialize semaphore structure.
 *
 * @param key Semaphore key.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int semaphore_create(unsigned key);

/**
 * @brief Delete or drop semaphore.
 *
 * @param semid Semaphore id.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int semaphore_delete(int semid);

/*
 * @brief Initialize semaphores table.
 */
extern void semtable_init(void);

/*
 * @brief Initialize control variables.
 *
 * @param semid Semaphore id.
 *
 * @param count Semaphore counter.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int semaphore_set(int semid, int count);

/*
 * @brief Return semaphore id.
 *
 * @p key Key associated with the semaphore.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int semaphore_getid(int key);

/*
 * @brief Return value of semaphore count.
 *
 * @p semid Semaphore id.
 *
 * @return Upon successful completion, Semaphore Count is returned. Upon
 * failure, a negative error code is returned instead.
 */
extern int semaphore_getcount(int semid);

/**
 * @brief Performs a down operation in a semaphore.
 *
 * @param sem Target semaphore.
 */
extern void semaphore_down(struct semaphore *sem);

/**
 * @brief Performs an up operation in a semaphore.
 *
 * @param sem target semaphore.
 */
extern void semaphore_up(struct semaphore *sem);

#endif /* NANVIX_KERNEL_PM_SEMAPHORE_H_ */
