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
#define __NEED_RESOURCE

#include <nanvix/hal.h>
#include <nanvix/hlib.h>
#include <posix/errno.h>
#include <posix/stdarg.h>

#include "communicator.h"
#include "mbuffer.h"
#include "active.h"

/*============================================================================*
 * active_choose_port()                                                       *
 *============================================================================*/

/**
 * @brief Searches for a free port on @p fd.
 *
 * @param fd ID of the target HW mailbox.
 *
 * @returns Upon successful completion, the index of the available port is
 * returned. A negative number is returned instead.
 */
PRIVATE int active_choose_port(const struct active * active)
{
	int ret; /* Return value. */
	const struct port_pool * pool = &active->portpool;

	ret = (-EINVAL);

	/* Checks if can exist an available port. */
	if (active->refcount < pool->nports)
	{
		/* Searches for a free port on the target mailbox. */
		for (int i = 0; i < pool->nports; ++i)
		{
			if (resource_is_used(&pool->ports[i].resource))
				continue;

			ret = i;
			break;
		}
	}

	return (ret);
}

/*============================================================================*
 * Physical mailbox functions                                                 *
 *============================================================================*/

/*============================================================================*
 * active_search()                                                            *
 *============================================================================*/

/**
 * @name Helper Macros for active_search()
 */
/**@{*/

/**
 * @brief Asserts an input mailbox.
 */
#define ACTIVE_SEARCH_IS_INPUT(_act, _type) \
	((_type == COMM_TYPE_INPUT) && !resource_is_readable(&_act->resource))

/**
 * @brief Asserts an output mailbox.
 */
#define ACTIVE_SEARCH_IS_OUTPUT(_act, _type) \
	 ((_type == COMM_TYPE_OUTPUT) && !resource_is_writable(&_act->resource))
/**@}*/

/**
 * @brief Searches for an active HW mailbox.
 *
 * Searches for a mailbox in the table of active mailboxes.
 *
 * @param nodenum Logic ID of the requesting node.
 * @param type    Type of the searched resource.
 *
 * @returns Upon successful completion, the ID of the mailbox found is
 * returned. Upon failure, a negative error code is returned instead.
 */
PRIVATE int active_search(
	const struct active_pool * pool,
	int local,
	int remote,
	int type
)
{
	struct active * active;

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
			if ((active->local != local) || (active->remote != remote))
			{
				spinlock_unlock(&active->lock);
				continue;
			}

		spinlock_unlock(&active->lock);

		return (i);
	}

	return (-EINVAL);
}

/*============================================================================*
 * do_mailbox_alloc()                                                         *
 *============================================================================*/

