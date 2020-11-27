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
#include <nanvix/kernel/thread.h>
#include <nanvix/kernel/sync.h>
#include <nanvix/kernel/syscall.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/hlib.h>
#include <posix/errno.h>

#if __TARGET_HAS_SYNC && !__NANVIX_IKC_USES_ONLY_MAILBOX

/**
 * @brief Number of sync points.
 */
#define HW_SYNC_MAX (SYNC_CREATE_MAX + SYNC_OPEN_MAX)

/**
 * @name Search types for do_sync_search().
 */
/**@{*/
#define VSYNC_TYPE_INPUT  (0)
#define VSYNC_TYPE_OUTPUT (1)
/**@}*/

/**
 * @name Fynctions wrappers.
 */
/**@{*/
typedef int (* hw_alloc_fn)(const int *, int, int);
typedef int (* hw_operation_fn)(int);
typedef int (* hw_release_fn)(int);
/**@}*/

/*============================================================================*
 * Counters structure.                                                        *
 *============================================================================*/

/**
 * @brief Communicator counters.
 */
PRIVATE struct
{
	uint64_t ncreates; /**< Number of creates. */
	uint64_t nunlinks; /**< Number of unlinks. */
	uint64_t nopens;   /**< Number of opens.   */
	uint64_t ncloses;  /**< Number of closes.  */
	uint64_t nwaits;   /**< Number of watis.   */
	uint64_t nsignals; /**< Number of signals. */
} vsync_counters;

/*============================================================================*
 * Control Structures.                                                        *
 *============================================================================*/

/**
 * @brief Table of active synchronization points.
 */
PRIVATE struct sync
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource; /**< Generic resource information. */

	/**
	 * @name Control and Operate variables.
	 */
	/**@{*/
	int hwfd;                 /**< Underlying file descriptor.   */
	int refcount;             /**< Reference counter.            */
	/**@}*/

	/**
	 * @name Identification variables.
	 */
	/**@{*/
	int mode;                 /**< Mode of the operation.        */
	int master;               /**< Node number of the ONE.       */
	uint64_t nodeslist;       /**< Nodeslist.                    */
	/**@}*/

	/**
	 * @name Statistics variables.
	 */
	/**@{*/
	uint64_t latency;         /**< Latency counter.              */
	/**@}*/

	/**
	 * @name Wait/Wakeup controllers.
	 */
	/**@{*/
	struct semaphore waiting;              /**< Thread queue waiting for a communication.   */
	/**@}*/

} ALIGN(sizeof(dword_t)) vsynctab[HW_SYNC_MAX];

/**
 * @brief Resource pool.
 */
PRIVATE const struct resource_pool vsyncpool = {
	vsynctab, HW_SYNC_MAX, sizeof(struct sync)
};

/**
 * @brief Global lock.
 */
PRIVATE spinlock_t vsync_lock = SPINLOCK_UNLOCKED;

/*============================================================================*
 * sync_wait_active()                                                         *
 *============================================================================*/

/**
 * @brief Waits a communication finishs on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PRIVATE void sync_wait_active(int hwfd)
{
	/* Search for the virtual sync. */
	for (int i = 0; i < HW_SYNC_MAX; ++i)
	{
		if (!resource_is_used(&vsynctab[i].resource))
			continue;

		/* Found. */
		if (vsynctab[i].hwfd == hwfd)
		{
			/* It myst be set to busy before the wait operation. */
			KASSERT(resource_is_busy(&vsynctab[i].resource));

			semaphore_down(&vsynctab[i].waiting);

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][sync] Tried to wait for an invalid active sync.");
}

/*============================================================================*
 * sync_wait_active()                                                         *
 *============================================================================*/

/**
 * @brief Complete a communication on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PRIVATE void sync_wakeup_active(int hwfd)
{
	/* Search for the virtual sync. */
	for (int i = 0; i < HW_SYNC_MAX; ++i)
	{
		if (!resource_is_used(&vsynctab[i].resource))
			continue;

		/* Found. */
		if (vsynctab[i].hwfd == hwfd)
		{
			semaphore_up(&vsynctab[i].waiting);

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][sync] Tried to wake up for an invalid active sync.");
}

/*============================================================================*
 * do_sync_search()                                                           *
 *============================================================================*/

