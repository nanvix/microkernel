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

#include "active.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

/**
 * @name Auxiliar macros
 */
/**@{*/
#define ACTIVE_GET_NR_PORTS(_act)      (_act->portpool.nports)         /**< Extracts #ports. */
#define ACTIVE_GET_PORTID(_act, _port) (port - active->portpool.ports) /**< Extracts portid. */
/**@}*/

/**
 * @brief Any source identification
 */
#define ACTIVE_ANY_SRC (-1)

/**
 * @brief Any source identification
 */
#define ACTIVE_COMPUTE_LATENCY(_t1, _t2) ((_t2 - _t1) - clock_get_error())

/*============================================================================*
 * do_register_request()                                                      *
 *============================================================================*/

/**
 * @brief Request a operation for @p active on the physical port id @p port.
 *
 * @param active Active resource.
 * @param port   Port ID.
 *
 * @returns Upon successful register, zero is returned. Upon failure,
 * a negative error code is returned instead.
 */
PRIVATE int do_request_operation(struct active * active, struct port * port)
{
	int head;                        /* FIFO head. */
	struct requests_fifo * requests; /* FIFO.      */

	/* Sanity checks. */
	KASSERT((active != NULL) && (port != NULL));
	KASSERT(WITHIN(port, active->portpool.ports, (active->portpool.ports + active->portpool.nports)));

	requests = &active->requests;

	/* Is the FIFO full? */
	if (requests->nelements == requests->max_capacity)
		return (-EBUSY);

	/* Register request. */
	port_set_requested(port);
	head = requests->head;
	requests->fifo[head] = ACTIVE_GET_PORTID(active, port);

	/* Updates head. */
	requests->head = __mod_pw2((head + 1), requests->max_capacity);
	requests->nelements++;

	return (0);
}

/*============================================================================*
 * do_request_complete()                                                      *
 *============================================================================*/

/**
 * @brief Complete a operation on the physical port @p port.
 *
 * @param active Active resource.
 * @param port   Port ID.
 *
 * @returns Upon successful register, zero is returned. Upon failure,
 * a negative error code is returned instead.
 */
PRIVATE int do_request_complete(struct active * active, struct port * port)
{
	int tail;                        /* FIFO tail. */
	struct requests_fifo * requests; /* FIFO.      */

	/* Sanity checks. */
	KASSERT((active != NULL) && (port != NULL));
	KASSERT(WITHIN(port, active->portpool.ports, (active->portpool.ports + active->portpool.nports)));

	requests = &active->requests;
	tail     = requests->tail;

	/* Isn't it his turn? */
	if (requests->fifo[tail] != ACTIVE_GET_PORTID(active, port))
		return (-EINVAL);

	/* Complete the request. */
	requests->fifo[tail] = -1;
	requests->tail = __mod_pw2((tail + 1), requests->max_capacity);
	requests->nelements--;
	port_set_notrequested(port);

	return (0);
}


/*============================================================================*
 * do_request_clear()                                                         *
 *============================================================================*/

/**
 * @brief Clear a operation on the physical port @p port.
 *
 * @param active Active resource.
 * @param port   Port ID.
 *
 * @returns Upon successful register, zero is returned. Upon failure,
 * a negative error code is returned instead.
 */
PRIVATE int do_request_clear(struct active * active, struct port * port)
{
	int curr;                        /* Current FIFO position.        */
	int next;                        /* Next FIFO position.           */
	int portid;                      /* Port ID.                      */
	int cleared;                     /* Indicates if port is cleared. */
	struct requests_fifo * requests; /* FIFO.                         */

	/* Sanity checks. */
	KASSERT((active != NULL) && (port != NULL));
	KASSERT(WITHIN(port, active->portpool.ports, (active->portpool.ports + active->portpool.nports)));

	requests = &active->requests;

	if (!requests->nelements)
		return (0);

	portid   = ACTIVE_GET_PORTID(active, port);
	curr     = requests->tail;
	next     = __mod_pw2((curr + 1), requests->max_capacity);
	cleared  = 0;

	while (curr != requests->head)
	{
		cleared |= (portid == requests->fifo[curr]);

		if (cleared)
			requests->fifo[curr] = requests->fifo[next];

		curr = next;
		next = __mod_pw2((curr + 1), requests->max_capacity);
	}

	/* Clear the request. */
	if (cleared)
	{
		requests->head = __mod_pw2((requests->head - 1), requests->max_capacity);
		requests->nelements--;
		port_set_notrequested(port);
	}

	return (0);
}

