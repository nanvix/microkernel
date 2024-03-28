/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/errno.h>
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/log.h>
#include <nanvix/kernel/pm.h>
#include <stdbool.h>

/*============================================================================*
 * Private Variables                                                          *
 *============================================================================*/

static struct semaphore semtable[SEMAPHORE_MAX];

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/*
 * @brief Initialize semaphore sharing structure.
 *
 * @param semid Semaphore id.
 */
static void init_proc_users(int semid)
{
    struct process *currproc = process_get_curr();

    semtable[semid].proc_owner = currproc->pid;

    for (int i = 0; i < PROCESS_MAX; i++) {
        semtable[semid].proc_user[i] = -1;
    }
}

/**
 * @brief Verify if the semaphore is SEMAPHORE_ACTIVE.
 *
 * @param semid Semaphore id.
 *
 * @return True if semaphore is SEMAPHORE_ACTIVE, false otherwise.
 */
static bool is_semaphore_SEMAPHORE_ACTIVE(int semid)
{
    return semtable[semid].state == SEMAPHORE_ACTIVE ? (true) : (false);
}

/**
 * @brief Check if semaphore Key is being used.
 *
 * @param key Semaphore key.
 *
 * @return semaphore id if key is being used, else -1.
 */
static int key_check(unsigned key)
{
    for (int semid = 0; semid < SEMAPHORE_MAX; semid++) {
        if (is_semaphore_SEMAPHORE_ACTIVE(semid) &&
            semtable[semid].key == key) {
            return (semid);
        }
    }

    return (-1);
}

/*
 * @brief Verify if the current process already got semaphore.
 *
 * @param semid Semaphore id.
 *
 * @return True if semaphore is get, false otherwise.
 */
static bool is_semaphore_get(int semid)
{
    struct process *currproc = process_get_curr();

    if (semtable[semid].proc_owner == currproc->pid) {
        return (true);
    }

    for (int i = 0; i < PROCESS_MAX; i++) {
        if (semtable[semid].proc_user[i] == currproc->pid) {
            return (true);
        }
    }

    return (false);
}

/*
 * @brief Drop semaphore got.
 *
 * @param semid Semaphore id.
 *
 * @return (0) if successful , (-1) otherwise.
 */
static int semaphore_drop(int semid)
{
    struct process *currproc = process_get_curr();

    for (int i = 0; i < PROCESS_MAX; i++) {
        if (semtable[semid].proc_user[i] == currproc->pid) {
            semtable[semid].proc_user[i] = -1;
            return (0);
        }
    }

    return (-1);
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

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
int semaphore_set(int semid, int count)
{
    // Verify if semaphore is SEMAPHORE_ACTIVE.
    if (!is_semaphore_SEMAPHORE_ACTIVE(semid)) {
        return (-ENOENT);
    }

    // Verify if semaphore is get.
    if (!is_semaphore_get(semid)) {
        return (-EACCES);
    }

    semaphore_init(&semtable[semid], count);

    return 0;
}

/**
 * @brief Set process as semaphore user.
 *
 * @param semid Semaphore id.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
int semaphore_get(int semid)
{
    // Verify if semaphore is SEMAPHORE_ACTIVE.
    if (!is_semaphore_SEMAPHORE_ACTIVE(semid)) {
        return (-ENOENT);
    }

    // Verify if semaphore already was get.
    if (is_semaphore_get(semid)) {
        return (semid);
    }

    struct process *currproc = process_get_curr();

    for (int i = 0; i < PROCESS_MAX; i++) {
        if (semtable[semid].proc_user[i] == -1) {
            semtable[semid].proc_user[i] = currproc->pid;
            return (semid);
        }
    }

    return (-ENOBUFS);
}

/**
 * @brief Initialize semaphore structure.
 *
 * @param key Semaphore key.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
int semaphore_create(unsigned key)
{
    // Check if key already exist
    int key_semid = key_check(key);
    if (key_semid != -1) {
        return (-EEXIST);
    }

    // Found SEMAPHORE_INACTIVE semaphores
    for (int semid = 0; semid < SEMAPHORE_MAX; semid++) {
        if (semtable[semid].state == SEMAPHORE_INACTIVE) {

            // Init semaphore sharing structure.
            init_proc_users(semid);
            semtable[semid].state = SEMAPHORE_ACTIVE;
            semtable[semid].key = key;

            return (semid);
        }
    }

    return (-ENOBUFS);
}

/**
 * @brief Delete or drop semaphore.
 *
 * @param semid Semaphore id.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
int semaphore_delete(int semid)
{
    // Verify if semaphore is SEMAPHORE_ACTIVE.
    if (!is_semaphore_SEMAPHORE_ACTIVE(semid)) {
        return (-ENOENT);
    }

    // Verify if semaphore is get.
    if (!is_semaphore_get(semid)) {
        return (-EACCES);
    }

    struct process *currproc = process_get_curr();

    // If currproc is owner, delete semaphore, else, drop semaphore.
    if (semtable[semid].proc_owner == currproc->pid) {
        semtable[semid].state = SEMAPHORE_INACTIVE;
        return (0);
    }

    if (semaphore_drop(semid)) {
        return (0);
    }

    return (-EBADMSG);
}

/*
 * @brief Return value of semaphore count.
 *
 * @p semid Semaphore id.
 *
 * @return Upon successful completion, Semaphore Count is returned. Upon
 * failure, a negative error code is returned instead.
 */
int semaphore_getcount(int semid)
{
    // Verify if semaphore is SEMAPHORE_ACTIVE.
    if (!is_semaphore_SEMAPHORE_ACTIVE(semid)) {
        return (-ENOENT);
    }

    // Verify if semaphore is get.
    if (!is_semaphore_get(semid)) {
        return (-EACCES);
    }

    return (semtable[semid].count);
}

/*
 * @brief Return semaphore id.
 *
 * @p key Key associated with the semaphore.
 *
 * @return Upon successful completion, (semid) is returned. Upon failure, a
 * negative error code is returned instead.
 */
int semaphore_getid(int key)
{
    int semid = key_check(key);

    if (semid != -1) {
        return (semid);
    }
    return (-ENOENT);
}

/*
 * @brief Initialize semaphores table.
 */
void semtable_init(void)
{
    static bool initialized = false;

    // Nothing to do.
    if (initialized) {
        log(WARN, "trying to initialize semaphore again?");
        return;
    }

    for (int semid = 0; semid < SEMAPHORE_MAX; semid++) {
        semtable[semid].state = SEMAPHORE_INACTIVE;
        init_proc_users(semid);
    }
}

/**
 * @details This function performs a down operation in the semaphore pointed to
 * by @p sem. It atomically checks the current value of @p sem. If it is greater
 * than one, it decrements the semaphore counter by one and the calling process
 * continue its execution, flow as usual.  Otherwise, the calling process sleeps
 * until another process performs a call to semaphore_up() on this semaphore.
 *
 * @see SEMAPHORE_INIT(), semaphore_up()
 */
void semaphore_down(struct semaphore *sem)
{
    KASSERT(sem != NULL);

    while (true) {
        if (sem->count > 0) {
            break;
        }

        cond_wait(&sem->cond);
    }

    sem->count--;
}

/**
 * @details This function performs an up operation in a semaphore pointed to by
 * @p sem. It atomically increments the current value of @p and wakes up all
 * processes that were sleeping in this semaphore, waiting for a semaphore_up()
 * operation.
 *
 * @see SEMAPHORE_INIT(), semaphore_down()
 */
void semaphore_up(struct semaphore *sem)
{
    KASSERT(sem != NULL);

    sem->count++;
    cond_broadcast(&sem->cond);
}