/**
 * @brief Searches for a sync.
 *
 * Searches for an already existing sync in syncs.
 *
 * @param master    Logic ID of the master node.
 * @param nodeslist Involved nodes list.
 *
 * @returns Upon successful completion, the ID of the HW sync found is
 * returned. Upon failure, a negative error code is returned instead.
 */
PRIVATE int do_sync_search(int master, uint64_t nodeslist, int mode, int type)
{
	for (int i = 0; i < HW_SYNC_MAX; ++i)
	{
		if (!resource_is_used(&vsynctab[i].resource))
			continue;

		if (type == VSYNC_TYPE_INPUT)
		{
			if (!resource_is_readable(&vsynctab[i].resource))
				continue;
		}

		/* type == VSYNC_TYPE_OUTPUT */
		else if (!resource_is_writable(&vsynctab[i].resource))
			continue;

		/* Not the same mode? */
		if (vsynctab[i].mode != mode)
			continue;

		/* Not the same master? */
		if (vsynctab[i].master != master)
			continue;

		/* Not the same node list? */
		if (vsynctab[i].nodeslist != nodeslist)
			continue;

		return (i);
	}

	return (-1);
}

/*============================================================================*
 * sync_build_nodeslist()                                                   *
 *============================================================================*/

/**
 * @brief Node list validation.
 *
 * @param nodes  IDs of target NoC nodes.
 * @param nnodes Number of target NoC nodes.
 *
 * @return Non zero if node list is valid and zero otherwise.
 */
PRIVATE uint64_t sync_build_nodeslist(const int * nodes, int nnodes)
{
	uint64_t nodeslist = 0ULL;

	for (int j = 0; j < nnodes; j++)
		nodeslist |= (1ULL << nodes[j]);

	return (nodeslist);
}

/*============================================================================*
 * do_vsync_create()                                                          *
 *============================================================================*/

/**
 * @brief Creates a hardware synchronization point.
 *
 * @param nodes     Logic IDs of target nodes.
 * @param nnodes    Number of target nodes.
 * @param type      Type of synchronization point.
 * @param nodeslist Target nodes footprint.
 *
 * @returns Upon successful completion, the ID of the newly created
 * synchronization point is returned. Upon failure, a negative error
 * code is returned instead.
 */
PRIVATE int _do_sync_alloc(
	const int * nodes,
	int nnodes,
	int mode,
	int type,
	hw_alloc_fn do_alloc
)
{
	int ret;            /* Return value.           */
	int hwfd;           /* File descriptor.        */
	int syncid;         /* Synchronization point.  */
	uint64_t nodeslist; /* Target nodes list.      */

	nodeslist = sync_build_nodeslist(nodes, nnodes);

	spinlock_lock(&vsync_lock);

		ret = (-EBUSY);

		/* Allocate a synchronization point. */
		if ((syncid = do_sync_search(nodes[0], nodeslist, mode, type)) >= 0)
		{
			vsynctab[syncid].refcount++;
			ret = (syncid);
			goto error;
		}

		ret = (-EAGAIN);

		/* Allocate a synchronization point. */
		if ((syncid = resource_alloc(&vsyncpool)) < 0)
			goto error;

		if ((hwfd = do_alloc(nodes, nnodes, mode)) < 0)
		{
			resource_free(&vsyncpool, syncid);
			ret = (hwfd);

			goto error;
		}

		/* Initialize synchronization point. */
		vsynctab[syncid].refcount  = 1;
		vsynctab[syncid].hwfd      = hwfd;
		vsynctab[syncid].mode      = mode;
		vsynctab[syncid].master    = nodes[0];
		vsynctab[syncid].nodeslist = nodeslist;
		vsynctab[syncid].latency   = 0ULL;

		if (type == VSYNC_TYPE_INPUT)
		{
			resource_set_rdonly(&vsynctab[syncid].resource);
			vsync_counters.ncreates++;
		}
		else
		{
			resource_set_wronly(&vsynctab[syncid].resource);
			vsync_counters.nopens++;
		}

		ret = (syncid);
error:
	spinlock_unlock(&vsync_lock);

	return (ret);
}

/*============================================================================*
 * do_vsync_create()                                                          *
 *============================================================================*/

/**
 * @brief Creates a hardware synchronization point.
 *
 * @param nodes     Logic IDs of target nodes.
 * @param nnodes    Number of target nodes.
 * @param mode      Type of synchronization point.
 *
 * @returns Upon successful completion, the ID of the newly created
 * synchronization point is returned. Upon failure, a negative error
 * code is returned instead.
 */
