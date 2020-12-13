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

/* Must come first. */
#define __NEED_SECTION_GUARD

#include "periodic_queue.h"

#if __NANVIX_USE_TASKS

/*============================================================================*
 * Task system variables                                                      *
 *============================================================================*/

/**
 * @brief Number of helpers nodes.
 */
#define NODE_TASK_MAX 64

/**
 * @brief Helpers
 */
/**@{*/
#define TASK_PTR(x)           ((struct task *) x)                /**< Cast to a task.                     */
#define NODE_PTR(x)           ((struct node_task *) x)           /**< Cast to a node task.                */
#define TASK_GET_FROM_NODE(n) (n ? (NODE_PTR(n)->task) : (NULL)) /**< Gets the task from a node resource. */
/**@}*/

/**
 * @brief Node of a Task.
 *
 * @details This node is used to put a task into a dependency queue.
 */
struct node_task
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource; /**< Resource struct.  */

	struct task * task;       /**< Task struct.      */
};

/**
 * @brief Task board.
 */
PRIVATE struct task_board
{
	/**
	 * @name Control.
	 */
	/**@{*/
	uint64_t counter;                       /**< New task ID control.   */
	spinlock_t lock;                        /**< Board protection.      */
	struct semaphore sem;                   /**< Actives tasks control. */
	/**@}*/

	/**
	 * @name Tracked Tasks.
	 */
	/**@{*/
	struct resource_arrangement actives;    /**< Ready tasks.           */
	struct resource_arrangement waiting;    /**< Blocked tasks.         */
	struct resource_arrangement periodics;  /**< Periodic tasks.        */
	/**@}*/

	/**
	 * @name Nodes control.
	 */
	/**@{*/
	struct resource_arrangement free_nodes; /**< Free nodes.           */
	struct node_task nodes[NODE_TASK_MAX];  /**< Nodes.                */
	/**@}*/
} tasks ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Current task.
 */
PRIVATE struct task * ctask;

/**
 * @brief Shutdown signal.
 */
PRIVATE volatile bool shutdown;

/*============================================================================*
 * Management                                                                 *
 *============================================================================*/

/*============================================================================*
 * __task_dispatch()                                                          *
 *============================================================================*/

/**
 * @brief Enqueue a task to the dispatcher thread operate.
 *
 * @param task Task pointer.
 */
PRIVATE void __task_dispatch(struct task * task)
{
	if (task->id == TASK_NULL_ID)
		task->id = tasks.counter++;

	/* Is a periodic task and is it not the first dispatch? */
	if (task->reload > 0 && task->period == 0)
	{
		/* Change task state to stopped. */
		task->state  = TASK_STATE_STOPPED;
		task->period = task->reload;

		/* Period task. */
		periodic_task_enqueue(&tasks.periodics, task);
	}
	else
	{
		/* Change task state to ready. */
		task->state  = TASK_STATE_READY;
		task->period = 0;

		/* Active task. */
		resource_enqueue(&tasks.actives, &task->resource);
		semaphore_up(&tasks.sem);
	}
}

/*============================================================================*
 * __task_search_node()                                                       *
 *============================================================================*/

/**
 * @brief Global helper to search a specific node.
 */
PRIVATE struct task * _target_child;

/**
 * @brief Search for the node that stores the target task.
 */
PRIVATE bool __task_search_node(struct resource * r)
{
	return (NODE_PTR(r)->task == _target_child);
}

/*============================================================================*
 * __task_disconnect()                                                        *
 *============================================================================*/

/**
 * @brief Disconnect tasks.
 *
 * @param parent Task pointer.
 * @param child  Task pointer.
 *
 * @returns Zero if success disconnect two tasks, and negative number they are not
 * connected.
 */
PRIVATE int __task_disconnect(struct task * parent, struct task * child)
{
	struct resource * node;

	KASSERT(parent && child);

	/* Configure search parameter. */
	_target_child = child;

	/* Not found? */
	if ((node = resource_remove_verify(&parent->children, __task_search_node)) == NULL)
		return (-EAGAIN);

	/* Sanity check. */
	KASSERT(NODE_PTR(node)->task == child);

	/* Disconnnect. */
	child->parents--;

	/* Releases node. */
	NODE_PTR(node)->task = NULL;
	resource_enqueue(&tasks.free_nodes, node);

	/* Success. */
	return (0);
}

/*============================================================================*
 * __task_complete()                                                          *
 *============================================================================*/

/**
 * @brief Complete a task
 *
 * @param task Task pointer.
 */
