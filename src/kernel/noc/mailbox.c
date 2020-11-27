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

#include "mailbox.h"

#if __TARGET_HAS_MAILBOX

/*============================================================================*
 * Control Structures.                                                        *
 *============================================================================*/

/**
 * @name Pool variables.
 */
/**@{*/
PRIVATE union mailbox_mbuffer mbuffers[KMAILBOX_MESSAGE_BUFFERS_MAX]; /**< Mailbox message buffer.                       */
PRIVATE uint64_t mbuffers_age;                                        /**< Counter used to set mbuffer age.              */
PRIVATE spinlock_t mbuffers_lock;                                     /**< Protection of the mbuffer pools.              */
PRIVATE struct mbuffer_pool mbufferpool;                              /**< Pool with all of mbuffer available.           */
PRIVATE struct mbuffer_pool mbufferpool_aux;                          /**< Pool with a subset of mbuffer in mbufferpool. */
/**@}*/

/**
 * @name Physical Mailbox variables.
 */
/**@{*/
PRIVATE struct port mbxports[HW_MAILBOX_MAX][MAILBOX_PORT_NR]; /**< Mailbox ports.     */
PRIVATE short fifos[HW_MAILBOX_MAX][MAILBOX_PORT_NR];          /**< Mailbox FIFOs.     */
PRIVATE struct active_functions mailbox_functions;             /**< Mailbox functions. */
PRIVATE struct active mailboxes[HW_MAILBOX_MAX];               /**< Mailboxes.         */
PRIVATE struct active_pool mbxpool;                            /**< Mailbox pool.      */
/**@}*/

/**
 * @name Prototype functions.
 */
/**@{*/
PRIVATE int wrapper_mailbox_open(int, int);
PRIVATE int wrapper_mailbox_allow(struct active *, int);
PRIVATE int mailbox_header_config(struct mbuffer *, const struct active_config *);
PRIVATE int mailbox_header_check(struct mbuffer *, const struct active_config *);
PRIVATE int mailbox_source_check(struct mbuffer *, int);
PRIVATE int mailbox_get_actid(int);
PRIVATE int mailbox_get_portid(int);
/**@}*/

/*============================================================================*
 * do_mailbox_table_init()                                                    *
 *============================================================================*/

/**
 * @brief Initializes the mbuffers table lock.
 */