PUBLIC int do_vsync_create(const int *nodes, int nnodes, int mode)
{
	return (
		_do_sync_alloc(
			nodes,
			nnodes,
			mode,
			VSYNC_TYPE_INPUT,
			sync_create
		)
	);
}


/*============================================================================*
 * do_vsync_open()                                                          *
 *============================================================================*/

/**
 * @brief Creates a hardware synchronization point.
 *
 * @param nodes     Logic IDs of target nodes.
 * @param nnodes    Number of target nodes.
 * @param mode      Type of synchronization point.
 *
 * @returns Upon successful completion, the ID of the newly created
 * synchronization point is returned. Upon failure, a negative error
 * code is returned instead.
 */
PUBLIC int do_vsync_open(const int * nodes, int nnodes, int mode)
{
	return (
		_do_sync_alloc(
			nodes,
			nnodes,
			mode,
			VSYNC_TYPE_OUTPUT,
			sync_open
		)
	);
}

/*============================================================================*
 * _do_sync_release()                                                         *
 *============================================================================*/

/**
 * @brief Release a synchronization resource.
 *
 * @param syncid     ID of the target synchronization point.
 * @param release_fn Underlying release function.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PRIVATE int _do_sync_release(int syncid, int type, hw_release_fn do_release)
{
	int ret; /* Return value. */

	spinlock_lock(&vsync_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&vsynctab[syncid].resource))
			goto error;


		/* Bad sync. */
		if (type == VSYNC_TYPE_INPUT)
		{
			if (!resource_is_readable(&vsynctab[syncid].resource))
				goto error;
		}

		/* type == VSYNC_TYPE_OUTPUT */
		else if (!resource_is_writable(&vsynctab[syncid].resource))
			goto error;

		ret = (-EBUSY);

		/* Sync not set as busy. */
		if (resource_is_busy(&vsynctab[syncid].resource))
			goto error;

		/* Releases the virtual sync. */
		if ((--vsynctab[syncid].refcount) == 0)
		{
			/* Releases the hardware sync. */
			if ((ret = do_release(vsynctab[syncid].hwfd)) < 0)
				goto error;

			vsynctab[syncid].hwfd      = -1;
			vsynctab[syncid].master    = -1;
			vsynctab[syncid].nodeslist = 0ULL;

			if (type == VSYNC_TYPE_INPUT)
				vsync_counters.nunlinks++;
			else
				vsync_counters.ncloses++;

			resource_free(&vsyncpool, syncid);
		}

		ret = (0);
error:
	spinlock_unlock(&vsync_lock);

	return (ret);
}

/*============================================================================*
 * do_vsync_unlink()                                                          *
 *============================================================================*/

/**
 * @brief Unlinks a created virtual synchronization point.
 *
 * @param syncid Logic ID of the target virtual synchronization point.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vsync_unlink(int syncid)
{
	return (_do_sync_release(syncid, VSYNC_TYPE_INPUT, sync_unlink));
}

/*============================================================================*
 * do_vsync_close()                                                           *
 *============================================================================*/

/**
 * @brief Closes an opened virtual synchronization point.
 *
 * @param syncid Logic ID of the target virtual synchronization point.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vsync_close(int syncid)
{
	return (_do_sync_release(syncid, VSYNC_TYPE_OUTPUT, sync_close));
}

/*============================================================================*
 * _do_sync_operate()                                                         *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int _do_sync_operate(int syncid, int type, hw_operation_fn do_operation)
{
	int ret;     /* Return value.                 */
	uint64_t t1; /* Clock value before operation. */
	uint64_t t2; /* Clock value after operation.  */

	spinlock_lock(&vsync_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&vsynctab[syncid].resource))
			goto error;

		/* Bad sync. */
		if (type == VSYNC_TYPE_INPUT)
		{
			if (!resource_is_readable(&vsynctab[syncid].resource))
				goto error;
		}

		/* type == VSYNC_TYPE_OUTPUT */
		else if (!resource_is_writable(&vsynctab[syncid].resource))
			goto error;

		ret = (-EBUSY);

		/* Sync not set as busy. */
		if (resource_is_busy(&vsynctab[syncid].resource))
			goto error;

		resource_set_busy(&vsynctab[syncid].resource);

	spinlock_unlock(&vsync_lock);

	t1 = clock_read();
		ret = do_operation(vsynctab[syncid].hwfd);
	t2 = clock_read();

	spinlock_lock(&vsync_lock);
		if (ret >= 0)
		{
			vsynctab[syncid].latency += (t2 - t1);

			if (type == VSYNC_TYPE_INPUT)
				vsync_counters.nwaits++;
			else
				vsync_counters.nsignals++;
		}
		resource_set_notbusy(&vsynctab[syncid].resource);
