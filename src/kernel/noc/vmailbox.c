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
#include "mailbox.h"

#if __TARGET_HAS_MAILBOX

/**
 * @brief Extracts fd and port from mbxid.
 */
/**@{*/
#define VMAILBOX_GET_LADDRESS_PORT(mbxid) (mbxid % (MAILBOX_PORT_NR + 1))
/**@}*/

/*============================================================================*
 * Virtual mailbox structure                                                  *
 *============================================================================*/

/**
 * @name Virtual mailbox resource.
 */
/**@{*/
PRIVATE struct communicator_counters vmailbox_counters;                      /**< Virtual mailbox counters. */
PRIVATE struct communicator ALIGN(sizeof(dword_t)) vmailboxes[KMAILBOX_MAX]; /**< Virtual mailbox talbe.    */
PRIVATE struct communicator_functions vmailbox_functions;                    /**< Mailbox Functions.        */
PRIVATE struct communicator_pool vmbxpool;                                   /**< Virtual mailbox pool.     */
/**@}*/

/*============================================================================*
 * do_vmailbox_init()                                                        *
 *============================================================================*/

/**
 * @brief Initializes the mbuffers table lock.
 */
PRIVATE void do_vmailbox_init(void)
{
	spinlock_init(&vmailbox_counters.lock);
	vmailbox_counters.ncreates = 0ULL;
	vmailbox_counters.nunlinks = 0ULL;
	vmailbox_counters.nopens   = 0ULL;
	vmailbox_counters.ncloses  = 0ULL;
	vmailbox_counters.nreads   = 0ULL;
	vmailbox_counters.nwrites  = 0ULL;

	vmailbox_functions.do_release    = do_mailbox_release;
	vmailbox_functions.do_comm       = do_mailbox_aread;
	vmailbox_functions.do_wait       = do_mailbox_wait;
	vmailbox_functions.laddress_calc = mailbox_laddress_calc;

	for (int i = 0; i < KMAILBOX_MAX; ++i)
	{
		spinlock_init(&vmailboxes[i].lock);
		vmailboxes[i].resource   = RESOURCE_INITIALIZER;
		vmailboxes[i].config     = ACTIVE_CONFIG_INITIALIZER;
		vmailboxes[i].stats      = PSTATS_INITIALIZER;
		vmailboxes[i].fn         = &vmailbox_functions;
		vmailboxes[i].counters   = &vmailbox_counters;
	}

	vmbxpool.communicators  = vmailboxes;
	vmbxpool.ncommunicators = KMAILBOX_MAX;
}

/*============================================================================*
 * do_vmailbox_alloc()                                                        *
 *============================================================================*/

/**
 * @brief Allocate a virtual mailbox.
 *
 * @param local  Local node ID.
 * @param remote Remote node ID (It can be -1).
 * @param port   Port ID.
 * @param type   Communication type (INPUT or OUTPUT).
 *
 * @returns Upon successful completion, the index of the virtual
 * mailbox in vmailboxes tab is returned. Upon failure,
 * a negative number is returned instead.
 */
PRIVATE int do_vmailbox_alloc(int local, int remote, int port, int type)
{
	int fd;    /* Active mailbox logic ID. */
	int mbxid; /* Virtual mailbox ID.      */
	struct active_config config;

	/* Allocates a physical mailbox port. */
	if ((fd = do_mailbox_alloc(local, remote, port, type)) < 0)
		return (fd);

	config.fd          = fd;
	config.local_addr  = ACTIVE_LADDRESS_COMPOSE(local, VMAILBOX_GET_LADDRESS_PORT(fd), MAILBOX_PORT_NR);
	config.remote_addr = (type == ACTIVE_TYPE_OUTPUT) ?
		ACTIVE_LADDRESS_COMPOSE(remote, port, MAILBOX_PORT_NR) :
		(-1);

	/* Allocates a communicator. */
	if ((mbxid = communicator_alloc(&vmbxpool, &config, type)) < 0)
	{
		if (do_mailbox_release(fd) < 0)
			kpanic("[mailbox] Failed on release a mailbox port!");
	}
	else
	{
		spinlock_lock(&vmailbox_counters.lock);
			if (type == ACTIVE_TYPE_INPUT)
				vmailbox_counters.ncreates++;
			else
				vmailbox_counters.nopens++;
		spinlock_unlock(&vmailbox_counters.lock);
	}

	return (mbxid);
}

/*============================================================================*
 * do_vmailbox_create()                                                       *
 *============================================================================*/

/**
 * @brief Creates a virtual mailbox.
 *
 * @param local Logic ID of the target local node.
 * @param port  Target port in @p local node.
 *
 * @returns Upon successful completion, the ID of the newly created
 * virtual mailbox is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int do_vmailbox_create(int local, int port)
{
	/* Checks if the input mailbox is local. */
	if (!node_is_local(local))
		return (-EINVAL);

	return (
		do_vmailbox_alloc(
			local,
			-1,
			port,
			ACTIVE_TYPE_INPUT
		)
	);
}

/*============================================================================*
 * do_vmailbox_open()                                                         *
 *============================================================================*/

/**
 * @brief Opens a virtual mailbox.
 *
 * @param remote      Logic ID of the target remote node.
 * @param remote_port Target port in @p remote node.
 *
 * @returns Upon successful completion, the ID of the newly opened
 * virtual mailbox is returned. Upon failure, a negative error code
 * is returned instead.
 */