/*============================================================================*
 * do_request_verify()                                                        *
 *============================================================================*/

/**
 * @brief Verifies if is the turns of the @p active.
 *
 * @param active Active resource.
 * @param port   Port ID.
 *
 * @returns Non-zero if is its turns, zero otherwise.
 */
PRIVATE int do_request_verify(struct active * active, struct port * port)
{
	/* Sanity checks. */
	KASSERT((active != NULL) && (port != NULL));
	KASSERT(WITHIN(port, active->portpool.ports, (active->portpool.ports + active->portpool.nports)));

	if (!active->requests.nelements)
		return (0);

	return (active->requests.fifo[active->requests.tail] == ACTIVE_GET_PORTID(active, port));
}

/*============================================================================*
 * Physical active functions                                                 *
 *============================================================================*/

/*============================================================================*
 * active_search()                                                            *
 *============================================================================*/

/**
 * @brief Validate a call.
 *
 * @param pool Active resource pool.
 * @param fd   Compose address by active and port id.
 *
 * @returns Non-zero if its valid, zero otherwize.
 */
PRIVATE int active_valid_call(const struct active_pool * pool, const int fd)
{
	int actid;  /* Active ID. */
	int portid; /* Port ID.   */

	/* Valid pool pointer. */
	if (pool == NULL)
		return (0);

	actid  = pool->actives->fn->get_actid(fd);

	/* Active ID. */
	if (!WITHIN(actid, 0, pool->nactives))
		return (0);

	portid = pool->actives->fn->get_portid(fd);

	/* Port ID. */
	return (WITHIN(portid, 0, ACTIVE_GET_NR_PORTS((&pool->actives[actid]))));
}

/*============================================================================*
 * active_search()                                                            *
 *============================================================================*/

/**
 * @name Helper Macros for active_search()
 */
/**@{*/

/**
 * @brief Asserts an input active.
 */
#define ACTIVE_SEARCH_IS_INPUT(_act, _type) \
	((_type == ACTIVE_TYPE_INPUT) && !resource_is_readable(&_act->resource))

/**
 * @brief Asserts an output active.
 */
#define ACTIVE_SEARCH_IS_OUTPUT(_act, _type) \
	 ((_type == ACTIVE_TYPE_OUTPUT) && !resource_is_writable(&_act->resource))
/**@}*/

/**
 * @brief Searches for an active HW active.
 *
 * Searches for a active in the table of activees.
 *
 * @param pool   Active resource pool.
 * @param local  Local node ID.
 * @param remote Remote node ID (It can be -1).
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the ID of the active found is
 * returned. Upon failure, a negative error code is returned instead.
 */
PRIVATE int active_search(
	const struct active_pool * pool,
	int local,
	int remote,
	int type
)
{
	struct active * active; /* Active pointer. */

	KASSERT(pool != NULL);

	/* Search for a free synchronization point. */
	for (int i = 0; i < pool->nactives; i++)
	{
		active = &pool->actives[i];

		spinlock_lock(&active->lock);

			if (!resource_is_used(&active->resource))
			{
				spinlock_unlock(&active->lock);
				continue;
			}

			if (ACTIVE_SEARCH_IS_INPUT(active, type))
			{
				spinlock_unlock(&active->lock);
				continue;
			}

			else if (ACTIVE_SEARCH_IS_OUTPUT(active, type))
			{
				spinlock_unlock(&active->lock);
				continue;
			}

			/* Not the node we are looking for. */
			if ((active->local != local) || (remote != -1 && active->remote != remote))
			{
				spinlock_unlock(&active->lock);
				continue;
			}

		spinlock_unlock(&active->lock);

		return (i);
	}

	return (-EBUSY);
}

/*============================================================================*
 * active_alloc()                                                             *
 *============================================================================*/

