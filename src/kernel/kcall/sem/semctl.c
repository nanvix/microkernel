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
    switch (cmd) {
        case 0:
            return (semaphore_getcount(id));
        case 1:
            return (semaphore_set(id, val));
        case 2:
            return (semaphore_delete(id));
        default:
            return (-1);
    }

    return (-1);
}
