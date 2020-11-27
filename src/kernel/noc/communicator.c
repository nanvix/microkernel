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

#include "communicator.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

/*============================================================================*
 * communicator_alloc()                                                       *
 *============================================================================*/

/**
 * @brief Allocate a communicator resource.
 *
 * @param pool   Communicator resource pool.
 * @param config Communication's configuration.
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the ID of the communicator is
 * returned. Upon failure, a negative error code is returned instead.
 */
PUBLIC int communicator_alloc(
	const struct communicator_pool * pool,
	struct active_config * config,
	int type
)
{
	struct communicator * comm;

	KASSERT(pool != NULL);

	/* Search for a free synchronization point. */
	for (int i = 0; i < pool->ncommunicators; i++)
	{
		comm = &pool->communicators[i];

		spinlock_lock(&comm->lock);

			/* Found. */
			if (!resource_is_used(&comm->resource))
			{
				comm->resource = RESOURCE_INITIALIZER;
				comm->flags    = 0;
				comm->config   = *config;
				comm->stats    = PSTATS_INITIALIZER;

				resource_set_used(&comm->resource);

				if (type == ACTIVE_TYPE_INPUT)
					resource_set_rdonly(&comm->resource);
				else
					resource_set_wronly(&comm->resource);

				spinlock_unlock(&comm->lock);

				return (i);
			}

		spinlock_unlock(&comm->lock);
	}

	return (-EAGAIN);
}

/*============================================================================*
 * communicator_free()                                                        *
 *============================================================================*/

/**
 * @brief Release a communicator resource.
 *
 * @param pool Communicator resource pool.
 * @param id   Communicator ID.
 * @param type Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int communicator_free(const struct communicator_pool * pool, int id, int type)
{
	int ret; /* Function return. */
	struct communicator * comm;

	KASSERT(pool != NULL);
	KASSERT(WITHIN(id, 0, pool->ncommunicators));

	comm = &pool->communicators[id];

	spinlock_lock(&comm->lock);

		ret = (-EBADF);

		/* Bad communicator. */
		if (!resource_is_used(&comm->resource))
			goto error;

		/* Bad communicator. */
		if (type == ACTIVE_TYPE_INPUT)
		{
			if (!resource_is_readable(&comm->resource))
				goto error;
		}

		/* Bad communicator (ACTIVE_TYPE_OUTPUT). */
		else if (!resource_is_writable(&comm->resource))
			goto error;

		ret = (-EBUSY);

		/* Busy communicator. */
		if (resource_is_busy(&comm->resource))
			goto error;

		/* Releases communicator. */
		if ((ret = comm->fn->do_release(comm->config.fd)) == 0)
		{
			resource_set_unused(&comm->resource);

			spinlock_lock(&comm->counters->lock);
				if (type == ACTIVE_TYPE_INPUT)
					comm->counters->nunlinks++;
				else
					comm->counters->ncloses++;
			spinlock_unlock(&comm->counters->lock);
		}
error:
	spinlock_unlock(&comm->lock);

	return (ret);
}

/*============================================================================*
 * communicator_operate()                                                     *
 *============================================================================*/

/**
 * @brief Call an operation from active resource.
 *
 * @param comm Communicator resource.
 * @param type Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, positive number is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC ssize_t communicator_operate(struct communicator * comm, int type)
{
	ssize_t ret; /* Return value. */

	KASSERT(comm != NULL);

	spinlock_lock(&comm->lock);

		ret = (-EBADF);

		/* Bad communicator. */
		if (!resource_is_used(&comm->resource))
			goto error;

		/* Bad communicator. */
		if (type == ACTIVE_TYPE_INPUT)
		{
			if (!resource_is_readable(&comm->resource))
				goto error;

			ret = (-EACCES);

			if (!communicator_is_allowed(comm))
				goto error;
		}

		/* Bad communicator (ACTIVE_TYPE_OUTPUT). */
		else if (!resource_is_writable(&comm->resource))
			goto error;

		ret = (-EBUSY);

		/* Busy communicator. */
		if (resource_is_busy(&comm->resource))
			goto error;

		ret = comm->fn->do_comm(
			comm->config.fd,
			&comm->config,
			&comm->stats
		);

		/* Successfully complete communication? */
		if (ret >= 0)
		{
			if (ret == ACTIVE_COMM_RECEIVED)
			{
				communicator_set_finished(comm);
				communicator_set_notallowed(comm);
			}

			ret = comm->config.size;

			/* Sets the communicator as busy. */
			resource_set_busy(&comm->resource);
		}

error:
	spinlock_unlock(&comm->lock);

	return (ret);
}

/*============================================================================*
 * communicator_wait()                                                         *
 *============================================================================*/