/**
 * @brief Allocate a active resource.
 *
 * @param pool   Active resource pool.
 * @param local  Local node ID.
 * @param remote Remote node ID (It can be -1).
 * @param portid Port ID.
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the ID of the active is
 * returned. Upon failure, a negative error code is returned instead.
 */
PUBLIC int active_alloc(
	const struct active_pool * pool,
	int local,
	int remote,
	int portid,
	int type
)
{
	int ret;                     /* Return value.        */
	int actid;                   /* Hardware active ID. */
	struct port * port;          /* Port pointer.        */
	struct active * active;      /* Active pointer.      */
	struct port_pool * portpool; /* Port pool pointer.   */

	KASSERT(pool != NULL);
	ret = (-EINVAL);

	/* Search target hardware active. */
	if ((actid = active_search(pool, local, remote, type)) < 0)
		return (actid);

	active = &pool->actives[actid];
	portpool = &active->portpool;

	spinlock_lock(&active->lock);

		/* Choose a port of output active. */
		if (type == ACTIVE_TYPE_OUTPUT)
		{
			if ((portid = portpool_choose_port(portpool)) < 0)
			{
				ret = portid;
				goto error;
			}
		}

		/* Choose a port of output active. */
		else if (!WITHIN(portid, 0, ACTIVE_GET_NR_PORTS(active)))
			goto error;

		port = &portpool->ports[portid];

		/* Is the *port free? */
		if (!resource_is_used(&port->resource))
		{
			/* Initialize active. */
			port->flags = 0;
			resource_set_used(&port->resource);

			active->refcount++;
			portpool->used_ports++;

			ret = ACTIVE_LADDRESS_COMPOSE(actid, portid, ACTIVE_GET_NR_PORTS(active));
		}
		else
			ret = (-EBUSY);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * active_release()                                                           *
 *============================================================================*/

/**
 * @brief Release a physical active port.
 *
 * @param pool   Active resource pool.
 * @param id     Compose address by active and port ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int active_release(const struct active_pool * pool, int id)
{
	int ret;                     /* Return value.        */
	int dest;                    /* Local address.       */
	struct port * port;          /* Port pointer.        */
	struct active * active;      /* Active pointer.      */
	struct port_pool * portpool; /* Port pool pointer.   */

	KASSERT(active_valid_call(pool, id));

	active   = &pool->actives[pool->actives->fn->get_actid(id)];
	portpool = &active->portpool;
	port     = &portpool->ports[active->fn->get_portid(id)];

	spinlock_lock(&active->lock);

		ret = (-EBADF);

		/* Is the port used? */
		if (!resource_is_used(&active->resource))
			goto error;

		ret = (-EBUSY);

		if (resource_is_readable(&active->resource))
		{
			dest = ACTIVE_LADDRESS_COMPOSE(
				active->local,
				active->fn->get_portid(id),
				ACTIVE_GET_NR_PORTS(active)
			);

			/* Check if exist pending messages for this port in principal mbufferpool. */
			if (mbuffer_search(active->mbufferpool, dest, ACTIVE_ANY_SRC) >= 0)
				goto error;
		}

		ret = (-EINVAL);

		/* Is the port used? */
		if (resource_is_used(&port->resource))
		{
			/* Releases active. */
			resource_set_unused(&port->resource);
			active->refcount--;
			portpool->used_ports--;

			ret = (0);
		}

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

#if __NANVIX_USE_TASKS

/*============================================================================*
 * active_set_task()                                                          *
 *============================================================================*/

/**
 * @brief Connect the task to the child of the current running task.
 *
 * @param pool Active resource pool.
 * @param id   Compose address by active and port ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PRIVATE void active_set_task(const struct active_pool * pool, const struct active * active)
{
	struct task * atask;
	struct task * ctask;
	struct task * child;

	atask = &pool->tasks[(active - pool->actives)];
	KASSERT(atask->state == TASK_STATE_NOT_STARTED || atask->state == TASK_STATE_COMPLETED);
	KASSERT(atask->children.size == 0);

	KASSERT((ctask = task_current()) != NULL);
	KASSERT(ctask->state == TASK_STATE_RUNNING);
	KASSERT(ctask->children.size == 1);
	KASSERT(ctask->children.head != NULL);

	child = ((struct node_task *) ctask->children.head)->task;
	KASSERT(child->parents == 1);

	/* Connect active task to the child of current task, creating
	 * a dependency to it:
	 *
	 *        ctask (awrite/aread) ----- child (wait)
	 *                                        /
	 *        atask (op. completed) ---------´
	 *
	 * As the handler is going to create dispatching the atask, the
	 * task of waiting will only happen when the operation is completed.
	 */
	KASSERT(task_connect(atask, child) == 0);
}