error:
	spinlock_unlock(&vsync_lock);

	return (ret);
}
/*============================================================================*
 * do_vsync_wait()                                                            *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_vsync_wait(int syncid)
{
	return (_do_sync_operate(syncid, VSYNC_TYPE_INPUT, sync_wait));
}

/*============================================================================*
 * do_vsync_signal()                                                          *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC int do_vsync_signal(int syncid)
{
	return (_do_sync_operate(syncid, VSYNC_TYPE_OUTPUT, sync_signal));
}

/*============================================================================*
 * do_vsync_ioctl()                                                           *
 *============================================================================*/

PRIVATE int vsync_ioctl_valid(void * ptr, size_t size)
{
	return ((ptr != NULL) && mm_check_area(VADDR(ptr), size, UMEM_AREA));
}

/**
 * @brief Request an I/O operation on a synchronization point.
 *
 * @param syncid  Sync resource.
 * @param request Type of request.
 * @param args    Arguments of the request.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_vsync_ioctl(int syncid, unsigned request, va_list args)
{
	int ret;        /* Return value.              */
	uint64_t * var; /* Auxiliar variable pointer. */

	spinlock_lock(&vsync_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&vsynctab[syncid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&vsynctab[syncid].resource))
			goto error;

		ret = (-EFAULT);
		var = va_arg(args, uint64_t *);

		/* Bad buffer. */
		if (!vsync_ioctl_valid(var, sizeof(uint64_t)))
			goto error;

		ret = 0;

		/* Parse request. */
		switch (request)
		{
			case KSYNC_IOCTL_GET_LATENCY:
				*var = vsynctab[syncid].latency;
				break;

			case KSYNC_IOCTL_GET_NCREATES:
				*var = vsync_counters.ncreates;
				break;

			case KSYNC_IOCTL_GET_NUNLINKS:
				*var = vsync_counters.nunlinks;
				break;

			case KSYNC_IOCTL_GET_NOPENS:
				*var = vsync_counters.nopens;
				break;

			case KSYNC_IOCTL_GET_NCLOSES:
				*var = vsync_counters.ncloses;
				break;

			case KSYNC_IOCTL_GET_NWAITS:
				*var = vsync_counters.nwaits;
				break;

			case KSYNC_IOCTL_GET_NSIGNALS:
				*var = vsync_counters.nsignals;
				break;

			/* Operation not supported. */
			default:
				ret = (-ENOTSUP);
				break;
		}

error:
	spinlock_unlock(&vsync_lock);

	return (ret);
}

/*============================================================================*
 * vsync_init()                                                               *
 *============================================================================*/

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PUBLIC void vsync_init(void)
{
	kprintf("[kernel][noc] initializing the ksync facility");

	vsync_counters.ncreates = 0ULL;
	vsync_counters.nunlinks = 0ULL;
	vsync_counters.nopens   = 0ULL;
	vsync_counters.ncloses  = 0ULL;
	vsync_counters.nwaits   = 0ULL;
	vsync_counters.nsignals = 0ULL;

	for (unsigned i = 0; i < HW_SYNC_MAX; ++i)
	{
		vsynctab[i].resource  = RESOURCE_INITIALIZER;
		vsynctab[i].hwfd      = -1;
		vsynctab[i].refcount  =  0;
		vsynctab[i].mode      = -1;
		vsynctab[i].master    = -1;
		vsynctab[i].nodeslist = 0ULL;
		vsynctab[i].latency   = 0ULL;

		/* Waiting controllers. */
		semaphore_init(&vsynctab[i].waiting, 0);
	}

	/* Configure HAL Portal subsystem to use microkernel lock functions. */
	KASSERT(
		sync_ioctl(
			0, SYNC_IOCTL_SET_ASYNC_BEHAVIOR,
			sync_wait_active,
			sync_wakeup_active
	) == 0);
}

#endif /* __TARGET_SYNC && !__NANVIX_IKC_USES_ONLY_MAILBOX */
