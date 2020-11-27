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

#ifndef NANVIX_NOC_PORT_H_
#define NANVIX_NOC_PORT_H_

	/* Must come first. */
	#define __NEED_RESOURCE

	#include <nanvix/hal.h>
	#include <nanvix/hal/resource.h>
	#include <nanvix/hlib.h>
	#include <nanvix/const.h>
	#include <posix/errno.h>
	#include <posix/stdarg.h>

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

	/**
	 * @brief Resource flags.
	 */
	/**@{*/
	#define PORT_FLAGS_REQUESTED (1 << 0) /**< Has it requested an op? */
	/**@}*/

   /*============================================================================*
	* Port Structures.                                                           *
	*============================================================================*/

	/**
	 * @brief Struct that represents a port abstraction.
	 */
	struct port
	{
		/*
		* XXX: Don't Touch! This Must Come First!
		*/
		struct resource resource; /**< Generic resource information. */
		short flags;              /**< Auxiliar flags.               */
		short mbufferid;          /**< Mbuffer ID.                   */
		void * mbufferpool;       /**< Mbuffer pool.                 */
	};

	/**
	 * @brief Port pool.
	 */
	struct port_pool
	{
		struct port * ports; /**< Pool of ports.      */
		int nports;          /**< Number of ports.    */
		int used_ports;      /**< Nr of ports in use. */
	};

   /*============================================================================*
	* Port interface.                                                            *
	*============================================================================*/

	EXTERN int portpool_choose_port(const struct port_pool *);

	/**
	 * @brief Sets a port as requested.
	 *
	 * @param port Target port.
	 */
	static inline void port_set_requested(struct port * port)
	{
		port->flags |= PORT_FLAGS_REQUESTED;
	}

	/**
	 * @brief Sets a port as not requested.
	 *
	 * @param port Target port.
	 */
	static inline void port_set_notrequested(struct port * port)
	{
		port->flags &= ~PORT_FLAGS_REQUESTED;
	}

	/**
	 * @brief Asserts whether or not a port is requested.
	 *
	 * @param port Target port.
	 *
	 * @returns One if the target port is requested, and zero otherwise.
	 */
	static inline int port_is_requested(const struct port * port)
	{
		return (port->flags & PORT_FLAGS_REQUESTED);
	}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */

#endif /* NANVIX_NOC_PORT_H_ */

/**@}*/