PRIVATE void __task_complete(struct task * task)
{
	struct task * child;
	struct resource * node;

	/* Sinalizes that the parent task are completed. */
	task->state = TASK_STATE_COMPLETED;

	/* Notifies the children. */
	node = task->children.head;
	while ((child = TASK_GET_FROM_NODE(node)) != NULL)
	{
		KASSERT(child->state != TASK_STATE_ERROR && child->parents > 0);

		/* Gets the next node because disconnect will reset it. */
		node = node->next;

		KASSERT(__task_disconnect(task, child) == 0);

		/* Indicates that a dependency is over. */
		if (child->parents == 0)
			__task_dispatch(child);
	}

	/* Notifies task completation. */
	semaphore_up(&task->sem);
}

/*============================================================================*
 * __task_stop()                                                              *
 *============================================================================*/

/**
 * @brief Remove a task and its children because a error.
 *
 * @param task Task pointer.
 */
PRIVATE void __task_stop(struct task * task)
{
	/* Sinalizes that the task is stopped. */
	task->state = TASK_STATE_STOPPED;

	/* Put it on the waiting list. */
	resource_push_back(&tasks.waiting, &task->resource);
}

/*============================================================================*
 * __task_error()                                                             *
 *============================================================================*/

/**
 * @brief Release task because an error occured.
 *
 * @param task Task pointer.
 */
PRIVATE void __task_error(struct task * task)
{
	struct task * child;
	struct resource * node;

	/* Sinalizes that the parent task enter into a error state. */
	task->state = TASK_STATE_ERROR;

	/* Propagates the error to children. */
	node = task->children.head;
	while ((child = TASK_GET_FROM_NODE(node)) != NULL)
	{
		/* Gets the next node because disconnect will reset it. */
		node = node->next;

		/* Disconnect. */
		KASSERT(__task_disconnect(task, child) == 0);

		/* Is the child already in an error state? */
		if (child->state == TASK_STATE_ERROR)
			continue;

		/* Propagates the error. */
		child->args.ret = task->args.ret;

		/* Go to down (deep-first). */
		__task_error(child);
	}

	/* TODO: Do we need to release the task before or after the children? */
	semaphore_up(&task->sem);
}

/*============================================================================*
 * Dispatcher                                                                 *
 *============================================================================*/

/*============================================================================*
 * task_loop()                                                                *
 *============================================================================*/

PUBLIC void task_loop(void)
{
	int ret;
	struct section_guard guard;

	kprintf("[kernel][dispatcher] Working on core %d!", core_get_id());

	/* We do not want to be interrupted in the critical region. */
	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

	/* Endless loop. */
	while (LIKELY(!shutdown))
	{
		section_guard_exit(&guard);

		/* Waits for tasks. */
		semaphore_down(&tasks.sem);

		/* Dispatch task. */
		section_guard_entry(&guard);
			ctask = TASK_PTR(resource_dequeue(&tasks.actives));
			ctask->state = TASK_STATE_RUNNING;
		section_guard_exit(&guard);

		/* Valid function. */
		KASSERT(ctask->fn != NULL);

		/* Execute the function of the task. */
		ret = ctask->fn(&ctask->args);

		/* Valid return. */
		KASSERT(WITHIN(ret, TASK_RET_ERROR, TASK_RET_STOP + 1));

		section_guard_entry(&guard);

			/* Evaluate the return type. */
			switch (ret)
			{
				/* Complete the task. */
				case TASK_RET_SUCCESS:
					if (LIKELY(ctask->state == TASK_STATE_RUNNING))
						__task_complete(ctask);
					break;

				/* Reschedule the task. */
				case TASK_RET_AGAIN:
					__task_dispatch(ctask);
					break;

				/* Block the task. */
				case TASK_RET_STOP:
					__task_stop(ctask);
					break;

				/* An error occured */
				case TASK_RET_ERROR:
				default:
				{
					KASSERT(((int) ctask->args.ret) < 0);
					__task_error(ctask);
				} break;
			}

			ctask = NULL;
	}

	section_guard_exit(&guard);
}

/*============================================================================*
 * Interface exported                                                         *
 *============================================================================*/

/*============================================================================*
 * task_current()                                                             *
 *============================================================================*/

/**
 * @brief Get current task.
 *
 * @returns Current thread running. NULL if the dispatcher is not executing
 * a task.
 */
PUBLIC struct task * task_current(void)
{
	struct task * curr;
	struct section_guard guard;

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);

	/* Consult the current thread. */
	section_guard_entry(&guard);
		curr = ctask;
	section_guard_exit(&guard);

	return (curr);
}

/*============================================================================*
 * task_create()                                                              *
 *============================================================================*/

/**
 * @brief Create a task.
 *
 * Sets the struct parameters and initializes the mutex for the waiting
 * control. The @p arg pointer can be a NULL pointer.
 *
 * @param task Task pointer.
 * @param fn   Function pointer.
 * @param arg  Arguments pointer.
 *
 * @return Zero if successfully create the task, non-zero otherwise.
 */