PUBLIC int do_vmailbox_open(int remote, int remote_port)
{
	return (
		do_vmailbox_alloc(
			processor_node_get_num(),
			remote,
			remote_port,
			ACTIVE_TYPE_OUTPUT
		)
	);
}

/*============================================================================*
 * do_vmailbox_unlink()                                                       *
 *============================================================================*/

/**
 * @brief Unlinks a created virtual mailbox.
 *
 * @param mbxid Logic ID of the target virtual mailbox.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vmailbox_unlink(int mbxid)
{
	return (communicator_free(&vmbxpool, mbxid, ACTIVE_TYPE_INPUT));
}

/*============================================================================*
 * do_vmailbox_close()                                                        *
 *============================================================================*/

/**
 * @brief Closes an opened virtual mailbox.
 *
 * @param mbxid Logic ID of the target virtual mailbox.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vmailbox_close(int mbxid)
{
	return (communicator_free(&vmbxpool, mbxid, ACTIVE_TYPE_OUTPUT));
}

/*============================================================================*
 * do_vmailbox_aread()                                                        *
 *============================================================================*/

/**
 * @brief Async reads from an virtual mailbox.
 *
 * @param mbxid  Virtual mailbox ID.
 * @param buffer User buffer.
 * @param size   Size of the buffer.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vmailbox_aread(int mbxid, void * buffer, size_t size)
{
	vmailboxes[mbxid].config.buffer = buffer;
	vmailboxes[mbxid].config.size   = size;
	vmailbox_functions.do_comm      = do_mailbox_aread;

	/* Dummy allow for mailbox. */
	spinlock_lock(&vmailboxes[mbxid].lock);
		communicator_set_allowed(&vmailboxes[mbxid]);
	spinlock_unlock(&vmailboxes[mbxid].lock);

	return (communicator_operate(&vmailboxes[mbxid], ACTIVE_TYPE_INPUT));
}

/*============================================================================*
 * do_vmailbox_awrite()                                                       *
 *============================================================================*/

/**
 * @brief Async write from an virtual mailbox.
 *
 * @param mbxid  Virtual mailbox ID.
 * @param buffer User buffer.
 * @param size   Size of the buffer.
 *
 * @returns Upon successful completion, positive number is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_vmailbox_awrite(int mbxid, const void * buffer, size_t size)
{
	vmailboxes[mbxid].config.buffer = buffer;
	vmailboxes[mbxid].config.size   = size;
	vmailbox_functions.do_comm      = do_mailbox_awrite;

	return (communicator_operate(&vmailboxes[mbxid], ACTIVE_TYPE_OUTPUT));
}

/*============================================================================*
 * do_vmailbox_wait()                                                         *
 *============================================================================*/

/**
 * @brief Waits on a virtual mailbox to finish an assynchronous operation.
 *
 * @param mbxid Logic ID of the target virtual mailbox.
 *
 * @returns Upon successful completion, a positive number is returned.
 * Upon failure, a negative error code is returned instead.
 */
PUBLIC int do_vmailbox_wait(int mbxid)
{
	int ret; /* Return value. */

	/* Wait for the operation to complete. */
	if ((ret = communicator_wait(&vmailboxes[mbxid])) == ACTIVE_COMM_SUCCESS)
	{
		/* Reset remote. */
		if (resource_is_readable(&vmailboxes[mbxid].resource))
			vmailboxes[mbxid].config.remote_addr = (-1);
	}

	return (ret);
}

/*============================================================================*
 * do_vmailbox_ioctl()                                                        *
 *============================================================================*/

/**
 * @brief Request an I/O operation on a virtual mailbox.
 *
 * @param mbxid   Virtual mailbox ID.
 * @param request Type of request.
 * @param args    Arguments of the request.
 *
 * @returns Upon successful completion, zero is returned.
 * Upon failure, a negative error code is returned instead.
 */
int do_vmailbox_ioctl(int mbxid, unsigned request, va_list args)
{
	return (communicator_ioctl(&vmailboxes[mbxid], request, args));
}

/*============================================================================*
 * do_vmailbox_get_port()                                                     *
 *============================================================================*/

/**
 * @brief Gets a releated port id to a virtual mailbox.
 *
 * @param mbxid Logic ID of the target virtual mailbox.
 *
 * @returns Upon successful completion, a positive number is returned.
 * Upon failure, a negative error code is returned instead.
 */
int do_vmailbox_get_port(int mbxid)
{
	int ret;

	if (!WITHIN(mbxid, 0, KMAILBOX_MAX))
		return (-EINVAL);

	spinlock_lock(&vmailboxes[mbxid].lock);

		if (!resource_is_used(&vmailboxes[mbxid].resource))
			ret = (-EBADF);
		else
			ret = (VMAILBOX_GET_LADDRESS_PORT(vmailboxes[mbxid].config.fd));

	spinlock_unlock(&vmailboxes[mbxid].lock);

	return (ret);
}

/*============================================================================*
 * vmailbox_init()                                                            *
 *============================================================================*/

/**
 * @brief Initializes the mailbox service.
 */
PUBLIC void vmailbox_init(void)
{
	kprintf("[kernel][noc] initializing the kmailbox facility");

	/* Initializes the hardware mailboxes structures. */
	do_mailbox_init();

	/* Initializes the virtual mailboxes structures. */
	do_vmailbox_init();
}

#endif /* __TARGET_HAS_MAILBOX */