/**
 * @brief Wait an operation on active resource.
 *
 * @param comm Communicator resource.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int communicator_wait(struct communicator * comm)
{
	int ret; /* Return value. */

	KASSERT(comm != NULL);

	spinlock_lock(&comm->lock);

		/* Bad communicator. */
		if (!resource_is_used(&comm->resource))
		{
			spinlock_unlock(&comm->lock);
			return (-EBADF);
		}

		/* communicator not set as busy. */
		if (!resource_is_busy(&comm->resource))
		{
			spinlock_unlock(&comm->lock);
			return (-EBADF);
		}

		/* communicator already finished its last operation. */
		if (communicator_is_finished(comm))
		{
			communicator_set_notfinished(comm);

			ret = 0;
			goto release;
		}

	spinlock_unlock(&comm->lock);

	ret = comm->fn->do_wait(
		comm->config.fd,
		&comm->config,
		&comm->stats
	);

	spinlock_lock(&comm->lock);

		/* Revoke communicator allow. */
		if (ret == ACTIVE_COMM_SUCCESS)
		{
			communicator_set_notallowed(comm);

			/* Resets remote_addr for input communicators. */
			if (resource_is_readable(&comm->resource))
				comm->config.remote_addr = -1;

			spinlock_lock(&comm->counters->lock);
				if (resource_is_readable(&comm->resource))
					comm->counters->nreads++;
				else
					comm->counters->nwrites++;
			spinlock_unlock(&comm->counters->lock);
		}

release:
		comm->config.buffer = NULL;
		comm->config.size   = 0ULL;
		resource_set_notbusy(&comm->resource);
	spinlock_unlock(&comm->lock);

	return (ret);
}

/*============================================================================*
 * communicator_ioctl()                                                       *
 *============================================================================*/

PRIVATE int communicator_ioctl_valid(void * ptr, size_t size)
{
	return ((ptr != NULL) && mm_check_area(VADDR(ptr), size, UMEM_AREA));
}

/**
 * @brief Request an I/O operation on a communicator.
 *
 * @param comm    Communicator resource.
 * @param request Type of request.
 * @param args    Arguments of the request.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int communicator_ioctl(
	struct communicator * comm,
	unsigned request,
	va_list args
)
{
	int ret; /* Return value. */

	KASSERT(comm != NULL);

	spinlock_lock(&comm->lock);
	spinlock_lock(&comm->counters->lock);

		ret = (-EBADF);

		/* Bad communicator. */
		if (!resource_is_used(&comm->resource))
			goto error;

		ret = (-EBUSY);

		/* Busy communicator. */
		if (resource_is_busy(&comm->resource))
			goto error;

		ret = (-EFAULT);

		/* Parse request. */
		switch (request)
		{
			/* Get the amount of data transferred so far. */
			case COMM_IOCTL_GET_VOLUME:
			{
				size_t * volume = va_arg(args, size_t *);

				/* Bad buffer. */
				if (!communicator_ioctl_valid(volume, sizeof(size_t)))
					goto error;

				*volume = comm->stats.volume;
				ret = 0;
			} break;

			/* Sets the remote address for a single read. */
			case COMM_IOCTL_SET_REMOTE:
			{
				int nodenum = va_arg(args, int);
				int port_nr = va_arg(args, int);

				/* Trying to set remote of an output comm? */
				if (!resource_is_readable(&comm->resource))
					goto error;

				/* Remote already set? */
				if (comm->config.remote_addr != -1)
					goto error;

				comm->config.remote_addr = comm->fn->laddress_calc(nodenum, port_nr);

				ret = 0;
			} break;

			/* Get uint64_t parameter. */
			case COMM_IOCTL_GET_LATENCY:
			case COMM_IOCTL_GET_NCREATES:
			case COMM_IOCTL_GET_NUNLINKS:
			case COMM_IOCTL_GET_NOPENS:
			case COMM_IOCTL_GET_NCLOSES:
			case COMM_IOCTL_GET_NREADS:
			case COMM_IOCTL_GET_NWRITES:
			{
				uint64_t * var = va_arg(args, uint64_t *);

				/* Bad buffer. */
				if (!communicator_ioctl_valid(var, sizeof(uint64_t)))
					goto error;

				ret = 0;

				switch(request)
				{
					case COMM_IOCTL_GET_LATENCY:
						*var = comm->stats.latency;
						break;

					case COMM_IOCTL_GET_NCREATES:
						*var = comm->counters->ncreates;
						break;

					case COMM_IOCTL_GET_NUNLINKS:
						*var = comm->counters->nunlinks;
						break;

					case COMM_IOCTL_GET_NOPENS:
						*var = comm->counters->nopens;
						break;

					case COMM_IOCTL_GET_NCLOSES:
						*var = comm->counters->ncloses;
						break;

					case COMM_IOCTL_GET_NREADS:
						*var = comm->counters->nreads;
						break;

					case COMM_IOCTL_GET_NWRITES:
						*var = comm->counters->nwrites;
						break;

					/* Operation not supported. */
					default:
						ret = (-ENOTSUP);
						break;
				}
			} break;

			/* Operation not supported. */
			default:
				ret = (-ENOTSUP);
				break;
		}

error:
	spinlock_unlock(&comm->counters->lock);
	spinlock_unlock(&comm->lock);

	return (ret);
}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */
