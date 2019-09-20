/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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

#include <nanvix/hal/hal.h>
#include <nanvix/klib.h>
#include <posix/errno.h>

#if __TARGET_HAS_SYNC

/*============================================================================*
 * Control Structures.                                                        *
 *============================================================================*/

/**
 * @brief Table of synchronization points.
 */
PRIVATE struct sync
{
	struct resource resource; /**< Underlying resource.        */
	int refcount;             /**< References count.           */
	int fd;                   /**< Underlying file descriptor. */
	int type;                 /**< Sync type.                  */
	int masternum;            /**< Node number of the ONE.     */
	uint64_t footprint;       /**< Node ID list.               */
} ALIGN(sizeof(dword_t)) synctab[(SYNC_CREATE_MAX + SYNC_OPEN_MAX)];

/**
 * @brief Resource pool.
 */
PRIVATE const struct resource_pool syncpool = {
	synctab, (SYNC_CREATE_MAX + SYNC_OPEN_MAX), sizeof(struct sync)
};

/*============================================================================*
 * do_sync_is_valid()                                                         *
 *============================================================================*/

/**
 * @brief Asserts whether or not a synchronization point is valid.
 *
 * @param syncid ID of the target synchronization point.
 *
 * @returns One if the target synchronization point is valid, and false
 * otherwise.
 *
 * @note This function is non-blocking.
 * @note This function is thread-safe.
 * @note This function is reentrant.
 */
PRIVATE int do_sync_is_valid(int syncid)
{
	return WITHIN(syncid, 0, (SYNC_CREATE_MAX + SYNC_OPEN_MAX));
}

/*============================================================================*
 * do_sync_create()                                                           *
 *============================================================================*/

/**
 * @brief Creates a synchronization point.
 *
 * @param nodes     Logic IDs of Target Nodes.
 * @param nnodes    Number of Target Nodes.
 * @param type      Type of synchronization point.
 * @param footprint Target nodes footprint.
 *
 * @returns Upon successful completion, the ID of the newly created
 * synchronization point is returned. Upon failure, a negative error
 * code is returned instead.
 */
PRIVATE int _do_sync_create(const int *nodes, int nnodes, int type, uint64_t footprint)
{
	int fd;
	int syncid;

	/* Allocate a synchronization point. */
	if ((syncid = resource_alloc(&syncpool)) < 0)
		return (-EAGAIN);

	if ((fd = sync_create(nodes, nnodes, type)) < 0)
	{
		resource_free(&syncpool, syncid);
		return (fd);
	}

	/* Initialize synchronization point. */
	synctab[syncid].fd        = fd;
	synctab[syncid].type      = type;
	synctab[syncid].refcount  = 1;
	synctab[syncid].masternum = nodes[0];
	synctab[syncid].footprint = footprint;

	resource_set_rdonly(&synctab[syncid].resource);
	resource_set_notbusy(&synctab[syncid].resource);

	return (syncid);
}

/**
 * @see _do_sync_create().
 */
PUBLIC int do_sync_create(const int *nodes, int nnodes, int type)
{
	int syncid;         /* Synchronization point.  */
	uint64_t footprint; /* Target nodes footprint. */

	footprint = 0ULL;
	for (int j = 0; j < nnodes; j++)
		footprint |= (1ULL << nodes[j]);

	/* Searchs for existing syncs. */
	for (int i = 0; i < (SYNC_CREATE_MAX + SYNC_OPEN_MAX); ++i)
	{
		if (!resource_is_used(&synctab[i].resource))
			continue;

		if (!resource_is_readable(&synctab[i].resource))
			continue;

		/* Not the same master? */
		if (synctab[i].masternum != nodes[0])
			continue;

		/* Not the same node list? */
		if (synctab[i].footprint != footprint)
			continue;

		/* Not the same type operation? */
		if (synctab[i].type != type)
			continue;

		syncid = i;
		synctab[i].refcount++;

		goto found;
	}

	/* Alloc a new synchronization point. */
	syncid = _do_sync_create(nodes, nnodes, type, footprint);

found:
	dcache_invalidate();

	return (syncid);
}

/*============================================================================*
 * do_sync_open()                                                             *
 *============================================================================*/

/**
 * @brief Opens a synchronization point.
 *
 * @param nodes  Logic IDs of Target Nodes.
 * @param nnodes Number of Target Nodes.
 * @param type   Type of synchronization point.
 * @param footprint Target nodes footprint.
 *
 * @returns Upon successful completion, the ID of the target
 * synchronization point is returned. Upon failure, a negative error
 * code is returned instead.
 *
 * @todo Check for Invalid Remote
 */