/**
 * @brief Creates a physical mailbox.
 *
 * @param nodenum Logic node ID.
 * @param port  Target port in @p nodenum node.
 *
 * @returns Upon successful completion, the ID of the active mailbox is
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
	int ret;                /* Return value.        */
	int actid;              /* Hardware mailbox ID. */
	struct port   * port;
	struct active * active;

	ret = (-EINVAL);

	/* Search target hardware mailbox. */
	if ((actid = active_search(pool, local, remote, type)) < 0)
		return (actid);

	active = &pool->actives[actid];

	spinlock_lock(&active->lock);

		/* Choose a port of output mailbox. */
		if (type == COMM_TYPE_OUTPUT)
		{
			if ((portid = active_choose_port(active)) < 0)
			{
				ret = portid;
				goto error;
			}
		}

		/* Choose a port of output mailbox. */
		else if (!WITHIN(portid, 0, ACTIVE_GET_NR_PORTS(active)))
			goto error;

		port = &active->portpool.ports[portid];

		/* Is the *port free? */
		if (!resource_is_used(&port->resource))
		{
			/* Initialize mailbox. */
			resource_set_used(&port->resource);
			active->refcount++;

			ret = ACTIVE_LADDRESS_COMPOSE(actid, portid, ACTIVE_GET_NR_PORTS(active));
		}
		else
			ret = (-EBUSY);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/**
 * @brief Unlink a physical mailbox.
 *
 * @param fd Physical mailbox ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int active_release(const struct active_pool * pool, int id)
{
	int ret; /* Return value. */
	int dest;
	struct port * port;
	struct active * active;

	active = &pool->actives[ACTIVE_GET_LADDRESS_FD(pool->actives, id)];
	port   = &active->portpool.ports[ACTIVE_GET_LADDRESS_PORT(active, id)];

	spinlock_lock(&active->lock);

		ret = (-EBUSY);

		if (resource_is_readable(&active->resource))
		{
			dest = ACTIVE_LADDRESS_COMPOSE(
				active->local,
				ACTIVE_GET_LADDRESS_PORT(active, id),
				ACTIVE_GET_NR_PORTS(active)
			);

			/* Check if exist pending messages for this port. */
			if (mbuffer_search(active->mbufferpool, dest, ACTIVE_ANY_SRC) >= 0)
				goto error;
		}

		ret = (-EINVAL);

		/* Is the port used? */
		if (resource_is_used(&port->resource))
		{
			/* Releases mailbox. */
			resource_set_unused(&port->resource);
			active->refcount--;

			ret = 0;
		}

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * do_mailbox_aread()                                                         *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC ssize_t active_aread(
	const struct active_pool * pool,
	int id,
	const struct comm_config * config,
	struct pstats * stats
)
{
	ssize_t ret;             /* Return value.                  */
	int remote;
	int mbufferid;       /* New alocated buffer.           */
	int local_addr;      /* Vmailbox hardware address.     */
	uint64_t t1;         /* Clock value before aread call. */
	uint64_t t2;         /* Clock value after aread call.  */
	struct port * port;
	struct mbuffer * buf;
	struct active * active;

	active = &pool->actives[ACTIVE_GET_LADDRESS_FD(pool->actives, id)];
	port   = &active->portpool.ports[ACTIVE_GET_LADDRESS_PORT(active, id)];

	local_addr = ACTIVE_LADDRESS_COMPOSE(
		active->local,
		ACTIVE_GET_LADDRESS_PORT(active, id),
		ACTIVE_GET_NR_PORTS(active)
	);
	remote = ACTIVE_GET_LADDRESS_FD(active, config->remote);

	spinlock_lock(&active->lock);

		ret = (-EBADF);

		/* Bad mailbox port. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Is there a pending message for this vmailbox? */
		if ((mbufferid = mbuffer_search(active->mbufferpool, local_addr, config->remote)) >= 0)
		{
			buf = mbuffer_get(active->mbufferpool, mbufferid);

			t1 = clock_read();
				active->do_copy(buf, config, ACTIVE_COPY_FROM_MBUFFER);
			t2 = clock_read();

			/* Update performance statistics. */
			stats->latency += (t2 - t1);
			stats->volume  += (config->size);

			KASSERT(mbuffer_release(active->mbufferpool, mbufferid, MBUFFER_DISCARD_MESSAGE) == 0);

			spinlock_unlock(&active->lock);

			return (COMM_STATUS_RECEIVED);
		}

		ret = (-ENOMSG);

		/* Is it a local communication? */
		if (active->local == remote)
			goto error;

		ret = (-EBUSY);

		/* Busy mailbox. */
		if (resource_is_busy(&active->resource))
			goto error;

		/* Allow communication. */
		if ((ret = active->do_allow(active, remote)) < 0)
			goto error;

		/* Allocates a data buffer to receive data. */
		if ((mbufferid = mbuffer_alloc(active->mbufferpool)) < 0)
		{
			ret = mbufferid;
			goto error;
		}

		port->mbufferid = mbufferid;
		buf = mbuffer_get(active->mbufferpool, mbufferid);

		t1 = clock_read();

			/* Setup asynchronous read. */
			if ((ret = active->do_aread(active->hwfd, (void *) &buf->message, active->size)) < 0)
				goto discard_message;

		t2 = clock_read();

		active->remote = -1;
		active_set_notallowed(active);

		/* Update performance statistics. */
		stats->latency += (t2 - t1);

		/* Sets the mailbox as busy. */
		resource_set_busy(&active->resource);

	spinlock_unlock(&active->lock);

	return (COMM_STATUS_SUCCESS);

discard_message:
		port->mbufferid = -1;
		KASSERT(mbuffer_release(active->mbufferpool, mbufferid, MBUFFER_DISCARD_MESSAGE) == 0);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * do_mailbox_awrite()                                                        *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC ssize_t active_awrite(
	const struct active_pool * pool,
	int id,
	const struct comm_config * config,
	struct pstats * stats
)
{
	ssize_t ret;       /* Return value.                   */
	int mbufferid; /* Message buffer used to write.   */
	uint64_t t1;   /* Clock value before awrite call. */
	uint64_t t2;   /* Clock value after awrite call.  */
	struct port * port;
	struct mbuffer * buf;
	struct active * active;

	active = &pool->actives[ACTIVE_GET_LADDRESS_FD(pool->actives, id)];
	port   = &active->portpool.ports[ACTIVE_GET_LADDRESS_PORT(active, id)];

	spinlock_lock(&active->lock);

		ret = (-EINVAL);

		/* Bad mailbox port. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Checks if there is already a mbuffer allocated. */
		if ((mbufferid = port->mbufferid) < 0)
		{
			/* Allocates a message buffer to send the message. */
			if ((mbufferid = mbuffer_alloc(active->mbufferpool)) < 0)
			{
				ret = (mbufferid);
				goto error;
			}
			
			buf = mbuffer_get(active->mbufferpool, mbufferid);

			buf->message.dest = config->remote;

			t1 = clock_read();
				active->do_copy(buf, config, ACTIVE_COPY_TO_MBUFFER);
			t2 = clock_read();

			/* Checks if the destination is the local node. */
			if (node_is_local(active->remote))
			{
				/* Forwards the message to the mbuffers table. */
				mbuffer_release(active->mbufferpool, mbufferid, MBUFFER_KEEP_MESSAGE);

				/* Update performance statistics. */
				stats->latency += (t2 - t1);
				stats->volume  += (config->size);

				spinlock_unlock(&active->lock);

				return (COMM_STATUS_RECEIVED);
			}
			else
				port->mbufferid = mbufferid;
		}
		else
			buf = mbuffer_get(active->mbufferpool, mbufferid);

		ret = (-EBUSY);

		/* Bad mailbox. */
		if (resource_is_busy(&active->resource))
			goto error;

		t1 = clock_read();

			/* Setup asynchronous write. */
			if ((ret = active->do_awrite(active->hwfd, (void *) &buf->message, active->size)) < 0)
				goto error;

		t2 = clock_read();

		/* Update performance statistics. */
		stats->latency += (t2 - t1);

		/* Sets the mailbox as busy. */
		resource_set_busy(&active->resource);

		ret = (COMM_STATUS_SUCCESS);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

/*============================================================================*
 * do_mailbox_wait()                                                          *
 *============================================================================*/

/**
 * @brief Waits on a mailbox to finish an assynchronous operation.
 *
 * @param fd Logic ID of the target mailbox.
 *
 * @returns Upon successful completion, a positive number is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int active_wait(
	const struct active_pool * pool,
	int id,
	const struct comm_config * config,
	struct pstats * stats
)
{
	int ret;       /* Return value.                   */
	int src;      /* Msg destination address.        */
	int dest;      /* Msg destination address.        */
	int local_addr;     /* Vmailbox hardware address.      */
	int mbufferid; /* Allocated mbufferid.            */
	int keep_rule; /* Discard rule.                   */
	uint64_t t1;   /* Clock value before awrite call. */
	uint64_t t2;   /* Clock value after awrite call.  */
	struct port * port;
	struct mbuffer * buf;
	struct active * active;

	active = &pool->actives[ACTIVE_GET_LADDRESS_FD(pool->actives, id)];
	port   = &active->portpool.ports[ACTIVE_GET_LADDRESS_PORT(active, id)];

	spinlock_lock(&active->lock);

		ret = (-EBADF);

		/* Bad mailbox. */
		if (!resource_is_used(&port->resource))
			goto error;

		/* Bad mailbox. */
		if (!resource_is_busy(&active->resource))
			goto error;

		/* Invalid mbufferid. */
		if ((mbufferid = port->mbufferid) < 0)
			goto error;

	spinlock_unlock(&active->lock);

	t1 = clock_read();

		/* Wait for asynchronous write to finish. */
		ret = active->do_wait(active->hwfd);

	t2 = clock_read();

	spinlock_lock(&active->lock);

		keep_rule = MBUFFER_DISCARD_MESSAGE;

		/* Was there no error? */
		if (ret == 0)
		{
			ret = (COMM_STATUS_SUCCESS);

			/* Read communication has extra operations. */
			if (resource_is_readable(&active->resource))
			{
				buf = mbuffer_get(active->mbufferpool, mbufferid);

				src   = buf->message.src;
				dest  = buf->message.dest;

				local_addr = ACTIVE_LADDRESS_COMPOSE(
					active->local,
					ACTIVE_GET_LADDRESS_PORT(active, id),
					ACTIVE_GET_NR_PORTS(active)
				);

				/* Checks if the message is addressed for the requesting port. */
				/* Consumes the message. */
				if ((local_addr == dest) && (config->remote == -1 || (config->remote != -1 && config->remote == src)))
					active->do_copy(buf, config, ACTIVE_COPY_FROM_MBUFFER);

				/* Ignore the message. */
				else
				{
					/* Keep message? true (MBUFFER_KEEP_MESSAGE) : false (MBUFFER_DISCARD_MESSAGE) */
					keep_rule = resource_is_used(
						&active->portpool.ports[ACTIVE_GET_LADDRESS_PORT(active, dest)].resource
					);

					/* Returns sinalizing that a message was read, but not for local port. */
					ret = (COMM_STATUS_AGAIN);
				}
			}

			if (ret == COMM_STATUS_SUCCESS)
				stats->volume += (config->size);

			/* Update performance statistics. */
			stats->latency += (t2 - t1);
		}

		port->mbufferid = -1;
		mbuffer_release(active->mbufferpool, mbufferid, keep_rule);

		resource_set_notbusy(&active->resource);

error:
	spinlock_unlock(&active->lock);

	return (ret);
}

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

PRIVATE void _active_free(const struct active_pool * pool, int id)
{
	resource_set_unused(&pool->actives[id].resource);
}

/**
 * @brief Creates a hardware mailbox.
 *
 * @param local Logic ID of the target local node.
 *
 * @returns Upon successful completion, the ID of the newly created
 * hardware mailbox is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int active_create(const struct active_pool * pool, int local)
{
	int hwfd;  /* File descriptor. */
	int actid;    /* Mailbox ID.      */
	struct active * active;

	/* Search target hardware mailbox. */
	if ((actid = active_search(pool, local, -1, COMM_TYPE_INPUT)) >= 0)
		return (-EBUSY);

	/* Allocate resource. */
	if ((active = _active_alloc(pool)) == NULL)
		return (-EAGAIN);

	/* Create underlying input hardware mailbox. */
	if ((hwfd = active->do_create(local)) < 0)
	{
		_active_free(pool, (active - pool->actives));
		return (hwfd);
	}

	/* Initialize hardware mailbox. */
	active->hwfd     = hwfd;
	active->local    = local;
	active->remote   = -1;
	active->refcount = 0;
	resource_set_rdonly(&active->resource);
	resource_set_notbusy(&active->resource);

	return (active - pool->actives);
}

