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

#ifndef NANVIX_NOC_MBUFFER_H_
#define NANVIX_NOC_MBUFFER_H_

	/* Must come first. */
	#define __NEED_RESOURCE

	#include <nanvix/hal.h>
	#include <nanvix/hal/resource.h>
	#include <nanvix/kernel/mailbox.h>
	#include <nanvix/kernel/portal.h>
	#include <nanvix/hlib.h>
	#include <nanvix/const.h>
	#include <posix/errno.h>
	#include <posix/stdarg.h>

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

	/**
	 * @name Release modes of mbuffers.
	 */
	/**@{*/
	#define MBUFFER_DISCARD_MESSAGE (0) /**< Discard message and releases the mbuffer. */
	#define MBUFFER_KEEP_MESSAGE    (1) /**< Keep message intact.                      */
	/**@}*/

	/**
	 * @brief Size of the mbuffer message header.
	 */
	#define MBUFFER_HEADER_SIZE (4 * 4) /**< sizeof(struct mbuffer_header) */

	/**
	 * @brief Mbuffer message initializer.
	 */
	#define MBUFFER_MESSAGE_INITIALIZER (struct mbuffer_message){ \
		.header = {-1, -1, 0, 0},                                 \
		.data   = '\0',                                           \
	}

	/**
	 * @brief Mbuffer initializer.
	 */
	#define MBUFFER_INITIALIZER (struct mbuffer){   \
		.abstract = {                               \
			.resource = RESOURCE_INITIALIZER,       \
			.age      = ~(0ULL),                    \
			.latency  = (0ULL),                     \
			.actid    = (-1),                       \
			.portid   = (-1),                       \
			.message  = MBUFFER_MESSAGE_INITIALIZER \
		}                                           \
	}

	/*============================================================================*
	 * Auxiliar Structures.                                                       *
	 *============================================================================*/

	/**
	 * @brief Mbuffer header.
	 */
	struct mbuffer_header
	{
		int dest;   /* Data sender.         */
		int src;    /* Data destination.    */
		int size;   /* Message data size.   */
		int unused; /* Padding to 16 bytes. */
	};

	/**
	 * @name Sanity checks of mbuffer header.
	 */
	/**@{*/
	#if (MBUFFER_HEADER_SIZE > KPORTAL_MESSAGE_HEADER_SIZE)
	#error Mbuffer header does not fit in the portal header.
	#endif
	#if (MBUFFER_HEADER_SIZE < KPORTAL_MESSAGE_HEADER_SIZE)
	#warning Mbuffer header is smaller then portal header.
	#endif
	#if (MBUFFER_HEADER_SIZE > KMAILBOX_MESSAGE_HEADER_SIZE)
	#error Mbuffer header does not fit in the mailbox header.
	#endif
	#if (MBUFFER_HEADER_SIZE < KMAILBOX_MESSAGE_HEADER_SIZE)
	#warning Mbuffer header is smaller then mailbox header.
	#endif
	/**@}*/

	/**
	 * @brief Abstract mbuffer message.
	 */
	struct mbuffer_message
	{
		struct mbuffer_header header; /* Header.     */
		char data;                    /* First data. */
	};

	/*============================================================================*
	 * Mbuffers structures.                                                       *
	 *============================================================================*/

	/**
	 * @brief Mailbox message.
	 */
	struct mailbox_message
	{
		struct mbuffer_header header;                                    /* Header. */
		char data[KMAILBOX_MESSAGE_DATA_SIZE];                           /* Data.   */
#if (MBUFFER_HEADER_SIZE < KMAILBOX_MESSAGE_HEADER_SIZE)
		char unused[KMAILBOX_MESSAGE_HEADER_SIZE - MBUFFER_HEADER_SIZE]; /* Unused. */
#endif
	} ALIGN(sizeof(dword_t));

	/**
	 * @brief Portal message.
	 */
	struct portal_message
	{
		struct mbuffer_header header;                                   /* Header. */
		char data[KPORTAL_MESSAGE_DATA_SIZE];                           /* Data.   */
#if (MBUFFER_HEADER_SIZE < KPORTAL_MESSAGE_HEADER_SIZE)
		char unused[KPORTAL_MESSAGE_HEADER_SIZE - MBUFFER_HEADER_SIZE]; /* Unused. */
#endif
	} ALIGN(sizeof(dword_t));

	/**
	 * @brief Abstract mbuffer.
	 */
	struct mbuffer
	{
		/*
		 * XXX: Don't Touch! This Must Come First!
		 */
		struct resource resource;       /**< Generic resource information.     */
		uint64_t age;                   /**< Number that guarantees order.     */
		uint64_t latency;               /**< Latency of receiving data.        */
		int actid;                      /**< Sender active ID of local sender. */
		int portid;                     /**< Sender port ID of local sender.   */
		struct mbuffer_message message; /**< Structure that holds a message.   */
	};

	/**
	 * @brief Mailbox mbuffer.
	 */
	union mailbox_mbuffer
	{
		struct mbuffer abstract;
		struct
		{
			/*
			* XXX: Don't Touch! This Must Come First!
			*/
			struct resource resource;       /**< Generic resource information.     */
			uint64_t age;                   /**< Number that guarantees order.     */
			uint64_t latency;               /**< Latency of receiving data.        */
			int actid;                      /**< Sender active ID of local sender. */
			int portid;                     /**< Sender port ID of local sender.   */
			struct mailbox_message message; /**< Structure that holds a message.   */
		};
	} ALIGN(sizeof(dword_t));

	/**
	 * @brief Portal mbuffer.
	 */
	union portal_mbuffer
	{
		struct mbuffer abstract;
		struct
		{
			/*
			* XXX: Don't Touch! This Must Come First!
			*/
			struct resource resource;      /**< Generic resource information.      */
			uint64_t age;                  /**< Number that guarantees order.      */
			uint64_t latency;              /**< Latency of receiving data.         */
			int actid;                     /**< Sender active ID of local sender.  */
			int portid;                    /**< Sender port ID of local sender.    */
			struct portal_message message; /**< Structure that holds a message.    */
		};
	} ALIGN(sizeof(dword_t));

	/**
	 * @brief Prototype function to check source of a message.
	 */
	typedef int (* source_check_fn)(struct mbuffer *, int);

	/**
	 * @brief Resource pool.
	 */
	struct mbuffer_pool
	{
		void * mbuffers;     /**< Pool of mbuffers.        */
		int nmbuffers;		 /**< Number of mbuffers.      */
		size_t mbuffer_size; /**< mbuffer size (in bytes). */
		uint64_t * curr_age; /**< Next mbuffer age.        */
		spinlock_t * lock;   /**< Protection.              */

		source_check_fn source_check; /**< Source header check. */
	};

	/*============================================================================*
	 * Mbuffer interface.                                                         *
	 *============================================================================*/

	EXTERN int mbuffer_alloc(struct mbuffer_pool *);
	EXTERN int mbuffer_release(struct mbuffer_pool *, int, int);
	EXTERN int mbuffer_search(struct mbuffer_pool *, int, int);
	EXTERN struct mbuffer * mbuffer_get(struct mbuffer_pool *, int);

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */

#endif /* NANVIX_NOC_MBUFFER_H_ */

/**@}*/
