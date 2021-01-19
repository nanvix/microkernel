/*
 * MIT License
 *
 * Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
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
 * @defgroup kernel- Event System
 * @ingroup kernel
 *
 * @brief Event System
 */

#ifndef NANVIX_EVENT_H_
#define NANVIX_EVENT_H_

	/* External dependencies. */
	#include <nanvix/hal.h>
	#include <nanvix/const.h>

/*============================================================================*
 * Signal System                                                              *
 *============================================================================*/

	/**
	 * @name Kernel Event Types
	 */
	/**@{*/
	#define KEVENT_WAKEUP 0 /**< Wake up.    */
	#define KEVENT_SCHED  1 /**< Scheduling. */
	#define KEVENT_TASK   2 /**< Task.       */
	#define _KEVENT_LIMIT 3 /**< Limit.      */
	/**@}*/

	/**
	 * @brief Number of mapped events.
	 *
	 * @note Must be updated every time that a new event type is added.
	 */
	/**@}*/
	#define KEVENT_NUM _KEVENT_LIMIT 
	/**@}*/

	/**
	 * @brief Signal Action handler.
	 */
	typedef void (*kevent_handler)(int);

	/**
	 * @brief Sets a new event handler.
	 *
	 * @param evnum   Event ID.
	 * @param handler Event handler.
	 *
	 * @returns Zero if successfully changes the handler, non zero otherwise.
	 */
	EXTERN int kevent_set_handler(int evnum, kevent_handler handler);

	/**
	 * @brief Notifies an event on a core.
	 *
	 * @param evnum  Event ID.
	 * @param coreid Core ID.
	 *
	 * @returns Zero if successfully sends the signal, non zero otherwise.
	 */
	EXTERN int kevent_notify(int evnum, int coreid);

	/**
	 * @brief Waits for the receipt of a event.
	 *
	 * @param evnum Event ID.
	 *
	 * @returns Zero if successfully receives the signal, non zero otherwise.
	 */
	EXTERN int kevent_wait(int evnum);

	/**
	 * @brief Drops event of a event type.
	 *
	 * @param evnum Event ID.
	 *
	 * @returns Zero if successfully receives the signal, non zero otherwise.
	 */
	EXTERN void kevent_drop(void);

	/**
	 * @brief Setup kernel events.
	 */
	EXTERN void kevent_init(void);

#endif /* NANVIX_EVENT_H_ */

/**@}*/