/*============================================================================*
 * active_open()                                                         *
 *============================================================================*/

/**
 * @brief Opens a hardware mailbox.
 *
 * @param remote Logic ID of the target remote node.
 *
 * @returns Upon successful completion, the ID of the newly opened
 * hardware mailbox is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int active_open(const struct active_pool * pool, int local, int remote)
{
	int hwfd;   /* Mailbox ID.      */
	int actid; /* File descriptor. */
	struct active * active;

	/* Search target hardware mailbox. */
	if ((actid = active_search(pool, local, remote, COMM_TYPE_OUTPUT)) >= 0)
		return (actid);

	/* Allocate resource. */
	if ((active = _active_alloc(pool)) == NULL)
		return (-EAGAIN);

	hwfd = -EINVAL;

	if (node_is_local(local) && !node_is_local(remote))
	{
		/* Open underlying output hardware mailbox. */
		if ((hwfd = active->do_open(local, remote)) < 0)
		{
			_active_free(pool, (active - pool->actives));
			return (hwfd);
		}
	}
	else if (!node_is_local(local))
		return (-EINVAL);

	/* Initialize hardware mailbox. */
	active->hwfd     = hwfd;
	active->refcount = 0;
	active->local    = local;
	active->remote   = remote;
	resource_set_wronly(&active->resource);
	resource_set_notbusy(&active->resource);

	return (active - pool->actives);
}

// /*============================================================================*
//  * _do_mailbox_release()                                                      *
//  *============================================================================*/

// /**
//  * @brief Releases a hardware mailbox.
//  *
//  * @param fd         ID of the target hardware mailbox.
//  * @param release_fn Release function.
//  *
//  * @returns Upon successful completion, zero is returned. Upon
//  * failure, a negative error code is returned instead.
//  */
// PRIVATE int _do_mailbox_release(int fd, int (*release_fn)(int))
// {
// 	int ret;  /* Return value.   */
// 	int hwfd; /* HWFD allocated on HAL. */

// 	/* Checks if there is a hwfd allocated to this mailbox. */
// 	if ((hwfd = active_mailboxes[fd].hwfd) >= 0)
// 	{
// 		if ((ret = release_fn(hwfd)) < 0)
// 			return (ret);
// 	}

// 	active_mailboxes[fd].hwfd    = -1;
// 	active_mailboxes[fd].nodenum = -1;
// 	resource_free(&mbxpool, fd);

// 	dcache_invalidate();
// 	return (0);
// }