PUBLIC int task_create(
	struct task * task,
	task_fn fn,
	struct task_args * args,
	int period
)
{
	/* Invalid task. */
	if (UNLIKELY(!task || !fn))
		return (-EINVAL);

	/* Init the resource. */
	task->resource = RESOURCE_INITIALIZER;
	resource_set_used(&task->resource);

	/* Indicate this is not dispatch yet. */
	task->reload = period > 0 ? period : 0;
	task->period = -1;

	/* Dependendy graph. */
	task->parents  = 0;
	task->children = RESOURCE_ARRANGEMENT_INITIALIZER;

	/* argument and return. */
	task->fn       = fn;
	task->args.ret = (-EINVAL);

	if (args)
	{
		task->args.arg0 = args->arg0;
		task->args.arg1 = args->arg1;
		task->args.arg2 = args->arg2;
		task->args.arg3 = args->arg3;
		task->args.arg4 = args->arg4;
		task->args.arg5 = args->arg5;
	}

	/**
	 * Configuration: Valid ID is setted when the task is dispatched,
	 * i.e., when it became tracked.
	 */
	task->id    = TASK_NULL_ID;
	task->state = TASK_STATE_NOT_STARTED;

	/* Control variables. */
	semaphore_init(&task->sem, 0);

	/* Success. */
	return (0);
}

/*============================================================================*
 * task_unlink()                                                              *
 *============================================================================*/

/**
 * @brief Destroy a task.
 *
 * Sets the struct parameters and initializes the mutex for the waiting
 * control. The @p arg pointer can be a NULL pointer.
 *
 * @param task Task pointer.
 *
 * @return Zero if successfully unlink the task, non-zero otherwise.
 */
PUBLIC int task_unlink(struct task * task)
{
	int ret = (-EINVAL);
	struct section_guard guard;

	/* Invalid task. */
	if (UNLIKELY(!task))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

		/* Invalid state. */
		if (UNLIKELY(task->state == TASK_STATE_RUNNING))
			goto error;

		/* Has parents or children. */
		if (UNLIKELY(task->parents || task->children.size))
			goto error;

		/* Are waiting on actives queue? */
		if (task->state == TASK_STATE_READY)
			ret = resource_pop(&tasks.actives, &task->resource);

		/* Are stopped on waiting or periodic queue? */
		else if (task->state == TASK_STATE_STOPPED)
		{
			ret = (task->reload == 0 || task->period < 0) ?
				resource_pop(&tasks.actives, &task->resource) : 
				periodic_task_remove(&tasks.periodics, task)  ;
		}

error:
	section_guard_exit(&guard);

	/* Destroy the task with success. */
	return ((ret >= 0) ? (0) : (-EINVAL));
}

/*============================================================================*
 * task_connect()                                                             *
 *============================================================================*/

/**
 * @brief Create a dependency on the @p child task to the @p parent task.
 *
 * @param parent Independent task.
 * @param child  Dependent task.
 *
 * @return Zero if successfully create the dependency, non-zero otherwise.
 */
PUBLIC int task_connect(struct task * parent, struct task * child)
{
	int ret = (-EINVAL);
	struct node_task * node;
	struct section_guard guard;

	/* Invalid tasks. */
	if (UNLIKELY(!parent || !child))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

		/* It is already initialized. */
		if (UNLIKELY(child->state != TASK_STATE_NOT_STARTED))
			goto error;

		/* Gets a free node. */
		node = NODE_PTR(resource_dequeue(&tasks.free_nodes));

		/* Configure the node. */
		node->task = child;
		resource_set_used(&node->resource);

		/* Connect parent to the child. */
		resource_push_back(&parent->children, &node->resource);

		/**
		 * Indicates a dependency on the child.
		 *
		 * @details The current version does not support a link to the parent task
		 * because the resource of the parent may be is in use by another
		 * queue/list.
		 */
		child->parents++;
		ret = 0;

error:
	section_guard_exit(&guard);

	/* Success. */
	return (ret);
}

/*============================================================================*
 * task_disconnect()                                                          *
 *============================================================================*/

/**
 * @brief Destroy a dependency on the @p child task to the @p parent task.
 *
 * @param parent Independent task.
 * @param child  Dependent task.
 *
 * @return Zero if successfully destroy the dependency, non-zero otherwise.
 */
PUBLIC int task_disconnect(struct task * parent, struct task * child)
{
	int ret;
	struct section_guard guard;

	/* Invalid tasks. */
	if (UNLIKELY(!parent || !child))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);
		ret = __task_disconnect(parent, child);
	section_guard_exit(&guard);

	return (ret);
}

/*============================================================================*
 * task_dispatch()                                                            *
 *============================================================================*/

/**
 * @brief Enqueue a task to the dispatcher thread operate.
 *
 * @param task Task pointer.
 *
 * @returns Zero if successfully dispatch a task, non-zero otherwise.
 */
PUBLIC int task_dispatch(struct task * task)
{
	struct section_guard guard;

	/* Invalid task or has dependencies. */
	if (UNLIKELY(!task || task->parents))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);

	/* Dispatch task. */
	section_guard_entry(&guard);
		__task_dispatch(task);
	section_guard_exit(&guard);

	/* Success. */
	return (0);
}

/*============================================================================*
 * task_wait()                                                                *
 *============================================================================*/

/**
 * @brief Wait for a task to complete.
 *
 * @param task Task pointer.
 *
 * @returns Zero if successfully wait for a task, non-zero otherwise.
 */
PUBLIC int task_wait(struct task * task)
{
	/* Invalid task. */
	if (UNLIKELY(!task))
		return (-EINVAL);

	/* Waits for all stages be completed. */
	semaphore_down(&task->sem);

	/* Success. */
	return (0);
}

/*============================================================================*
 * task_continue()                                                            *
 *============================================================================*/

/**
 * @brief Continue a blocked task.
 *
 * @details If the target is the current task, it will do not make any
 * operation over the task. If it is blocked, we move the task to the active
 * task queue.
 *
 * @param target Task pointer.
 *
 * @returns Zero if successfully wakeup the task. -EAGAIN if the thread is not
 * tracked (THREAD_STATE_NOT_STARTED). Or, -EINVAL for invalid task pointer.
 */
PUBLIC int task_continue(struct task * target)
{
	int ret = (-EAGAIN);
	struct section_guard guard;

	/* Invalid target. */
	if (UNLIKELY(!target))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

		/* Current task is already running. */
		if (ctask == target)
			ret = (0);

		/* Unblock a target task? */
		else if(resource_pop(&tasks.waiting, &target->resource) >= 0)
		{
			__task_dispatch(target);
			ret = (0);
		}

	section_guard_exit(&guard);

	return (ret);
}

/*============================================================================*
 * task_complete()                                                            *
 *============================================================================*/

/**
 * @brief Complete a task.
 *
 * @param target Task pointer.
 *
 * @returns Zero if successfully complete the task, non-zero otherwise.
 */
PUBLIC int task_complete(struct task * target)
{
	int ret = (-EAGAIN);
	struct section_guard guard;

	/* Invalid target. */
	if (UNLIKELY(!target))
		return (-EINVAL);

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

		/* Complete the current task? */
		if (ctask == target)
			ret = (0);

		/* Complete a blocked task? */
		else if (resource_pop(&tasks.waiting, &target->resource) >= 0)
			ret = (0);

		/* Complete a ready task? */
		else if (resource_pop(&tasks.actives, &target->resource) >= 0)
			ret = (0);

		/* Found? */
		if (LIKELY(ret == 0))
			__task_complete(target);

	section_guard_exit(&guard);

	return (ret);
}

/*============================================================================*
 * task_tick()                                                                *
 *============================================================================*/

/**
 * @brief Notify a system tick to the periodic queue.
 */
PUBLIC void task_tick(void)
{
	struct task * task;
	struct section_guard guard;

	section_guard_init(&guard, &tasks.lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

	/* Pop some periodic task. */
	while ((task = periodic_task_dequeue(&tasks.periodics)) != NULL)
	{
		__task_dispatch(task);

		/* There is no more task to pop. */
		if (periodic_task_next_period(&tasks.periodics) != 0)
			break;
	}

	section_guard_exit(&guard);
}

/*============================================================================*
 * task_init()                                                                *
 *============================================================================*/

/**
 * @brief Initializes the task system.
 */
PUBLIC void task_init(void)
{
	ctask    = NULL;
	shutdown = false;

	tasks.counter    = 0ULL;
	tasks.actives    = RESOURCE_ARRANGEMENT_INITIALIZER;
	tasks.waiting    = RESOURCE_ARRANGEMENT_INITIALIZER;
	tasks.periodics  = RESOURCE_ARRANGEMENT_INITIALIZER;
	tasks.free_nodes = RESOURCE_ARRANGEMENT_INITIALIZER;

	for (int i = 0; i < NODE_TASK_MAX; ++i)
	{
		tasks.nodes[i].resource = RESOURCE_INITIALIZER;
		resource_enqueue(&tasks.free_nodes, &tasks.nodes[i].resource);
	}

	spinlock_init(&tasks.lock);
	semaphore_init(&tasks.sem, 0);
}

#endif /* __NANVIX_USE_TASKS */