/*============================================================================*
 * active_unset_task()                                                        *
 *============================================================================*/

/**
 * @brief Disconnect the task to the child of the current running task.
 *
 * @param pool Active resource pool.
 * @param id   Compose address by active and port ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PRIVATE void active_unset_task(const struct active_pool * pool, const struct active * active)
{
	struct task * atask;
	struct task * ctask;
	struct task * child;

	atask = &pool->tasks[(active - pool->actives)];
	//KASSERT(atask->state == TASK_STATE_NOT_STARTED || atask->state == TASK_STATE_COMPLETED);
	KASSERT(atask->children.size == 1);

	ctask = task_current();
	KASSERT(ctask->state == TASK_STATE_RUNNING);
	KASSERT(ctask->children.size == 1);
	KASSERT(ctask->children.head != NULL);

	child = ((struct node_task *) ctask->children.head)->task;
	//KASSERT(child->parents == 2);

	/* Disconnect active task to the child of current task, removing
	 * the dependency:
	 *
	 *        ctask (awrite/aread) ----- child (wait)
	 *
	 *        atask (op. completed) ---X
	 *
	 * This is needed because a awrite/aread can failed for N reasons.
	 */
	if (task_disconnect(atask, child) != 0)
	{
		kprintf("error");
		KASSERT(false);
	}
}

#endif

/*============================================================================*
 * active_handler_wait()                                                      *
 *============================================================================*/

/**
 * @brief Waits a communication finishs on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PUBLIC void active_handler_wait(const struct active_pool * pool, int hwfd, char * rule)
{
	struct active * actives = pool->actives;

	/* Search for the active portal. */
	for (int i = 0; i < pool->nactives; ++i)
	{
		if (!resource_is_used(&actives[i].resource))
			continue;

		/* Found. */
		if (actives[i].hwfd == hwfd)
		{
			/* It myst be set to busy before the wait operation. */
			KASSERT(resource_is_busy(&actives[i].resource));

			semaphore_down(&actives[i].waiting);

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][%s] Tried to wait for an invalid active.", rule);
}

/*============================================================================*
 * mailbox_wait_active()                                                      *
 *============================================================================*/

/**
 * @brief Complete a communication on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PUBLIC void active_handler_wakeup(const struct active_pool * pool, int hwfd, char * rule)
{
	struct active * actives = pool->actives;

	/* Search for the active portal. */
	for (int i = 0; i < pool->nactives; ++i)
	{
		if (!resource_is_used(&actives[i].resource))
			continue;

		/* Found. */
		if (actives[i].hwfd == hwfd)
		{
			semaphore_up(&actives[i].waiting);

#if __NANVIX_USE_TASKS
			task_dispatch(&pool->tasks[i]);
#endif

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][%s] Tried to wake up for an invalid active.", rule);
}

/*============================================================================*
 * active_aread()                                                             *
 *============================================================================*/

