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
 * @defgroup kernel-portal Portal Facility
 * @ingroup kernel
 *
 * @brief Portal Facility
 */

#ifndef NANVIX_PORTAL_H_
#define NANVIX_PORTAL_H_

	#include <nanvix/hal.h>

#ifdef __NANVIX_MICROKERNEL

	#include <nanvix/const.h>
	#include <posix/stdarg.h>

#endif /* __NANVIX_MICROKERNEL */

	/**
	 * @name Portal I/O Control types.
	 */
	/**@{*/
	#define KPORTAL_IOCTL_GET_VOLUME   (1) /**< Gets communication volume.  */
	#define KPORTAL_IOCTL_GET_LATENCY  (2) /**< Gets communication latency. */
	#define KPORTAL_IOCTL_GET_NCREATES (3) /**< Gets number of creates.     */
	#define KPORTAL_IOCTL_GET_NUNLINKS (4) /**< Gets number of unlinks.     */
	#define KPORTAL_IOCTL_GET_NOPENS   (5) /**< Gets number of opens.       */
	#define KPORTAL_IOCTL_GET_NCLOSES  (6) /**< Gets number of closes.      */
	#define KPORTAL_IOCTL_GET_NREADS   (7) /**< Gets number of reads.       */
	#define KPORTAL_IOCTL_GET_NWRITES  (8) /**< Gets number of writes.      */
	/**@}*/

	/**
	 * @brief Number of ports per portal.
	 */
	#define KPORTAL_PORT_NR 24

	/**
	 * @brief Maximum number of active portals.
	 *
	 * Maximum number of HW portals that may be created/opened.
	 */
	#define HW_PORTAL_MAX (HAL_PORTAL_CREATE_MAX + HAL_PORTAL_OPEN_MAX + 1)

	/**
	 * @brief Maximum number of virtual portals.
	 *
	 * Maximum number of virtual portals that may be created/opened.
	 */
	#define KPORTAL_MAX (64)

	/**
	 * @brief Portal message header size.
	 *
	 */
	#define KPORTAL_MESSAGE_HEADER_SIZE (HAL_PORTAL_RESERVED_SIZE)

	/**
	 * @brief Portal single data size.
	 */
	#define KPORTAL_MESSAGE_DATA_SIZE (HAL_PORTAL_DATA_SIZE)

	/**
	 * @brief Maximum size of transfer data.
	 */
	#define KPORTAL_MAX_SIZE (1 * MB)

	/**
	 * @brief Auxiliary definitions for src header check.
	 */
	#define PORTAL_ANY_SOURCE PROCESSOR_NOC_NODES_NUM
	#define PORTAL_ANY_PORT   KPORTAL_PORT_NR

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Maximum number of message buffer resources.
	 *
	 * Maximum number of message buffers used to hold temporary data on kernel space.
	 */
	#define KPORTAL_MESSAGE_BUFFERS_MAX (32)

	/**
	 * @brief Maximum number of auxiliar message buffer resources.
	 *
	 * Maximum number of message buffers used to hold temporary data on kernel space.
	 * WARNING: That constant uses a subset of mbuffers set by @c KPORTAL_MESSAGE_BUFFERS_MAX
	 */
	#define KPORTAL_AUX_BUFFERS_MAX (16)

#if !__NANVIX_IKC_USES_ONLY_MAILBOX

	/**
	 * @brief Creates a virtual portal.
	 *
	 * @param local Logic ID of the Local Node.
	 * @param port  Logic ID of the Local Node port.
	 *
	 * @returns Upon successful completion, the ID of a newly created virtual
	 * portal is returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	EXTERN int do_vportal_create(int local, int port);

	/**
	 * @brief Enables read operations from a remote.
	 *
	 * @param portalid    ID of the target virtual portal.
	 * @param remote      Logic ID of target node.
	 * @param remote_port Target port number in @p remote.
	 *
	 * @returns Upons successful completion zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	EXTERN int do_vportal_allow(int portalid, int remote, int remote_port);

	/**
	 * @brief Opens a virtual portal.
	 *
	 * @param local       Logic ID of the local NoC node.
	 * @param remote      Logic ID of the target NoC node.
	 * @param remote_port Target port number in @p remote.
	 *
	 * @returns Upon successful completion, the ID of the target virtual
	 * portal is returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	EXTERN int do_vportal_open(int local, int remote, int remote_port);

	/**
	 * @brief Destroys a virtual portal.
	 *
	 * @param portalid ID of the target virtual portal.
	 *
	 * @returns Upon successful completion zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	EXTERN int do_vportal_unlink(int portalid);

	/**
	 * @brief Closes a virtual portal.
	 *
	 * @param portalid ID of the target virtual portal.
	 *
	 * @returns Upon successful completion zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	EXTERN int do_vportal_close(int portalid);

	/**
	 * @brief Reads data asynchronously from a virtual portal.
	 *
	 * @param portalid ID of the target virtual portal.
	 * @param buffer   Location from where data should be written.
	 * @param size     Number of bytes to read.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vportal_aread(int portalid, void * buffer, size_t size);

	/**
	* @brief Writes data asynchronously to a virtual portal.
	*
	* @param portalid ID of the target virtual portal.
	* @param buffer   Location from where data should be read.
	* @param size     Number of bytes to write.
	*
	* @returns Upon successful, zero is returned. Upon failure, a
	* negative error code is returned instead.
	*/
	EXTERN int do_vportal_awrite(int portalid, const void * buffer, size_t size);

	/**
	 * @brief Waits for an asynchronous operation on a virtual portal to complete.
	 *
	 * @param portalid ID of the target virtual portal.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int do_vportal_wait(int portalid);

	/**
	 * @brief Performs control operations in a virtual portal.
	 *
	 * @param portalid Target virtual portal.
	 * @param request  Request.
	 * @param args     Additional arguments.
	 *
	 * @param Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	EXTERN int do_vportal_ioctl(int portalid, unsigned request, va_list args);

	/**
	 * @brief Gets the @p portalid logic port.
	 *
	 * @param portalid Target portal.
	 *
	 * @returns Upon successful completion, the logic port is returned.
	 * Upon failure, a negative error code is returned instead.
	 */
	EXTERN int do_vportal_get_port(int portalid);

	/**
	 * @brief Initializes the portal facility.
	 */
	EXTERN void vportal_init(void);

#endif /* !__NANVIX_IKC_USES_ONLY_MAILBOX */

#endif /* __NANVIX_MICROKERNEL */

#endif /* NANVIX_PORTAL_H_ */

/**@}*/
