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

/* Must come first. */
#define __NEED_SECTION_GUARD

#include <nanvix/hal.h>
#include <nanvix/kernel/event.h>
#include <nanvix/const.h>
#include <posix/errno.h>

/**
 * @brief Table information about events.
 */
EXTENSION PRIVATE struct kevent_info
{
	kevent_handler handler; /**< Event handler */
} kevent_handlers[KEVENT_NUM];

/**
 * @brief Table information about events.
 */
EXTENSION PRIVATE struct kevent
{
	uint64_t requests;
	uint64_t handled;
} kevents[CORES_NUM];

/**
 * @brief Event Handlers Table lock.
 */
PRIVATE spinlock_t kevent_lock;

/*============================================================================*
 * kevent_dispatcher()                                                        *
 *============================================================================*/

/**
 * @brief Handles HAL events interrupts.
 *
 * @details This function can be called in the normal execution of a thread if
 * it lose interrupt when working a high level of interrupts. So, when the
 * core verify that has pending events but not handled when execute the wait
 * funtions, it will execute the handler by itself with a none interrupt level.
 * This occurs at HAL events.
 */
PRIVATE void kevent_dispatcher(void)
{
	int coreid;             /* Local core ID.       */
	kevent_handler handler; /* Microkernel handler. */

	spinlock_lock(&kevent_lock);

		coreid = core_get_id();

		/* While local core has events. */
		while (kevents[coreid].requests)
		{
			for (uint64_t ev = 0; ev < KEVENT_NUM; ++ev)
			{
				/* Unregistered event. */
				if (!(kevents[coreid].requests & (1ULL << ev)))
					continue;

				/* Event handled. */
				kevents[coreid].requests &= ~(1ULL << ev);
				kevents[coreid].handled  |=  (1ULL << ev);

				handler = kevent_handlers[ev].handler;

				/* Valid handler. */
				if (handler)
				{
					spinlock_unlock(&kevent_lock);
						handler(ev);
					spinlock_lock(&kevent_lock);

					coreid = core_get_id();
				}
			}
		}

	spinlock_unlock(&kevent_lock);
}

/*============================================================================*
 * kevent_set_handler()                                                       *
 *============================================================================*/

/**
 * @brief Sets a new event handler.
 *
 * @param evnum   Event ID.
 * @param handler Event handler.
 *
 * @returns Zero if successfully changes the handler, non zero otherwise.
 */
PUBLIC int kevent_set_handler(int evnum, kevent_handler handler)
{
	struct section_guard guard; /* Section guard. */

	if (UNLIKELY(!WITHIN(evnum, 0, KEVENT_NUM)))
		return (-EINVAL);

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &kevent_lock, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);
		kevent_handlers[evnum].handler = handler;
	section_guard_exit(&guard);

	return (0);
}

/*============================================================================*
 * kevent_notify()                                                            *
 *============================================================================*/

/**
 * @brief Notify the event @p evnum to the core @p coreid.
 *
 * @param evnum  Event ID.
 * @param coreid Core ID.
 *
 * @returns Zero if successfully sends the event, non zero otherwise.
 */
PUBLIC int kevent_notify(int evnum, int coreid)
{
	int ret;                    /* Return value.  */
	struct section_guard guard; /* Section guard. */

	if (UNLIKELY(!WITHIN(evnum, 0, KEVENT_NUM)))
		return (-EINVAL);

	if (UNLIKELY(!WITHIN(coreid, 0, CORES_NUM)))
		return (-EINVAL);

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &kevent_lock, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);

		/* Sets event request. */
		kevents[coreid].requests |= (1ULL << evnum);

		/* Send event notification. */
		ret = event_notify(coreid);

	section_guard_exit(&guard);

	return (ret);
}

/*============================================================================*
 * kevent_wait()                                                              *
 *============================================================================*/

/**
 * @brief Waits for the receipt of the event @p evnum.
 *
 * @param evnum Event ID.
 *
 * @returns Zero if successfully receives the event, non zero otherwise.
 */
PUBLIC int kevent_wait(int evnum)
{
	int coreid;                 /* Local core ID.                */
	bool handled;               /* Was the target event handled? */
	struct section_guard guard; /* Section guard.                */

	if (UNLIKELY(!WITHIN(evnum, 0, KEVENT_NUM)))
		return (-EINVAL);


	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &kevent_lock, INTERRUPT_LEVEL_NONE);

	do
	{
		section_guard_entry(&guard);

			coreid = core_get_id();

			handled = kevents[coreid].handled & (1 << evnum);

			if (!handled)
			{
				section_guard_exit(&guard);

					event_wait();

				section_guard_entry(&guard);

				handled = kevents[coreid].handled & (1 << evnum);
			}

			kevents[coreid].handled &= ~(1 << evnum);

		section_guard_exit(&guard);
	}
	while (UNLIKELY(!handled));

	return (0);
}

/*============================================================================*
 * kevent_drop()                                                              *
 *============================================================================*/

/**
 * @brief Drops event of a event type.
 *
 * @param evnum Event ID.
 *
 * @returns Zero if successfully receives the event, non zero otherwise.
 */
PUBLIC void kevent_drop(void)
{
	int coreid;                 /* Local core ID.       */
	uint64_t handled;           /* Handled events mask. */
	uint64_t requests;          /* Request events mask. */
	struct section_guard guard; /* Section guard.       */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &kevent_lock, INTERRUPT_LEVEL_NONE);

	section_guard_entry(&guard);

		coreid = core_get_id();

		requests = kevents[coreid].requests;
		handled  = kevents[coreid].handled;

		kevents[coreid].requests = 0ULL;
		kevents[coreid].handled  = 0ULL;
		requests = kevents[coreid].requests;
	section_guard_exit(&guard);

	kprintf("[kevent] Dropping events (requests:%x handled:%x)",
		requests,
		handled
	);
}

/*============================================================================*
 * kevent_init()                                                              *
 *============================================================================*/

/**
 * @brief Setup kernel events.
 */
PUBLIC void kevent_init(void)
{
	/* Initialize the event lock. */
	spinlock_init(&kevent_lock);

	/* Sets the default handler to NULL. */
	for (int i = 0; i < KEVENT_NUM; ++i)
		kevent_handlers[i].handler = NULL;

	/* Reset requests and handled events flags. */
	for (int i = 0; i < CORES_NUM; ++i)
	{
		kevents[i].requests = 0ULL;
		kevents[i].handled  = 0ULL;
	}

	/* Configure the HAL event handler. */
	KASSERT(event_register_handler(kevent_dispatcher) == 0);
}