PRIVATE void do_mailbox_table_init(void)
{
	/* Initializes the mbuffers. */
	for (int i = 0; i < KMAILBOX_MESSAGE_BUFFERS_MAX; ++i)
	{
		mbuffers[i].abstract.resource = RESOURCE_INITIALIZER;
		mbuffers[i].abstract.age      = ~(0ULL);
		mbuffers[i].abstract.latency  = (0ULL);
		mbuffers[i].abstract.actid    = (-1);
		mbuffers[i].abstract.portid   = (-1);
		mbuffers[i].abstract.message  = MBUFFER_MESSAGE_INITIALIZER;
	}

	/* Initializes shared pool variables. */
	mbuffers_age = (0ULL);
	spinlock_init(&mbuffers_lock);

	/* Initializes principal mbuffers pool. */
	mbufferpool.mbuffers     = mbuffers;
	mbufferpool.nmbuffers    = KMAILBOX_MESSAGE_BUFFERS_MAX;
	mbufferpool.mbuffer_size = sizeof(union mailbox_mbuffer);
	mbufferpool.curr_age     = &mbuffers_age;
	mbufferpool.lock         = &mbuffers_lock;
	mbufferpool.source_check = mailbox_source_check;

	/* Initializes auxiliary mbuffers pool. */
	mbufferpool_aux.mbuffers     = mbuffers + (KMAILBOX_MESSAGE_BUFFERS_MAX - KMAILBOX_AUX_BUFFERS_MAX);
	mbufferpool_aux.nmbuffers    = KMAILBOX_AUX_BUFFERS_MAX;
	mbufferpool_aux.mbuffer_size = sizeof(union mailbox_mbuffer);
	mbufferpool_aux.curr_age     = &mbuffers_age;
	mbufferpool_aux.lock         = &mbuffers_lock;
	mbufferpool_aux.source_check = mailbox_source_check;

	mailbox_functions.do_create        = mailbox_create;
	mailbox_functions.do_open          = wrapper_mailbox_open;
	mailbox_functions.do_allow         = wrapper_mailbox_allow;
	mailbox_functions.do_aread         = mailbox_aread;
	mailbox_functions.do_awrite        = mailbox_awrite;
	mailbox_functions.do_wait          = mailbox_wait;
	mailbox_functions.do_header_config = mailbox_header_config;
	mailbox_functions.do_header_check  = mailbox_header_check;
	mailbox_functions.get_actid        = mailbox_get_actid;
	mailbox_functions.get_portid       = mailbox_get_portid;

	/* Initializes the mailboxes. */
	for (int i = 0; i < HW_MAILBOX_MAX; ++i)
	{
		/* Initializes main variables. */
		spinlock_init(&mailboxes[i].lock);
		mailboxes[i].hwfd     = -1;
		mailboxes[i].local    = -1;
		mailboxes[i].remote   = -1;
		mailboxes[i].refcount =  0;
		mailboxes[i].size     = (KMAILBOX_MESSAGE_HEADER_SIZE + KMAILBOX_MESSAGE_DATA_SIZE);

		/* Initializes port pool. */
		mailboxes[i].portpool.ports      = NULL;
		mailboxes[i].portpool.nports     = MAILBOX_PORT_NR;
		mailboxes[i].portpool.used_ports = 0;
		mailboxes[i].portpool.ports      = mbxports[i];

		/* Initializes request fifo. */
		mailboxes[i].requests.head         = 0;
		mailboxes[i].requests.tail         = 0;
		mailboxes[i].requests.max_capacity = MAILBOX_PORT_NR;
		mailboxes[i].requests.nelements    = 0;
		mailboxes[i].requests.fifo         = fifos[i];

		/* Initializes the mailboxes ports and FIFOs. */
		for (int j = 0; j < MAILBOX_PORT_NR; ++j)
		{
			mbxports[i][j].resource    = RESOURCE_INITIALIZER;
			mbxports[i][j].mbufferid   = -1;
			mbxports[i][j].mbufferpool = NULL;

			fifos[i][j] = -1;
		}

		/* Initializes auxiliary functions. */
		mailboxes[i].mbufferpool      = &mbufferpool;
		mailboxes[i].mbufferpool_aux  = &mbufferpool_aux;
		mailboxes[i].fn               = &mailbox_functions;
	}

	/* Initializes mailbox pool. */
	mbxpool.actives    = mailboxes;
	mbxpool.nactives   = HW_MAILBOX_MAX;
}

/*============================================================================*
 * wrapper_mailbox_open()                                                     *
 *============================================================================*/

/**
 * @brief Open a physical mailbox.
 *
 * @param local  Local node ID.
 * @param remote Remote node ID.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PRIVATE int wrapper_mailbox_open(int local, int remote)
{
	UNUSED(local);

	return (mailbox_open(remote));
}

/*============================================================================*
 * wrapper_mailbox_allow()                                                     *
 *============================================================================*/

/**
 * @brief Allow a physical mailbox communication.
 *
 * @param act    Active resource.
 * @param remote Remote node ID.
 *
 * @returns Zero is returned.
 */
PRIVATE int wrapper_mailbox_allow(struct active * act, int remote)
{
	UNUSED(act);
	UNUSED(remote);

	return (0);
}

/*============================================================================*
 * mailbox_header_config()                                                    *
 *============================================================================*/

/**
 * @brief Configurate a message header.
 *
 * @param buf    Mbuffer resource.
 * @param config Communication's configuration.
 *
 * @returns Zero is returned.
 */
PRIVATE int mailbox_header_config(struct mbuffer * buf, const struct active_config * config)
{
	buf->message.header.src  = config->local_addr;
	buf->message.header.dest = config->remote_addr;

	return (0);
}

/*============================================================================*
 * mailbox_header_check()                                                     *
 *============================================================================*/

/**
 * @brief Checks if a message is to current configuration.
 *
 * @param buf    Mbuffer resource.
 * @param config Communication's configuration.
 *
 * @returns Non-zero if the mbuffer is destinate to current configuration.
 */
PRIVATE int mailbox_header_check(struct mbuffer * buf, const struct active_config * config)
{
	return (
		(buf->message.header.dest == config->local_addr) &&
		(mailbox_source_check(buf, config->remote_addr))
	);
}

