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
    return (mailbox_tag(mbxid));
}

/**
 * Checks whether a mailbox is assigned.
 */
int kcall_mailbox_is_assigned(const int mbxid)
{
    return (mailbox_is_assigned(mbxid));
}

/**
 * Gets the owner of a mailbox.
 */
pid_t kcall_mailbox_owner(const int mbxid)
{
    return (mailbox_owner(mbxid));
}

/**
 * Initializes a mailbox with the default values.
 */
int kcall_mailbox_default(const int mbxid)
{
    return (mailbox_default(mbxid));
}

/**
 * Assigns a mailbox.
 */
int kcall_mailbox_assign(const int mbxid, const pid_t owner, const int tag)
{
    return (mailbox_assign(mbxid, owner, tag));
}

/**
 * Links a mailbox.
 */
int kcall_mailbox_link(const int mbxid)
{
    return (mailbox_link(mbxid));
}

/**
 * Unlinks a mailbox.
 */
int kcall_mailbox_unlink(const int mbxid)
{
    return (mailbox_unlink(mbxid));
}

/**
 * Pushes a message into a mailbox.
 */
int kcall_mailbox_push(const int mbxid, const void *msg, const size_t sz)
{
    return (mailbox_push(mbxid, msg, sz));
}

/**
 * Pop a message from a mailbox.
 */
int kcall_mailbox_pop(const int mbxid, void *msg, const size_t sz)
{
    return (mailbox_pop(mbxid, msg, sz));
}