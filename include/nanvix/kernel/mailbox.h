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
 * @defgroup kernel-mailbox Mailbox Facility
 * @ingroup kernel
 *
 * @brief Mailbox Facility
 */

#ifndef NANVIX_MAILBOX_H_
#define NANVIX_MAILBOX_H_

	#include <nanvix/hal.h>
	#include <nanvix/const.h>
	#include <posix/stdarg.h>

	/**
	 * @name Mailbox I/O Control types.
	 */
	/**@{*/
	#define KMAILBOX_IOCTL_GET_VOLUME   (1) /**< Gets communication volume.            */
	#define KMAILBOX_IOCTL_GET_LATENCY  (2) /**< Gets communication latency.           */
	#define KMAILBOX_IOCTL_GET_NCREATES (3) /**< Gets number of creates.               */
	#define KMAILBOX_IOCTL_GET_NUNLINKS (4) /**< Gets number of unlinks.               */
	#define KMAILBOX_IOCTL_GET_NOPENS   (5) /**< Gets number of opens.                 */
	#define KMAILBOX_IOCTL_GET_NCLOSES  (6) /**< Gets number of closes.                */
	#define KMAILBOX_IOCTL_GET_NREADS   (7) /**< Gets number of reads.                 */
	#define KMAILBOX_IOCTL_GET_NWRITES  (8) /**< Gets number of writes.                */
	#define KMAILBOX_IOCTL_SET_REMOTE   (9) /**< Sets the remote_addr until next read. */
	/**@}*/

	/**
	 * @brief Number of ports per HW mailbox.
	 *
	 * Maximum number of virtual mailboxes that can be vinculated to each HW mailbox.
	 */
#if __NANVIX_IKC_USES_ONLY_MAILBOX
	#define MAILBOX_PORT_NR (64)
#else
	#define MAILBOX_PORT_NR (24)
#endif

	/**
	 * @brief Number of ports per Kernel mailbox.
	 */
#if __NANVIX_IKC_USES_ONLY_MAILBOX
	#define KMAILBOX_PORT_NR (32)
#else
	#define KMAILBOX_PORT_NR (24)
#endif

	/**
	 * @brief Auxiliary definitions for src header check.
	 */
	#define MAILBOX_ANY_SOURCE PROCESSOR_NOC_NODES_NUM
	#define MAILBOX_ANY_PORT   MAILBOX_PORT_NR

	/**
	 * @brief Maximum number of HW mailboxes.
	 *
	 * Maximum number of active mailboxes that may be created/opened.
	 *
	 * @note This constant is based on the Hal exported mailboxes plus 1 relative
	 * to when the user open a mailbox to the local node, what is supported by the
	 * kernel but not by the Hal.
	 */
	#define HW_MAILBOX_MAX (HAL_MAILBOX_CREATE_MAX + HAL_MAILBOX_OPEN_MAX + 1)

	/**
	 * @brief Maximum number of virtual mailboxes.
	 *
	 * Maximum number of virtual mailboxes that may be created/opened.
	 */
#if __NANVIX_IKC_USES_ONLY_MAILBOX
	#define KMAILBOX_MAX (256)
#else
	#define KMAILBOX_MAX (64)
#endif

	/**
	 * @brief Mailbox message header size.
	 *
	 */
	#define KMAILBOX_MESSAGE_HEADER_SIZE (HAL_MAILBOX_RESERVED_SIZE)

	/**
	 * @brief Mailbox message size.
	 */
	#define KMAILBOX_MESSAGE_DATA_SIZE (HAL_MAILBOX_DATA_SIZE)

	/**
	 * @brief Maximum number of message buffer resources.
	 *
	 * Maximum number of message buffers used to hold temporary data on kernel space.
	 */
#if __NANVIX_IKC_USES_ONLY_MAILBOX
	#define KMAILBOX_MESSAGE_BUFFERS_MAX (64)
#else
	#define KMAILBOX_MESSAGE_BUFFERS_MAX (32)
#endif

	/**
	 * @brief Maximum number of auxiliary message buffer resources.
	 *
	 * Maximum number of message buffers used to hold temporary data on kernel space.
	 * WARNING: That constant uses a subset of mbuffers set by @c KMAILBOX_MESSAGE_BUFFERS_MAX
	 *
	 * @todo TODO: introduce a check for this.
	 */
	#define KMAILBOX_AUX_BUFFERS_MAX (16)

	/**
	 * @brief Mailbox message buffer max size.
	 *
	 * Maximum size of mailbox message data buffer.
	 */
	#define KMAILBOX_MESSAGE_SIZE (HAL_MAILBOX_MSG_SIZE - KMAILBOX_MESSAGE_HEADER_SIZE)

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Creates a virtual mailbox.
	 *
	 * @param local Logic ID of the local node.
	 * @param port  Target port in @p local node.
	 *
	 * @returns Upon successful completion, the ID of the newly created
	 * mailbox is returned. Upon failure, a negative error code is
	 * returned instead.
	 */
	EXTERN int do_vmailbox_create(int local, int port);

	/**
	 * @brief Opens a virtual mailbox.
	 *
	 * @param remote      Logic ID of the target node.
	 * @param remote_port Target port in @p remote node.
	 *
	 * @returns Upon successful completion, the ID of the target mailbox
	 * is returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	EXTERN int do_vmailbox_open(int remote, int remote_port);

	/**
	 * @brief Destroys a virtual mailbox.
	 *
	 * @param mbxid ID of the target virtual mailbox.
	 *
	 * @returns Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_unlink(int mbxid);

	/**
	 * @brief Closes a virtual mailbox.
	 *
	 * @param mbxid ID of the target virtual mailbox.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_close(int mbxid);

	/**
	 * @brief Reads data from a virtual mailbox.
	 *
	 * @param mbxid  ID of the target virtual mailbox.
	 * @param buffer Buffer where the data should be written to.
	 * @param size   Number of bytes to read.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_aread(int mbxid, void * buffer, size_t size);

	/**
	 * @brief Writes data to a virtual mailbox.
	 *
	 * @param mbxid ID of the target virtual mailbox.
	 * @param buffer   Buffer where the data should be read from.
	 * @param size     Number of bytes to write.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_awrite(int mbxid, const void * buffer, size_t size);

	/**
	 * @brief Waits for an asynchronous operation on a virtual mailbox.
	 *
	 * @param mbxid ID of the target virtual mailbox.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_wait(int mbxid);

	/**
	 * @brief Performs control operations in a virtual mailbox.
	 *
	 * @param mbxid   Target mailbox.
	 * @param request Request.
	 * @param args    Additional arguments.
	 *
	 * @returns Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_ioctl(int mbxid, unsigned request, va_list args);

	/**
	 * @brief Gets the @p mbxid logic port.
	 *
	 * @param mbxid Target mailbox.
	 *
	 * @returns Upon successful completion, the logic port is returned.
	 * Upon failure, a negative error code is returned instead.
	 */
	EXTERN int do_vmailbox_get_port(int mbxid);

	/**
	 * @brief Initializes the mailbox facility.
	 */
	EXTERN void vmailbox_init(void);

#endif /* __NANVIX_MICROKERNEL */

#endif /* NANVIX_MAILBOX_H_ */

/**@}*/
