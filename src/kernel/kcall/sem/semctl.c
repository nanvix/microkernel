/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/errno.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/pm/process.h>
#include <nanvix/kernel/pm/semaphore.h>

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @brief Command Semaphore Get Value
 */
#define SEMAPHORE_GETVALUE 0

/**
 * @brief Command Semaphore Set Value
 */
#define SEMAPHORE_SETVALUE 1

/**
 * @brief Command Semaphore Delete
 */
#define SEMAPHORE_DELETE 2

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Manages Semaphores.
 */
int kcall_semctl(int id, int cmd, int val)
{
    int ret = -EINVAL;

    switch (cmd) {
        case SEMAPHORE_GETVALUE:
            ret = semaphore_getcount(id);
            return (ret);
        case SEMAPHORE_SETVALUE:
            ret = semaphore_set(id, val);
            return (ret);
        case SEMAPHORE_DELETE:
            ret = semaphore_delete(id);
            return (ret);
        default:
            return (ret);
    }

    return (-EBADMSG);
}