/**
 * @brief Async reads from an active.
 *
 * @param pool   Active resource pool.
 * @param id     Compose address by active and port ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t active_aread(
	const struct active_pool * pool,
	int id,
	const struct active_config * config,
	struct pstats * stats
)
{
	ssize_t ret;                      /* Return value.                  */
	int remote;                       /* Remote node ID.                */
	int mbufferid;                    /* Mbuffer ID.                    */
	int comm_is_local;                /* Checks value for local comm.   */
	uint64_t t1;                      /* Clock value before aread call. */
	uint64_t t2;                      /* Clock value after aread call.  */
	struct port * port;               /* Port pointer.                  */
	struct mbuffer * buf;             /* Mbuffer pointer.               */
	struct active * active;           /* Active pointer.                */
	struct mbuffer_pool * bufferpool; /* Mbuffer pool pointer.          */

	KASSERT(active_valid_call(pool, id));

	active = &pool->actives[pool->actives->fn->get_actid(id)];
	port   = &active->portpool.ports[pool->actives->fn->get_portid(id)];
	remote = active->fn->get_actid(config->remote_addr);

	/* Checks if it is a local receive. */
	comm_is_local = node_is_local(remote);

	spinlock_lock(&active->lock);

		ret = (-EBADF);

		/* Bad active port. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Is the communication local? */
		if (comm_is_local)
		{
			/* Search only in a subset of the mbufferpool. */
			mbufferid = mbuffer_search(active->mbufferpool_aux, config->local_addr, config->remote_addr);
			bufferpool = active->mbufferpool_aux;
		}
		else
		{
			/* Search in the whole mbufferpool. */
			mbufferid = mbuffer_search(active->mbufferpool, config->local_addr, config->remote_addr);
			bufferpool = active->mbufferpool;
		}

		/* Is there a pending message for this active? */
		if (mbufferid >= 0)
		{
			buf = mbuffer_get(bufferpool, mbufferid);

			t1 = clock_read();
				kmemcpy((void *) config->buffer, (void *) &buf->message.data, config->size);
			t2 = clock_read();

			/* Update performance statistics. */
			stats->latency += buf->latency + ACTIVE_COMPUTE_LATENCY(t1, t2);
			stats->volume  += config->size;

			/* Releases sender. */
			if ((buf->actid >= 0) && (buf->portid >= 0))
			{
				struct active * _active = &pool->actives[buf->actid];
				struct port * _port     = &_active->portpool.ports[buf->portid];

				KASSERT(do_request_clear(_active, _port) == 0);
			}

			KASSERT(mbuffer_release(bufferpool, mbufferid, MBUFFER_DISCARD_MESSAGE) == 0);

			spinlock_unlock(&active->lock);

			return (ACTIVE_COMM_RECEIVED);
		}

		ret = (-ENOMSG);

		/* Is it a local communication? Then retry read for search on local mbufferpool. */
		if (comm_is_local)
			goto error;

		ret = (-EBUSY);

		/* Busy active. */
		if (resource_is_busy(&active->resource))
			goto error;

		/* Allow communication. */
		if ((ret = active->fn->do_allow(active, remote)) < 0)
			goto error;

		active_set_allowed(active);
		active->remote = remote;

		/* Allocates a data buffer to receive data. */
		if ((mbufferid = mbuffer_alloc(active->mbufferpool)) < 0)
		{
			ret = (mbufferid);
			goto error;
		}

		port->mbufferid   = mbufferid;
		port->mbufferpool = active->mbufferpool;
		buf = mbuffer_get(port->mbufferpool, mbufferid);

#if __NANVIX_USE_TASKS
		/* Connect active task to current task. */
		active_set_task(pool, active);
#endif

		t1 = clock_read();

			/* Setup asynchronous read. */
			if ((ret = active->fn->do_aread(active->hwfd, (void *) &buf->message, active->size)) < 0)
				goto discard_message;

		t2 = clock_read();

		active->remote = -1;
		active_set_notallowed(active);

		/* Update performance statistics. */
		buf->latency = ACTIVE_COMPUTE_LATENCY(t1, t2);

		/* Sets the active as busy. */
		resource_set_busy(&active->resource);

	spinlock_unlock(&active->lock);

	return (ACTIVE_COMM_SUCCESS);

discard_message:
		KASSERT(mbuffer_release(port->mbufferpool, mbufferid, MBUFFER_DISCARD_MESSAGE) == 0);
		port->mbufferid   = -1;
		port->mbufferpool = NULL;

#if __NANVIX_USE_TASKS
		/* Disconnect active task. */
		active_unset_task(pool, active);
#endif

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * active_awrite()                                                            *
 *============================================================================*/

