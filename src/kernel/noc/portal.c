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

#include "portal.h"

#if __TARGET_HAS_PORTAL && !__NANVIX_IKC_USES_ONLY_MAILBOX

/*============================================================================*
 * Control Structures.                                                        *
 *============================================================================*/

/**
 * @name Pool variables.
 */
/**@{*/
PRIVATE union portal_mbuffer pbuffers[KPORTAL_MESSAGE_BUFFERS_MAX]; /**< Portal message buffer.                        */
PRIVATE uint64_t pbuffers_age;                                      /**< Counter used to set mbuffer age.              */
PRIVATE spinlock_t pbuffers_lock;                                   /**< Protection of the mbuffer pools.              */
PRIVATE struct mbuffer_pool pbufferpool;                            /**< Pool with all of mbuffer available.           */
PRIVATE struct mbuffer_pool pbufferpool_aux;                        /**< Pool with a subset of mbuffer in mbufferpool. */
/**@}*/

/**
 * @name Physical Portal variables.
 */
/**@{*/
PRIVATE struct port portalports[HW_PORTAL_MAX][KPORTAL_PORT_NR]; /**< Portal ports.     */
PRIVATE short fifos[HW_PORTAL_MAX][KPORTAL_PORT_NR];             /**< Portal FIFOs.     */
PRIVATE struct active_functions portal_functions;                /**< Portal functions. */
PRIVATE struct active portals[HW_PORTAL_MAX];                    /**< Portals.          */
PRIVATE struct active_pool portalpool;                           /**< Portal pool.      */
/**@}*/

/**
 * @name Prototype functions.
 */
/**@{*/
PRIVATE int wrapper_portal_allow(struct active *, int);
PRIVATE int portal_header_config(struct mbuffer *, const struct active_config *);
PRIVATE int portal_header_check(struct mbuffer *, const struct active_config *);
PRIVATE int portal_source_check(struct mbuffer *, int);
PRIVATE int portal_get_actid(int);
PRIVATE int portal_get_portid(int);
/**@}*/

/**
 * @name Functions to wait/wakeup for a portal.
 */
/**{**/
PRIVATE void portal_wait_active(int hwfd);
PRIVATE void portal_wakeup_active(int hwfd);
/**}**/

/*============================================================================*
 * do_portal_table_init()                                                     *
 *============================================================================*/

/**
 * @brief Initializes the mbuffers table lock.
 */
PRIVATE void do_portal_table_init(void)
{
	/* Initializes the mbuffers. */
	for (int i = 0; i < KPORTAL_MESSAGE_BUFFERS_MAX; ++i)
	{
		pbuffers[i].abstract.resource = RESOURCE_INITIALIZER;
		pbuffers[i].abstract.age      = ~(0ULL);
		pbuffers[i].abstract.latency  = (0ULL);
		pbuffers[i].abstract.actid    = (-1);
		pbuffers[i].abstract.portid   = (-1);
		pbuffers[i].abstract.message  = MBUFFER_MESSAGE_INITIALIZER;
	}

	/* Initializes shared pool variables. */
	pbuffers_age = (0ULL);
	spinlock_init(&pbuffers_lock);

	/* Initializes principal mbuffers pool. */
	pbufferpool.mbuffers     = pbuffers;
	pbufferpool.nmbuffers    = KPORTAL_MESSAGE_BUFFERS_MAX;
	pbufferpool.mbuffer_size = sizeof(union portal_mbuffer);
	pbufferpool.curr_age     = &pbuffers_age;
	pbufferpool.lock         = &pbuffers_lock;
	pbufferpool.source_check = portal_source_check;

	/* Initializes auxiliary mbuffers pool. */
	pbufferpool_aux.mbuffers     = pbuffers + (KPORTAL_MESSAGE_BUFFERS_MAX - KPORTAL_AUX_BUFFERS_MAX);
	pbufferpool_aux.nmbuffers    = KPORTAL_AUX_BUFFERS_MAX;
	pbufferpool_aux.mbuffer_size = sizeof(union portal_mbuffer);
	pbufferpool_aux.curr_age     = &pbuffers_age;
	pbufferpool_aux.lock         = &pbuffers_lock;
	pbufferpool_aux.source_check = portal_source_check;

	portal_functions.do_create        = portal_create;
	portal_functions.do_open          = portal_open;
	portal_functions.do_allow         = wrapper_portal_allow;
	portal_functions.do_aread         = portal_aread;
	portal_functions.do_awrite        = portal_awrite;
	portal_functions.do_wait          = portal_wait;
	portal_functions.do_header_config = portal_header_config;
	portal_functions.do_header_check  = portal_header_check;
	portal_functions.get_actid        = portal_get_actid;
	portal_functions.get_portid       = portal_get_portid;

	/* Initializes the portals. */
	for (int i = 0; i < HW_PORTAL_MAX; ++i)
	{
		/* Initializes main variables. */
		spinlock_init(&portals[i].lock);
		portals[i].hwfd     = -1;
		portals[i].local    = -1;
		portals[i].remote   = -1;
		portals[i].refcount =  0;
		portals[i].size     = (KPORTAL_MESSAGE_HEADER_SIZE + KPORTAL_MESSAGE_DATA_SIZE);

		/* Initializes port pool. */
		portals[i].portpool.ports      = NULL;
		portals[i].portpool.nports     = KPORTAL_PORT_NR;
		portals[i].portpool.used_ports = 0;
		portals[i].portpool.ports      = portalports[i];

		/* Initializes request fifo. */
		portals[i].requests.head         = 0;
		portals[i].requests.tail         = 0;
		portals[i].requests.max_capacity = KPORTAL_PORT_NR;
		portals[i].requests.nelements    = 0;
		portals[i].requests.fifo         = fifos[i];

		/* Initializes the portals ports and FIFOs. */
		for (int j = 0; j < KPORTAL_PORT_NR; ++j)
		{
			portalports[i][j].resource    = RESOURCE_INITIALIZER;
			portalports[i][j].mbufferid   = -1;
			portalports[i][j].mbufferpool = NULL;

			fifos[i][j] = -1;
		}

		/* Initializes auxiliary functions. */
		portals[i].mbufferpool      = &pbufferpool;
		portals[i].mbufferpool_aux  = &pbufferpool_aux;
		portals[i].fn               = &portal_functions;

		/* Waiting controllers. */
		semaphore_init(&portals[i].waiting, 0);
	}

	/* Initializes portal pool. */
	portalpool.actives    = portals;
	portalpool.nactives   = HW_PORTAL_MAX;

	/* Configure HAL Portal subsystem to use microkernel lock functions. */
	KASSERT(
		portal_ioctl(
			0, HAL_PORTAL_IOCTL_SET_ASYNC_BEHAVIOR,
			portal_wait_active,
			portal_wakeup_active
	) == 0);
}

/*============================================================================*
 * wrapper_portal_allow()                                                     *
 *============================================================================*/

/**
 * @brief Allow a physical portal communication.
 *
 * @param act    Active resource.
 * @param remote Remote node ID.
 *
 * @returns Value return by portal_allow function.
 */
PRIVATE int wrapper_portal_allow(struct active * act, int remote)
{
	if (active_is_allowed(act))
	{
		if (act->remote != remote)
			return (-EBUSY);
		else
			return (0);
	}

	return (portal_allow(act->hwfd, remote));
}

/*============================================================================*
 * portal_header_config()                                                     *
 *============================================================================*/

/**
 * @brief Configurate a message header.
 *
 * @param buf    Mbuffer resource.
 * @param config Communication's configuration.
 *
 * @returns Zero is returned.
 */
PRIVATE int portal_header_config(struct mbuffer * buf, const struct active_config * config)
{
	buf->message.header.src  = config->local_addr;
	buf->message.header.dest = config->remote_addr;
	buf->message.header.size = config->size;

	return (0);
}

/*============================================================================*
 * portal_header_check()                                                      *
 *============================================================================*/

/**
 * @brief Checks if a message is to current configuration.
 *
 * @param buf    Mbuffer resource.
 * @param config Communication's configuration.
 *
 * @returns Non-zero if the mbuffer is destinate to current configuration.
 */
PRIVATE int portal_header_check(struct mbuffer * buf, const struct active_config * config)
{
	return (
		(buf->message.header.dest == config->local_addr) &&
		(portal_source_check(buf, config->remote_addr))
	);
}

/*============================================================================*
 * portal_source_check()                                                      *
 *============================================================================*/

/**
 * @brief Checks if the message source matches the expected mask.
 *
 * @param buf      Message buffer to be evaluated.
 * @param src_mask Expected source config.
 *
 * @returns Non-zero if the mbuffer src matches the current configuration, and
 * zero otherwise.
 */
PRIVATE int portal_source_check(struct mbuffer * buf, int src_mask)
{
	int msg_source;
	int mask_node;
	int mask_port;

	/* Return immediately. */
	if (src_mask == -1)
		return (1);

	mask_node = portal_get_actid(src_mask);
	mask_port = portal_get_portid(src_mask);
	msg_source = buf->message.header.src;

	/* Check nodenum. */
	if ((mask_node != PORTAL_ANY_SOURCE) && (mask_node != portal_get_actid(msg_source)))
		return (0);

	/* Check port number. */
	if ((mask_port != PORTAL_ANY_PORT) && (mask_port != portal_get_portid(msg_source)))
		return (0);

	return (1);
}

/*============================================================================*
 * portal_get_actid()                                                         *
 *============================================================================*/

/**
 * @brief Gets active id.
 *
 * @param id Composed ID.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PRIVATE int portal_get_actid(int id)
{
	return ((id < 0) ? (id) : (id / (KPORTAL_PORT_NR + 1)));
}

/*============================================================================*
 * portal_get_portid()                                                        *
 *============================================================================*/

