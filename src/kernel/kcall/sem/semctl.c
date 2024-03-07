/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/pm/process.h>
#include <nanvix/kernel/pm/semaphore.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Manages Semaphores.
 */
int kcall_semctl(int id, int cmd, int val)
{
    int ret = -1;

    switch (cmd) {
        case 0:
            ret = semaphore_getcount(id);
            if (ret < 0) {
                // Semaphore inactive or didn't get.
                ret == -1 ? (ret = -ENOENT) : (ret = -EACCES);
            }
            return (ret);
        case 1:
            ret = semaphore_set(id, val);
            if (ret < 0) {
                // Semaphore inactive or didn't get.
                ret == -1 ? (ret = -ENOENT) : (ret = -EACCES);
            }
            return (ret);
        case 2:
            ret = semaphore_delete(id);
            if (ret < 0) {
                // Semaphore inactive or didn't get.
                ret == -1 ? (ret = -ENOENT) : (ret = -EACCES);
            }
            return (ret);
        default:
            return (-EBADMSG);
    }

    return (-EBADMSG);
}