/**
 * @brief Async writes to an active.
 *
 * @param pool   Active resource pool.
 * @param id     Compose address by active and port ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, an status is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t active_awrite(
	const struct active_pool * pool,
	int id,
	const struct active_config * config,
	struct pstats * stats
)
{
	ssize_t ret;                      /* Return value.                   */
	int forward;                      /* Check value for local comm.     */
	int mbufferid;                    /* Message buffer used to write.   */
	uint64_t t1;                      /* Clock value before awrite call. */
	uint64_t t2;                      /* Clock value after awrite call.  */
	struct port * port;               /* Port pointer.                   */
	struct mbuffer * buf;             /* Mbuffer pointer.                */
	struct active * active;           /* Active pointer.                 */
	struct mbuffer_pool * bufferpool; /* Mbuffer pool pointer.           */

	KASSERT(active_valid_call(pool, id));

	active = &pool->actives[pool->actives->fn->get_actid(id)];
	port   = &active->portpool.ports[pool->actives->fn->get_portid(id)];

	/* Checks if the write is a forward or a send through NoC. */
	forward = node_is_local(active->remote);

	spinlock_lock(&active->lock);

		ret = (-EINVAL);

		/* Bad active port. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Checks if there is already a mbuffer allocated. */
		if ((mbufferid = port->mbufferid) < 0)
		{
			/* Allocates a message buffer to send the message. */
			if (forward)
			{
				ret = (-EBUSY);

				/* Checks if the current port already requested an operation. */
				if (port_is_requested(port))
					goto error;

				/* Request an operation. */
				if (do_request_operation(active, port) < 0)
					goto error;

				mbufferid  = mbuffer_alloc(active->mbufferpool_aux);
				bufferpool = active->mbufferpool_aux;
			}
			else
			{
				mbufferid  = mbuffer_alloc(active->mbufferpool);
				bufferpool = active->mbufferpool;
			}

			/* Successful allocated a message buffer? */
			if (mbufferid < 0)
			{
				ret = (mbufferid);
				goto error;
			}

			buf = mbuffer_get(bufferpool, mbufferid);

			active->fn->do_header_config(buf, config);

			t1 = clock_read();
				kmemcpy((void *) &buf->message.data, (void *) config->buffer, config->size);
			t2 = clock_read();

			buf->latency = ACTIVE_COMPUTE_LATENCY(t1, t2);

			/* Checks if the destination is the local node. */
			if (forward)
			{
				buf->actid  = pool->actives->fn->get_actid(id);
				buf->portid = pool->actives->fn->get_portid(id);

				/* Forwards the message to the mbuffers table. */
				mbuffer_release(bufferpool, mbufferid, MBUFFER_KEEP_MESSAGE);

				/* Update performance statistics. */
				stats->latency += buf->latency;
				stats->volume  += config->size;

				spinlock_unlock(&active->lock);

				return (ACTIVE_COMM_RECEIVED);
			}
			else
			{
				port->mbufferpool = bufferpool;
				port->mbufferid   = mbufferid;
			}
		}
		else
			buf = mbuffer_get(port->mbufferpool, mbufferid);

		ret = (-EBUSY);

		/* Checks if the current port already requested an operation. */
		if (!port_is_requested(port))
		{
			/* Request an operation. */
			if (do_request_operation(active, port) < 0)
				goto error;
		}

		/* Check if it is the current comm's turn. */
		if (!do_request_verify(active, port))
			goto error;

		/* Bad active. */
		if (resource_is_busy(&active->resource))
			goto error;

#if __NANVIX_USE_TASKS
		/* Connect active task to current task. */
		active_set_task(pool, active);
#endif

		t1 = clock_read();

			/* Setup asynchronous write. */
			if ((ret = active->fn->do_awrite(active->hwfd, (void *) &buf->message, active->size)) < 0)
				goto unset_task;

		t2 = clock_read();

		/* Update performance statistics. */
		buf->latency = ACTIVE_COMPUTE_LATENCY(t1, t2);

		/* Sets the active as busy. */
		resource_set_busy(&active->resource);

		ret = (ACTIVE_COMM_SUCCESS);

unset_task:
#if __NANVIX_USE_TASKS
		/* Connect active task to current task. */
		if (ret < 0)
			active_unset_task(pool, active);