/**
 * @brief Gets port id.
 *
 * @param id Composed ID.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PRIVATE int portal_get_portid(int id)
{
	return ((id < 0) ? (id) : (id % (KPORTAL_PORT_NR + 1)));
}

/*============================================================================*
 * portal_wait_active()                                                       *
 *============================================================================*/

/**
 * @brief Waits a communication finishs on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PRIVATE void portal_wait_active(int hwfd)
{
	/* Search for the active portal. */
	for (int i = 0; i < HW_PORTAL_MAX; ++i)
	{
		if (!resource_is_used(&portals[i].resource))
			continue;

		/* Found. */
		if (portals[i].hwfd == hwfd)
		{
			/* It myst be set to busy before the wait operation. */
			KASSERT(resource_is_busy(&portals[i].resource));

			semaphore_down(&portals[i].waiting);

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][portal] Tried to wait for an invalid active portal.");
}

/*============================================================================*
 * portal_wait_active()                                                       *
 *============================================================================*/

/**
 * @brief Complete a communication on a active.
 *
 * @param hwfd Hardware file descriptor allocated by the active.
 */
PRIVATE void portal_wakeup_active(int hwfd)
{
	/* Search for the active portal. */
	for (int i = 0; i < HW_PORTAL_MAX; ++i)
	{
		if (!resource_is_used(&portals[i].resource))
			continue;

		/* Found. */
		if (portals[i].hwfd == hwfd)
		{
			semaphore_up(&portals[i].waiting);

			return;
		}
	}

	/* Should not happens. */
	kpanic("[kernel][noc][portal] Tried to wake up for an invalid active portal.");
}

/*============================================================================*
 * portal_laddress_calc()                                                     *
 *============================================================================*/

/**
 * @brief Calculates the laddress of a communicator.
 *
 * @param fd   Node fd.
 * @param port Port number.
 *
 * @returns Returns the composed laddress.
 *
 * @note The correctness of parameters is responsibility of the caller. This
 * function only applies the parameters to the calculation formula.
 */
PUBLIC int portal_laddress_calc(int fd, int port)
{
	return (ACTIVE_LADDRESS_COMPOSE(fd, port, KPORTAL_PORT_NR));
}

/*============================================================================*
 * do_portal_alloc()                                                          *
 *============================================================================*/

/**
 * @brief Allocate a physical portal.
 *
 * @param local  Local node ID.
 * @param remote Remote node ID (It can be -1).
 * @param port   Port ID.
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the ID of the active portal is
 * returned. Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_portal_alloc(int local, int remote, int port, int type)
{
	return (active_alloc(&portalpool, local, remote, port, type));
}

/*============================================================================*
 * do_portal_release()                                                        *
 *============================================================================*/

/**
 * @brief Releases a physical portal.
 *
 * @param portalid Active portal ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_portal_release(int portalid)
{
	return (active_release(&portalpool, portalid));
}

/*============================================================================*
 * do_portal_aread()                                                          *
 *============================================================================*/

/**
 * @brief Async reads from an active.
 *
 * @param portalid  Active portal ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t do_portal_aread(int portalid, const struct active_config * config, struct pstats * stats)
{
	return (active_aread(&portalpool, portalid, config, stats));
}

/*============================================================================*
 * do_portal_awrite()                                                         *
 *============================================================================*/

/**
 * @brief Async writes from an active.
 *
 * @param portalid  Active portal ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t do_portal_awrite(int portalid, const struct active_config * config, struct pstats * stats)
{
	return (active_awrite(&portalpool, portalid, config, stats));
}

/*============================================================================*
 * do_portal_wait()                                                           *
 *============================================================================*/

/**
 * @brief Waits on a portal to finish an assynchronous operation.
 *
 * @param portalid  Active portal ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_portal_wait(int portalid, const struct active_config * config, struct pstats * stats)
{
	return (active_wait(&portalpool, portalid, config, stats));
}

/*============================================================================*
 * do_portal_init()                                                           *
 *============================================================================*/

/**
 * @todo Initializtion of the active portales and structures.
 */
PUBLIC void do_portal_init(void)
{
	int local;

	local = processor_node_get_num();

	KASSERT((((uintptr_t) &pbuffers[0].message) & 0x7) == 0);
	KASSERT((((uintptr_t) &pbuffers[1].message) & 0x7) == 0);
	KASSERT((sizeof(pbuffers[0].message) % 8) == 0);
	KASSERT((sizeof(pbuffers[1].message) % 8) == 0);

	/* Initializes the portals structures. */
	do_portal_table_init();

	/* Create the input portal. */
	KASSERT(_active_create(&portalpool, local) >= 0);

	/* Opens all portal interfaces. */
	for (int i = 0; i < PROCESSOR_NOC_NODES_NUM; ++i)
		KASSERT(_active_open(&portalpool, local, i) >= 0);
}

#endif /* __TARGET_HAS_PORTAL && !__NANVIX_IKC_USES_ONLY_MAILBOX */

