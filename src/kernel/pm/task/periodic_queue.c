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

#include "periodic_queue.h"

#if __NANVIX_USE_TASKS

/**
 * @brief Cast a resource pointer to a task pointer.
 */
#define TASK_PTR(t) ((struct task *) t)

/*============================================================================*
 * periodic_task_order()                                                      *
 *============================================================================*/

/**
 * @brief Compare two resource to order them. Specifically, if b has a period
 * that is less than the period of a, we sinalizes that b is smaller and it
 * will be insert before a. Otherwise, we decrement the period of b by the
 * period o @p a.
 *
 * @param a Current period task on arrangement.
 * @param b New periodic task.
 *
 * @return 1 if b is smaller than a, -1 otherwise.
 */
PRIVATE int periodic_task_order(struct resource * a, struct resource * b)
{
	struct task * curr = TASK_PTR(a);
	struct task * newt = TASK_PTR(b);

	if (newt->period < curr->period)
		return (1);

	newt->period -= curr->period;

	return (-1);
}

/*============================================================================*
 * periodic_task_enqueue()                                                    *
 *============================================================================*/

/**
 * @brief Enqueue a periodic resource on a periodic queue.
 *
 * @param arr Periodic queue
 * @param t   New periodic task 
 */
PUBLIC int periodic_task_enqueue(
	struct resource_arrangement * arr,
	struct task * t
)
{
	/* Sanity checks. */
	KASSERT(!arr || !t);

	/* Insert in order based on period. */
	KASSERT(resource_insert_ordered(arr, &t->resource, periodic_task_order) >= 0);

	/**
	 * Update next period.
	 *
	 * @details We need to do this because the next task needs to have its period
	 * based on the new task inserted.
	 */
	if (t->resource.next)
		TASK_PTR(t->resource.next)->period -= t->period;

	return (0);
}

/*============================================================================*
 * periodic_task_remove()                                                     *
 *============================================================================*/

/**
 * @brief Remove a periodic resource from a periodic queue.
 *
 * @param arr Periodic queue
 * @param t   Periodic task 
 */
PUBLIC int periodic_task_remove(
	struct resource_arrangement * arr,
	struct task * t
)
{
	/* Sanity checks. */
	KASSERT(!arr || !t);

	/* Not found? */
	if (resource_search(arr, &t->resource) < 0)
		return (-EINVAL);

	/**
	 * Update next period.
	 *
	 * @details We must add the period that it will be remove to the next task.
	 */
	if (t->resource.next)
		TASK_PTR(t->resource.next)->period += t->period;

	/* Remove it. */
	return (resource_pop(arr, &t->resource));
}

/*============================================================================*
 * periodic_task_tick()                                                       *
 *============================================================================*/

/**
 * @brief Try to dequeue a resource from a queue, but it is poped only if its
 * period reach 0.
 *
 * @param arr Periodic resource queue
 *
 * @returns Valid task if it reachs 0 on its period. NULL otherwise.
 */
PUBLIC struct task * periodic_task_dequeue(
	struct resource_arrangement * arr
)
{
	struct task * head;

	KASSERT(arr != NULL);

	/* Empty queue. */
	if ((head = TASK_PTR(arr->head)) == NULL)
		return (NULL);

	/* The head period is over. */
	if (head->period == 0)
	{
		KASSERT(resource_dequeue(arr) == &head->resource);
		head->period = -1;

		return (head);
	}

	/* Decrement head period. */
	head->period--;

	/* The head period is not over. */
	return (NULL);
}

/*============================================================================*
 * periodic_task_next_period()                                                *
 *============================================================================*/

/**
 * @brief Get the period of the head.
 *
 * @param queue Periodic resource queue
 *
 * @returns Greater or equal to 0 if the queue head is valid, Negative otherwise.
 */
PUBLIC int periodic_task_next_period(struct resource_arrangement * arr)
{
	KASSERT(arr != NULL);

	if (arr->head == NULL)
		return (-1);

	return (TASK_PTR(arr->head)->period);
}

#endif /* __NANVIX_USE_TASKS */
