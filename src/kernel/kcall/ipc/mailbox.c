/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/cc.h>
#include <nanvix/errno.h>
#include <nanvix/kernel/ipc/mailbox.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/pm.h>

// #include <nanvix/kernel/ipc/mailbox.h>
// #include <nanvix/cc.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * Gets mailbox tag.
 */
int kcall_mailbox_tag(int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Checks whether a mailbox is assigned.
 */
int kcall_mailbox_is_assigned(const int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Gets the owner of a mailbox.
 */
pid_t kcall_mailbox_owner(const int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Initializes a mailbox with the default values.
 */
int kcall_mailbox_default(const int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Assigns a mailbox.
 */
int kcall_mailbox_assign(const int mbxid, const pid_t owner, const int tag)
{
    UNUSED(mbxid);
    UNUSED(owner);
    UNUSED(tag);

    return (ENOTSUP);
}

/**
 * Links a mailbox.
 */
int kcall_mailbox_link(const int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Unlinks a mailbox.
 */
int kcall_mailbox_unlink(const int mbxid)
{
    UNUSED(mbxid);

    return (ENOTSUP);
}

/**
 * Pushes a message into a mailbox.
 */
int kcall_mailbox_push(const int mbxid, const void *msg, const size_t sz)
{
    UNUSED(mbxid);
    UNUSED(msg);
    UNUSED(sz);

    return (ENOTSUP);
}

/**
 * Pop a message from a mailbox.
 */
int kcall_mailbox_pop(const int mbxid, void *msg, const size_t sz)
{
    UNUSED(mbxid);
    UNUSED(msg);
    UNUSED(sz);

    return (0);
}