/*============================================================================*
 * mailbox_source_check()                                                     *
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
PRIVATE int mailbox_source_check(struct mbuffer * buf, int src_mask)
{
	int msg_source;
	int mask_node;
	int mask_port;

	/* Return immediately. */
	if (src_mask == -1)
		return (1);

	mask_node = mailbox_get_actid(src_mask);
	mask_port = mailbox_get_portid(src_mask);
	msg_source = buf->message.header.src;

	/* Check nodenum. */
	if ((mask_node != MAILBOX_ANY_SOURCE) && (mask_node != mailbox_get_actid(msg_source)))
		return (0);

	/* Check port number. */
	if ((mask_port != MAILBOX_ANY_PORT) && (mask_port != mailbox_get_portid(msg_source)))
		return (0);

	return (1);
}

/*============================================================================*
 * mailbox_get_actid()                                                        *
 *============================================================================*/

/**
 * @brief Gets active id.
 *
 * @param id Composed ID.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PRIVATE int mailbox_get_actid(int id)
{
	return ((id < 0) ? (id) : (id / (MAILBOX_PORT_NR + 1)));
}

/*============================================================================*
 * mailbox_get_portid()                                                       *
 *============================================================================*/

/**
 * @brief Gets port id.
 *
 * @param id Composed ID.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PRIVATE int mailbox_get_portid(int id)
{
	return ((id < 0) ? (id) : (id % (MAILBOX_PORT_NR + 1)));
}

/*============================================================================*
 * mailbox_laddress_calc()                                                    *
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
PUBLIC int mailbox_laddress_calc(int fd, int port)
{
	return (ACTIVE_LADDRESS_COMPOSE(fd, port, MAILBOX_PORT_NR));
}

/*============================================================================*
 * do_mailbox_alloc()                                                         *
 *============================================================================*/

/**
 * @brief Allocate a physical mailbox.
 *
 * @param local  Local node ID.
 * @param remote Remote node ID (It can be -1).
 * @param port   Port ID.
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the ID of the active mailbox is
 * returned. Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_mailbox_alloc(int local, int remote, int port, int type)
{
	return (active_alloc(&mbxpool, local, remote, port, type));
}

/*============================================================================*
 * do_mailbox_release()                                                       *
 *============================================================================*/

/**
 * @brief Releases a physical mailbox.
 *
 * @param mbxid Active mailbox ID.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_mailbox_release(int mbxid)
{
	return (active_release(&mbxpool, mbxid));
}

/*============================================================================*
 * do_mailbox_aread()                                                         *
 *============================================================================*/

/**
 * @brief Async reads from an active.
 *
 * @param mbxid  Active mailbox ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t do_mailbox_aread(int mbxid, const struct active_config * config, struct pstats * stats)
{
	return (active_aread(&mbxpool, mbxid, config, stats));
}

/*============================================================================*
 * do_mailbox_awrite()                                                        *
 *============================================================================*/

/**
 * @brief Async writes from an active.
 *
 * @param mbxid  Active mailbox ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC ssize_t do_mailbox_awrite(int mbxid, const struct active_config * config, struct pstats * stats)
{
	return (active_awrite(&mbxpool, mbxid, config, stats));
}

/*============================================================================*
 * do_mailbox_wait()                                                          *
 *============================================================================*/

/**
 * @brief Waits on a mailbox to finish an assynchronous operation.
 *
 * @param mbxid  Active mailbox ID.
 * @param config Communication's configuration.
 * @param stats  Structure to store statstics.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_mailbox_wait(int mbxid, const struct active_config * config, struct pstats * stats)
{
	return (active_wait(&mbxpool, mbxid, config, stats));
}

/*============================================================================*
 * do_mailbox_init()                                                          *
 *============================================================================*/

/**
 * @todo Initializtion of the active mailboxes and structures.
 */
PUBLIC void do_mailbox_init(void)
{
	int local;

	local = processor_node_get_num();

	KASSERT((((unsigned long) &mbuffers[0].message) & 0x7) == 0);
	KASSERT((((unsigned long) &mbuffers[1].message) & 0x7) == 0);
	KASSERT((sizeof(mbuffers[0].message) % 8) == 0);
	KASSERT((sizeof(mbuffers[1].message) % 8) == 0);

	/* Initializes the mailboxes structures. */
	do_mailbox_table_init();

	/* Create the input mailbox. */
	KASSERT(_active_create(&mbxpool, local) >= 0);

	/* Opens all mailbox interfaces. */
	for (int i = 0; i < PROCESSOR_NOC_NODES_NUM; ++i)
		KASSERT(_active_open(&mbxpool, local, i) >= 0);

}

#endif /* __TARGET_HAS_MAILBOX */

