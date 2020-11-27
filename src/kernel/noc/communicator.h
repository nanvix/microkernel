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
 * @defgroup kernel-noc Noc Facility
 * @ingroup kernel
 *
 * @brief Noc Facility
 */

#ifndef NANVIX_NOC_COMMUNICATOR_H_
#define NANVIX_NOC_COMMUNICATOR_H_

	/* Must come first. */
	#define __NEED_RESOURCE

	#include <nanvix/hal.h>
	#include <nanvix/hal/resource.h>
	#include <nanvix/kernel/mm.h>
	#include <nanvix/hlib.h>
	#include <nanvix/const.h>
	#include <posix/errno.h>
	#include <posix/stdarg.h>

	#include "active.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

	/**
	 * @name Resource flags.
	 */
	/**@{*/
	#define COMMUNICATOR_FLAGS_FINISHED (1 << 0) /**< Has it finished?     */
	#define COMMUNICATOR_FLAGS_ALLOWED  (1 << 1) /**< Has it been allowed? */
	/**@}*/

	/**
	 * @name I/O Control types.
	 */
	/**@{*/
	#define COMM_IOCTL_GET_VOLUME   (1) /**< Gets communication volume.            */
	#define COMM_IOCTL_GET_LATENCY  (2) /**< Gets communication latency.           */
	#define COMM_IOCTL_GET_NCREATES (3) /**< Gets number of creates.               */
	#define COMM_IOCTL_GET_NUNLINKS (4) /**< Gets number of unlinks.               */
	#define COMM_IOCTL_GET_NOPENS   (5) /**< Gets number of opens.                 */
	#define COMM_IOCTL_GET_NCLOSES  (6) /**< Gets number of closes.                */
	#define COMM_IOCTL_GET_NREADS   (7) /**< Gets number of reads.                 */
	#define COMM_IOCTL_GET_NWRITES  (8) /**< Gets number of writes.                */
	#define COMM_IOCTL_SET_REMOTE   (9) /**< Sets the remote_addr until next read. */
	/**@}*/

	/**
	 * @brief Communicator initializer.
	 */
	#define COMMUNICATOR_INITIALIZER(_do_release, _do_comm, _do_wait) { \
		.resource   = RESOURCE_INITIALIZER,                             \
		.config     = ACTIVE_CONFIG_INITIALIZER,                        \
		.stats      = PSTATS_INITIALIZER,                               \
		.lock       = SPINLOCK_UNLOCKED,                                \
		.do_release = _do_release,                                      \
		.do_comm    = _do_comm,                                         \
		.do_wait    = _do_wait,                                         \
	}

	/*============================================================================*
	 * Counters structure.                                                        *
	 *============================================================================*/

	/**
	 * @brief Communicator counters.
	 */
	struct communicator_counters
	{
		spinlock_t lock;   /**< Protection.        */
		uint64_t ncreates; /**< Number of creates. */
		uint64_t nunlinks; /**< Number of unlinks. */
		uint64_t nopens;   /**< Number of opens.   */
		uint64_t ncloses;  /**< Number of closes.  */
		uint64_t nreads;   /**< Number of reads.   */
		uint64_t nwrites;  /**< Number of writes.  */
	};

	/*============================================================================*
	 * Communicator structure definition.                                         *
	 *============================================================================*/

	/**
	 * @brief Auxiliar functions.
	 */
	struct communicator_functions
	{
		active_release_fn do_release;          /**< Active release function. */
		active_comm_fn do_comm;                /**< Active comm function.    */
		active_wait_fn do_wait;                /**< Active wait function.    */
		active_laddress_calc_fn laddress_calc; /**< Active laddress calc.    */
	};

	/**
	 * @brief Communicator structure.
	 */
	struct communicator
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource;                /**< Generic resource information. */

		int flags;                               /**< Auxiliar flags.               */
		struct active_config config;             /**< Communicaton configuration.   */
		struct pstats stats;                     /**< Performance Statistics.       */
		struct communicator_counters * counters; /**< Global counters.              */
		spinlock_t lock;                         /**< Protection.                   */

		/**
		 * @name Auxiliar functions.
		 */
		/**@{*/
		struct communicator_functions * fn;      /**< Active release function.      */
		/**@}*/
	};

	/**
	* @brief Resource pool.
	*/
	struct communicator_pool
	{
		struct communicator * communicators;      /**< Pool of communicators.   */
		int ncommunicators;		                  /**< Number of communicators. */
	};

	/*============================================================================*
	 * Communicator interface.                                                    *
	 *============================================================================*/

	EXTERN int communicator_alloc(const struct communicator_pool *, struct active_config *, int);
	EXTERN int communicator_free(const struct communicator_pool *, int, int);
	EXTERN ssize_t communicator_operate(struct communicator *, int);
	EXTERN int communicator_wait(struct communicator *);
	EXTERN int communicator_ioctl(struct communicator *, unsigned, va_list);

	/**
	 * @brief Sets a communicator as finished.
	 *
	 * @param comm Target communicator.
	 */
	static inline void communicator_set_finished(struct communicator *comm)
	{
		comm->flags |= COMMUNICATOR_FLAGS_FINISHED;
	}

	/**
	 * @brief Sets a communicator as not finished.
	 *
	 * @param comm Target communicator.
	 */
	static inline void communicator_set_notfinished(struct communicator *comm)
	{
		comm->flags &= ~COMMUNICATOR_FLAGS_FINISHED;
	}

	/**
	 * @brief Sets a communicator as allowed.
	 *
	 * @param comm Target communicator.
	 */
	static inline void communicator_set_allowed(struct communicator *comm)
	{
		comm->flags |= COMMUNICATOR_FLAGS_ALLOWED;
	}

	/**
	 * @brief Sets a communicator as not allowed.
	 *
	 * @param comm Target communicator.
	 */
	static inline void communicator_set_notallowed(struct communicator *comm)
	{
		comm->flags &= ~COMMUNICATOR_FLAGS_ALLOWED;
	}

	/**
	 * @brief Asserts whether or not a communicator is finished.
	 *
	 * @param comm Target communicator.
	 *
	 * @returns One if the target communicator is finished, and zero otherwise.
	 */
	static inline int communicator_is_finished(const struct communicator *comm)
	{
		return (comm->flags & COMMUNICATOR_FLAGS_FINISHED);
	}

	/**
	 * @brief Asserts whether or not a communicator is allowed.
	 *
	 * @param comm Target communicator.
	 *
	 * @returns One if the target communicator is allowed, and zero otherwise.
	 */
	static inline int communicator_is_allowed(const struct communicator *comm)
	{
		return (comm->flags & COMMUNICATOR_FLAGS_ALLOWED);
	}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */

#endif /* NANVIX_NOC_COMMUNICATOR_H_ */

/**@}*/