#endif

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * active_wait()                                                              *
 *============================================================================*/

/**
 * @brief Waits an operation on an active.
 *
 * @param pool   Active resource pool.
 * @param id     Compose address by active and port ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int active_wait(
	const struct active_pool * pool,
	int id,
	const struct active_config * config,
	struct pstats * stats
)
{
	int ret;                /* Return value.                   */
	int dest;               /* Msg destination address.        */
	int mbufferid;          /* Allocated mbufferid.            */
	int keep_rule;          /* Discard rule.                   */
	uint64_t t1;            /* Clock value before awrite call. */
	uint64_t t2;            /* Clock value after awrite call.  */
	struct port * port;     /* Port pointer.                   */
	struct mbuffer * buf;   /* Mbuffer pointer.                */
	struct active * active; /* Active pointer.                 */

	KASSERT(active_valid_call(pool, id));

	active = &pool->actives[pool->actives->fn->get_actid(id)];
	port   = &active->portpool.ports[active->fn->get_portid(id)];

	spinlock_lock(&active->lock);

		ret = (-EBADF);

		/* Bad active. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Bad active. */
		if (!resource_is_busy(&active->resource))
			goto error;

		/* Invalid mbufferid. */
		if ((mbufferid = port->mbufferid) < 0)
			goto error;

		/* Not requested write. */
		if ((resource_is_writable(&active->resource)) && !(do_request_verify(active, port)))
			goto error;

	spinlock_unlock(&active->lock);

	t1 = clock_read();

		/* Wait for asynchronous write to finish. */
		ret = active->fn->do_wait(active->hwfd);

	t2 = clock_read();

	spinlock_lock(&active->lock);

		keep_rule = MBUFFER_DISCARD_MESSAGE;

		/* Was there no error? */
		if (ret == 0)
		{
			ret = (ACTIVE_COMM_SUCCESS);
			buf = mbuffer_get(port->mbufferpool, mbufferid);

			buf->latency += ACTIVE_COMPUTE_LATENCY(t1, t2);

			/* Read communication has extra operations. */
			if (resource_is_readable(&active->resource))
			{
				dest  = buf->message.header.dest;

				/* Checks if the message is addressed for the requesting port. */
				/* Consumes the message. */
				if (active->fn->do_header_check(buf, config))
				{
					t1 = clock_read();
						kmemcpy((void *) config->buffer, (void *) &buf->message.data, config->size);
					t2 = clock_read();

					buf->latency += ACTIVE_COMPUTE_LATENCY(t1, t2);
				}

				/* Ignore the message. */
				else
				{
					/* Keep message? true (MBUFFER_KEEP_MESSAGE) : false (MBUFFER_DISCARD_MESSAGE) */
					keep_rule = resource_is_used(
						&active->portpool.ports[active->fn->get_portid(dest)].resource
					);

					/* Returns sinalizing that a message was read, but not for local port. */
					ret = (ACTIVE_COMM_AGAIN);
				}
			}
			else if (resource_is_writable(&active->resource))
				KASSERT(do_request_complete(active, port) == 0);

			/* Update performance statistics. */
			if (ret == ACTIVE_COMM_SUCCESS)
			{
				stats->volume  += (config->size);
				stats->latency += buf->latency;
			}
		}

		/* Releases mbuffer. */
		mbuffer_release(port->mbufferpool, mbufferid, keep_rule);
		port->mbufferid   = -1;
		port->mbufferpool = NULL;

		resource_set_notbusy(&active->resource);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * _active_alloc()                                                            *
 *============================================================================*/

/**
 * @brief Allocate a active resource.
 *
 * @param pool Active resource pool.
 *
 * @returns Upon successful completion, active pointer is returned. Upon
 * failure, NULL pointer is returned.
 */
PRIVATE struct active * _active_alloc(const struct active_pool * pool)
{
	struct active * active;

	/* Search for a free synchronization point. */
	for (int i = 0; i < pool->nactives; i++)
	{
		active = &pool->actives[i];

		/* Found. */
		if (!resource_is_used(&active->resource))
		{
			active->resource = RESOURCE_INITIALIZER;
			resource_set_used(&active->resource);

			return (active);
		}
	}

	return (NULL);
}