PRIVATE int _do_sync_open(const int *nodes, int nnodes, int type, uint64_t footprint)
{
	int fd;		/* File descriptor.       */
	int syncid; /* Synchronization point. */

	/* Allocate a synchronization point. */
	if ((syncid = resource_alloc(&syncpool)) < 0)
		return (-EAGAIN);

	/* Open connector. */
	if ((fd = sync_open(nodes, nnodes, type)) < 0)
	{
		resource_free(&syncpool, syncid);
		return (fd);
	}

	/* Initialize synchronization point. */
	synctab[syncid].fd        = fd;
	synctab[syncid].type      = type;
	synctab[syncid].refcount  = 1;
	synctab[syncid].masternum = nodes[0];
	synctab[syncid].footprint = footprint;

	resource_set_wronly(&synctab[syncid].resource);
	resource_set_notbusy(&synctab[syncid].resource);

	return (syncid);
}

/**
 * @see _do_sync_open().
 */
PUBLIC int do_sync_open(const int *nodes, int nnodes, int type)
{
	int syncid;         /* Synchronization point.  */
	uint64_t footprint; /* Target nodes footprint. */

	footprint = 0ULL;
	for (int j = 0; j < nnodes; j++)
		footprint |= (1ULL << nodes[j]);

	/* Searchs for existing syncs. */
	for (int i = 0; i < (SYNC_CREATE_MAX + SYNC_OPEN_MAX); ++i)
	{
		if (!resource_is_used(&synctab[i].resource))
			continue;

		if (!resource_is_writable(&synctab[i].resource))
			continue;

		/* Not the same master? */
		if (synctab[i].masternum != nodes[0])
			continue;

		/* Not the same node list? */
		if (synctab[i].footprint != footprint)
			continue;

		/* Not the same type operation? */
		if (synctab[i].type != type)
			continue;

		syncid = i;
		synctab[i].refcount++;

		goto found;
	}

	/* Alloc a new synchronization point. */
	syncid = _do_sync_open(nodes, nnodes, type, footprint);

found:
	dcache_invalidate();

	return (syncid);
}

/*============================================================================*
 * _do_sync_release()                                                         *
 *============================================================================*/

/**
 * @brief Relase a synchronization resource.
 *
 * @param syncid     ID of the target synchronization point.
 * @param release_fn Underlying release function.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PRIVATE int _do_sync_release(int syncid, int (*release_fn)(int))
{
	int ret; /* HAL function return. */

	synctab[syncid].refcount--;

	if (synctab[syncid].refcount == 0)
	{
		if ((ret = release_fn(synctab[syncid].fd)) < 0)
			return (ret);

		synctab[syncid].fd        = -1;
		synctab[syncid].masternum = -1;
		synctab[syncid].footprint = 0ULL;

		resource_free(&syncpool, syncid);

		dcache_invalidate();
	}

	return (0);
}

/*============================================================================*
 * do_sync_unlink()                                                           *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_sync_unlink(int syncid)
{
	/* Invalid sync. */
	if (!do_sync_is_valid(syncid))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_used(&synctab[syncid].resource))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_readable(&synctab[syncid].resource))
		return (-EBADF);

	/* Release resource. */
	return (_do_sync_release(syncid, sync_unlink));
}

/*============================================================================*
 * do_sync_close()                                                            *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_sync_close(int syncid)
{
	/* Invalid sync. */
	if (!do_sync_is_valid(syncid))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_used(&synctab[syncid].resource))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_writable(&synctab[syncid].resource))
		return (-EBADF);

	/* Release resource. */
	return (_do_sync_release(syncid, sync_close));
}

/*============================================================================*
 * do_sync_wait()                                                             *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_sync_wait(int syncid)
{
	/* Invalid sync. */
	if (!do_sync_is_valid(syncid))
		return (-EBADF);

	dcache_invalidate();

	/* Bad sync. */
	if (!resource_is_used(&synctab[syncid].resource))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_readable(&synctab[syncid].resource))
		return (-EBADF);

	/* Waits. */
	return (sync_wait(synctab[syncid].fd));
}

/*============================================================================*
 * do_sync_signal()                                                           *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_sync_signal(int syncid)
{
	/* Invalid sync. */
	if (!do_sync_is_valid(syncid))
		return (-EBADF);

	dcache_invalidate();

	/* Bad sync. */
	if (!resource_is_used(&synctab[syncid].resource))
		return (-EBADF);

	/* Bad sync. */
	if (!resource_is_writable(&synctab[syncid].resource))
		return (-EBADF);

	/* Sends signal. */
	return (sync_signal(synctab[syncid].fd));
}

#endif /* __TARGET_SYNC */
