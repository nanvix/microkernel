/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_IPC_MAILBOX_H_
#define NANVIX_KERNEL_IPC_MAILBOX_H_

#include <nanvix/kernel/pm.h>

/**
 * @addtogroup kernel-ipc-mailbox Mailbox
 * @ingroup kernel-ipc
 *
 * @brief Mailbox IPC
 */
/**@{*/

/*============================================================================*
 * Kernel-Public Functions                                                    *
 *============================================================================*/

/**
 * @brief Initialize the mailbox module.
 */
extern void mailbox_init(void);

/**
 * @brief Gets the owner of a mailbox.
 */
extern pid_t mailbox_owner(const int);

/**
 * @brief Assigns a mailbox.
 */
extern int mailbox_assign(const int, const pid_t, const int);

/**
 * @brief Links a mailbox.
 */
extern int mailbox_link(const int);

/**
 * @brief Unlinks a mailbox.
 */
extern int mailbox_unlink(const int);

/**
 * @brief Pushes a message into a mailbox.
 */
extern int mailbox_push(const int, const void *, const size_t);

/**
 * Pop a message from a mailbox.
 */
extern int mailbox_pop(const int, void *, const size_t);
/*============================================================================*/

/**@}*/

#endif /* !NANVIX_KERNEL_IPC_MAILBOX_H_ */
