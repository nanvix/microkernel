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

#ifndef NANVIX_NOC_ACTIVE_H_
#define NANVIX_NOC_ACTIVE_H_

	/* Must come first. */
	#define __NEED_RESOURCE

	#include <nanvix/hal.h>
	#include <nanvix/hal/resource.h>
	#include <nanvix/kernel/thread.h>
	#include <nanvix/hlib.h>
	#include <nanvix/const.h>
	#include <posix/errno.h>
	#include <posix/stdarg.h>

	#include "mbuffer.h"
	#include "port.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

	/**
	 * @brief Resource flags.
	 */
	/**@{*/
	#define ACTIVE_FLAGS_ALLOWED (1 << 0) /**< Has it been allowed. */
	/**@}*/

	/**
	 * @brief Composes the logic address based on logid id, logic portid and number of ports.
	 */
	#define ACTIVE_LADDRESS_COMPOSE(_fd, _port, _nports) (_fd * (_nports + 1) + _port)

	/**
	 * @name Communication types.
	 */
	/**@{*/
	#define ACTIVE_TYPE_INPUT  (0) /**< Input communication.  */
	#define ACTIVE_TYPE_OUTPUT (1) /**< Output communication. */
	/**@}*/

	/**
	 * @name Communication status.
	 */
	/**@{*/
	#define ACTIVE_COMM_SUCCESS  (0)
	#define ACTIVE_COMM_AGAIN    (1)
	#define ACTIVE_COMM_RECEIVED (2)
	/**@}*/

	/**
	 * @name Communication mode.
	 */
	/**@{*/
	#define ACTIVE_COMM_ONE_PHASE_MODE (0)
	#define ACTIVE_COMM_TWO_PHASE_MODE (1)
	/**@}*/

	/**
	 * @name Structures initializers.
	 */
	/**@{*/
	#define ACTIVE_CONFIG_INITIALIZER (struct active_config){-1, -1, -1, NULL, 0ULL}
	#define PSTATS_INITIALIZER        (struct pstats){0ULL, 0ULL}
	#define REQUESTS_FIFO_INITIALIZER (struct requests_fifo){0, 0, 0, 0, NULL}
	/**@}*/

	/*============================================================================*
	 * Auxiliar Structures.                                                       *
	 *============================================================================*/

	/**
	 * @brief Configuration helper.
	 */
	struct active_config
	{
		int fd;              /**< Active id.          */
		int local_addr;      /**< Local address.      */
		int remote_addr;     /**< Remote address.     */
		const void * buffer; /**< User level buffer.  */
		size_t size;         /**< Data transfer size. */
	};

	/**
	 * @brief Measure performance statistics helper.
	 */
	struct pstats
	{
		size_t volume;    /**< Amount of data transferred. */
		uint64_t latency; /**< Transfer latency.           */
	};

	/**
	 * @brief Circular FIFO to hold write requests.
	 */
	struct requests_fifo
	{
		short head;         /**< Index of the first element. */
		short tail;         /**< Index of the last element.  */
		short max_capacity; /**< Max number of elements.     */
		short nelements;    /**< Nr of elements in the fifo. */
		short * fifo;       /**< Buffer to store elements.   */
	};

	struct active;

	/**
	 * @name Hardware interface abstractions.
	 */
	/**@{*/
	typedef int (* hw_create_fn)(int);
	typedef int (* hw_open_fn)(int, int);
	typedef int (* hw_allow_fn)(struct active *, int);
	typedef ssize_t (* hw_aread_fn)(int mbxid, void *buffer, uint64_t size);
	typedef ssize_t (* hw_awrite_fn)(int mbxid, const void *buffer, uint64_t size);
	typedef int (* hw_wait_fn)(int);
	typedef int (* hw_config_fn)(struct mbuffer *, const struct active_config *);
	typedef int (* hw_check_fn)(struct mbuffer *, const struct active_config *);
	typedef int (* hw_getter_fn)(int);
	/**@}*/

	/**
	 * @name Active interface abstractions.
	 */
	/**@{*/
	typedef int (* active_release_fn)(int);
	typedef ssize_t (* active_comm_fn)(int, const struct active_config *, struct pstats *);
	typedef int (* active_wait_fn)(int, const struct active_config *, struct pstats *);
	typedef int (* active_laddress_calc_fn)(int, int);
	/**@}*/

	/*============================================================================*
	 * Active structure definition.                                               *
	 *============================================================================*/

	/**
	 * @name Auxiliary functions.
	 */
	struct active_functions
	{
		hw_create_fn do_create;        /**< Hardware create function.          */
		hw_open_fn do_open;            /**< Hardware open function.            */
		hw_allow_fn do_allow;          /**< Hardware allow function.           */
		hw_aread_fn do_aread;          /**< Hardware aread function.           */
		hw_awrite_fn do_awrite;        /**< Hardware awrite function.          */
		hw_wait_fn do_wait;            /**< Hardware wait function.            */
		hw_config_fn do_header_config; /**< Header config function.            */
		hw_check_fn do_header_check;   /**< Header checker function.           */
		hw_getter_fn get_actid;        /**< Gets active id from a composed ID. */
		hw_getter_fn get_portid;       /**< Gets port id from a composed ID.   */
	};

	/**
	 * @brief Table of active mailboxes.
	 */
	struct active
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource;              /**< Generic resource information.               */

		/**
		 * @name Identification variables.
		 */
		/**@{*/
		int flags;                             /**< Auxiliary flags.                            */
		int hwfd;                              /**< Underlying file descriptor.                 */
		int local;                             /**< Local node number.                          */
		int remote;                            /**< Target node number.                         */
		int refcount;                          /**< References counter.                         */
		size_t size;                           /**< Data transfer size.                         */
		/**@}*/

		/**
		 * @name Auxiliar resources pools.
		 */
		/**@{*/
		struct port_pool portpool;             /**< Pool of port.                               */
		struct mbuffer_pool * mbufferpool;     /**< Pool with all of mbuffer available.         */
		struct mbuffer_pool * mbufferpool_aux; /**< Pool with subset of mbuffer in mbufferpool. */
		/**@}*/

		/**
		 * @name Control variables.
		 */
		/**@{*/
		struct requests_fifo requests;         /**< Ring buffer of awrite requests.             */
		spinlock_t lock;                       /**< Protection.                                 */
		/**@}*/

		/**
		 * @name Wait/Wakeup controllers.
		 */
		/**@{*/
		struct semaphore waiting;              /**< Thread queue waiting for a communication.   */
		/**@}*/

		/**
		 * @name Auxiliary functions.
		 */
		/**@{*/
		struct active_functions * fn;
		/**@}*/
	};

	/**
	 * @brief Active pool.
	 */
	struct active_pool
	{
		struct active * actives; /**< Pool of actives.    */
		int nactives;            /**< Number of actives.  */
	};

	/*============================================================================*
	 * Active interface.                                                          *
	 *============================================================================*/

	EXTERN int active_alloc(const struct active_pool *, int, int, int, int);
	EXTERN int active_release(const struct active_pool *, int);
	EXTERN ssize_t active_aread(const struct active_pool *, int, const struct active_config *, struct pstats *);
	EXTERN ssize_t active_awrite(const struct active_pool *, int, const struct active_config *, struct pstats *);
	EXTERN int active_wait(const struct active_pool *, int, const struct active_config *, struct pstats *);
	EXTERN int _active_create(const struct active_pool *, int);
	EXTERN int _active_open(const struct active_pool *, int, int);

	/**
	 * @brief Sets a active as allowed.
	 *
	 * @param active Target active.
	 */
	static inline void active_set_allowed(struct active * active)
	{
		active->flags |= ACTIVE_FLAGS_ALLOWED;
	}

	/**
	 * @brief Sets a active as not allowed.
	 *
	 * @param active Target active.
	 */
	static inline void active_set_notallowed(struct active * active)
	{
		active->flags &= ~ACTIVE_FLAGS_ALLOWED;
	}

	/**
	 * @brief Asserts whether or not a active is allowed.
	 *
	 * @param active Target active.
	 *
	 * @returns One if the target active is allowed, and zero otherwise.
	 */
	static inline int active_is_allowed(const struct active * active)
	{
		return (active->flags & ACTIVE_FLAGS_ALLOWED);
	}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */

#endif /* NANVIX_NOC_ACTIVE_H_ */

/**@}*/
