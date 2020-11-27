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

#include "mbuffer.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

/*============================================================================*
 * mbuffer_alloc()                                                            *
 *============================================================================*/

/**
 * @brief Allocates a message buffer.
 *
 * @param pool Mbuffer resource pool.
 *
 * @return Upon successful completion, mbufferid returned. Upon failure,
 * a negative number is returned instead.
 */
PUBLIC int mbuffer_alloc(struct mbuffer_pool * pool)
{
	int ret;
	char *base = (char *) pool->mbuffers;
	int n = pool->nmbuffers;
	size_t size = pool->mbuffer_size;

	KASSERT(pool != NULL);
	ret = (-EBUSY);

	spinlock_lock(pool->lock);

		/* Search for a free synchronization point. */
		for (int i = 0; i < n; i++)
		{
			struct mbuffer * buf;

			buf = (struct mbuffer *)(&base[mult(i, size)]);

			/* Found. */
			if (!resource_is_used(&buf->resource))
			{
				buf->resource = RESOURCE_INITIALIZER;
				buf->age      = ~(0ULL);
				buf->latency  = (0ULL);
				buf->actid    = (-1);
				buf->portid   = (-1);
				buf->message  = MBUFFER_MESSAGE_INITIALIZER;
				resource_set_used(&buf->resource);

				ret = (i);
				break;
			}
		}

	spinlock_unlock(pool->lock);

	return (ret);
}

/*============================================================================*
 * mbuffer_release()                                                          *
 *============================================================================*/

/**
 * @brief Releases the message buffer allocated to @p id.
 *
 * @param pool     Mbuffer resource pool.
 * @param id       mbuffer id to release.
 * @param keep_msg Keep / Discard the mbuffer message?
 *
 * @return Upon successful completion, zero is returned. Upon failure,
 * a negative number is returned instead.
 */
PUBLIC int mbuffer_release(struct mbuffer_pool * pool, int id, int keep_msg)
{
	char * base;
	size_t size;
	struct mbuffer * buf;

	KASSERT(pool != NULL);
	KASSERT(WITHIN(id, 0, pool->nmbuffers));

	base = (char *) pool->mbuffers;
	size = pool->mbuffer_size;
	buf  = (struct mbuffer *)(&base[mult(id, size)]);

	spinlock_lock(pool->lock);

		if (!resource_is_used(&buf->resource))
		{
			spinlock_unlock(pool->lock);
			return (-EINVAL);
		}

		/* Sets the mbuffer as not used keeping its message. */
		if (keep_msg == MBUFFER_KEEP_MESSAGE)
		{
			buf->age = (*pool->curr_age)++;
			resource_set_busy(&buf->resource);
		}

		/* Frees the mbuffer resource. */
		else
		{
			buf->resource = RESOURCE_INITIALIZER;
			buf->age      = ~(0ULL);
			buf->latency  = (0ULL);
			buf->actid    = (-1);
			buf->portid   = (-1);
			buf->message  = MBUFFER_MESSAGE_INITIALIZER;
		}

	/* Unlocks the mbuffers table. */
	spinlock_unlock(pool->lock);

	return (0);
}

/*============================================================================*
 * mbuffer_search()                                                           *
 *============================================================================*/

/**
 * @brief Searches for a stored message destinated to @p local_address.
 *
 * @param pool Mbuffer resource pool.
 * @param dest Destination of the message.
 * @param srt  Source of the message.
 *
 * @returns Upon successful completion, the mbuffer that contains the first
 * message found is returned. A negative error number is returned instead.
 */
PUBLIC int mbuffer_search(struct mbuffer_pool * pool, int dest, int src)
{
	int ret;           /* Return value.                            */
	int n;             /* Number of mbuffers.                      */
	char * base;       /* Pointer to the first mbuffer.            */
	size_t size;       /* Size of a mbuffer.                       */
	uint64_t curr_age; /* The age of the current mbuffer selected. */

	KASSERT(pool != NULL);

	base     = (char *) pool->mbuffers;
	n        = pool->nmbuffers;
	size     = pool->mbuffer_size;
	ret      = (-EINVAL);
	curr_age = ~(0ULL);

	/* Locks the mbuffers table. */
	spinlock_lock(pool->lock);

		for (int i = 0; i < n; i++)
		{
			struct mbuffer * buf;

			buf = (struct mbuffer *)(&base[mult(i, size)]);

			/* Is the buffer being used? */
			if (!resource_is_used(&buf->resource))
				continue;

			/* The buffer contains a stored message? */
			if (!resource_is_busy(&buf->resource))
				continue;

			/* Is this message addressed to the dest? */
			if (buf->message.header.dest != dest)
				continue;

			/* Checks the message source if it was the expected. */
			if (!pool->source_check(buf, src))
				continue;

			/* Is the buffer the older? */
			if (buf->age > curr_age)
				continue;

			ret      = i;
			curr_age = buf->age;
		}

	/* Unlocks the mbuffers table. */
	spinlock_unlock(pool->lock);

	return (ret);
}

/*============================================================================*
 * mbuffer_get()                                                              *
 *============================================================================*/

/**
 * @brief Gets a mbuffer struct pointer.
 *
 * @param pool Mbuffer resource pool.
 * @param id Mbuffer ID.
 *
 * @returns Upon successful completion, A mbuffer pointer is returned.
 */
PUBLIC struct mbuffer * mbuffer_get(struct mbuffer_pool * pool, int id)
{
	char * base;
	size_t size;

	KASSERT(pool != NULL);
	KASSERT(WITHIN(id, 0, pool->nmbuffers));

	base = (char *) pool->mbuffers;
	size = pool->mbuffer_size;

	return (struct mbuffer *)(&base[mult(id, size)]);
}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */

