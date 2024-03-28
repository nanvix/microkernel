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
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Get a semaphore.
 */
int kcall_semget(unsigned key)
{
    int semid = -1;
    int ret = semaphore_create(key);

    // Try create a semaphore.
    switch (ret) {
        case -EEXIST:
            // Return semaphore id if success in get semaphore or return error
            ret = semaphore_getid(key);
            return (ret);
        case -ENOBUFS:
            return (-ENOBUFS);
        default:
            semid = ret;
            break;
    }

    // Try get semaphore.
    ret = semaphore_get(semid);
    if (ret >= 0) {
        return (ret);
    }

    return (-ENOENT);
}