/*============================================================================*
 * _active_free()                                                             *
 *============================================================================*/

/**
 * @brief Release a active resource.
 *
 * @param pool Active resource pool.
 * @param id   Active ID.
 */
PRIVATE void _active_free(const struct active_pool * pool, int id)
{
	resource_set_unused(&pool->actives[id].resource);
}

#if __NANVIX_USE_TASKS

/*============================================================================*
 * active_task_dummy()                                                        *
 *============================================================================*/

PRIVATE int active_task_dummy(struct task_args * args)
{
	UNUSED(args);
	return (TASK_RET_SUCCESS);
}

#endif

/*============================================================================*
 * _active_create()                                                           *
 *============================================================================*/

/**
 * @brief Creates a hardware active.
 *
 * @param pool  Active resource pool.
 * @param local Logic ID of the target local node.
 *
 * @returns Upon successful completion, the ID of the newly created
 * hardware active is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int _active_create(const struct active_pool * pool, int local)
{
	int hwfd;  /* File descriptor. */
	int actid;    /* active ID.      */
	struct active * active;

	KASSERT(pool != NULL);

	/* Search target hardware active. */
	if ((actid = active_search(pool, local, -1, ACTIVE_TYPE_INPUT)) >= 0)
		return (actid);

	/* Allocate resource. */
	if ((active = _active_alloc(pool)) == NULL)
		return (-EAGAIN);

	/* Create underlying input hardware active. */
	if ((hwfd = active->fn->do_create(local)) < 0)
	{
		_active_free(pool, (active - pool->actives));
		return (hwfd);
	}

	actid = (active - pool->actives);

	/* Initialize hardware active. */
	active->flags    = 0;
	active->hwfd     = hwfd;
	active->local    = local;
	active->remote   = -1;
	active->refcount = 0;
	resource_set_rdonly(&active->resource);
	resource_set_notbusy(&active->resource);

#if __NANVIX_USE_TASKS

	pool->tasks[actid].args.arg0 = actid;
	if (task_create(&pool->tasks[actid], active_task_dummy, &pool->tasks[actid].args, 0) != 0)
		return (-EINVAL);

#endif

	return (actid);
}

/*============================================================================*
 * _active_open()                                                             *
 *============================================================================*/

/**
 * @brief Opens a hardware active.
 *
 * @param pool   Active resource pool.
 * @param local  Logic ID of the target local node.
 * @param remote Logic ID of the target remote node.
 *
 * @returns Upon successful completion, the ID of the newly opened
 * hardware active is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int _active_open(const struct active_pool * pool, int local, int remote)
{
	int hwfd;               /* Hardware ID.    */
	int actid;              /* Active ID.      */
	struct active * active; /* Active Pointer. */

	KASSERT(pool != NULL);

	/* Search target hardware active. */
	if ((actid = active_search(pool, local, remote, ACTIVE_TYPE_OUTPUT)) >= 0)
		return (actid);

	/* Allocate resource. */
	if ((active = _active_alloc(pool)) == NULL)
		return (-EAGAIN);

	hwfd = -EINVAL;

	if (node_is_local(local) && !node_is_local(remote))
	{
		/* Open underlying output hardware active. */
		if ((hwfd = active->fn->do_open(local, remote)) < 0)
		{
			_active_free(pool, (active - pool->actives));
			return (hwfd);
		}
	}
	else if (!node_is_local(local))
		return (-EINVAL);

	actid = (active - pool->actives);

	/* Initialize hardware active. */
	active->flags    = 0;
	active->hwfd     = hwfd;
	active->refcount = 0;
	active->local    = local;
	active->remote   = remote;
	resource_set_wronly(&active->resource);
	resource_set_notbusy(&active->resource);

#if __NANVIX_USE_TASKS

	pool->tasks[actid].args.arg0 = (word_t) actid;
	if (task_create(&pool->tasks[actid], active_task_dummy, &pool->tasks[actid].args, 0) != 0)
		return (-EINVAL);

#endif

	return (actid);
}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */
