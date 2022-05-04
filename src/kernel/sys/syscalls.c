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

#include <nanvix/const.h>
#include <nanvix/hal.h>
#include <nanvix/kernel/thread.h>
#include <nanvix/kernel/syscall.h>
#include <nanvix/hlib.h>
#include <posix/errno.h>

/**
 * @brief Semaphore variable for system call dispatcher.
 */
PRIVATE struct semaphore syssem;

/**
 * @brief System call scoreboard.
 */
PRIVATE struct sysboard
{
	word_t arg0;             /**< First argument of system call.           */
	word_t arg1;             /**< Second argument of system call.          */
	word_t arg2;             /**< Third argument of system call.           */
	word_t arg3;             /**< Fourth argument of system call.          */
	word_t arg4;             /**< Fifth argument of system call.           */
	word_t syscall_nr;       /**< System call number.                      */
	word_t ret;              /**< Return value of system call.             */
	struct semaphore syssem; /**< Semaphore to wait a syscall to complete. */
	struct mutex sysmutex;   /**< Reserve the sysboard by a thread.        */
	int pending;
} ALIGN(CACHE_LINE_SIZE) sysboard[CORES_NUM];

/**
 * @brief Next core that may request an operation.
 */
static int sys_next_coreid = 0;

/**
 * @brief Handles a system call IPI.
 */
PUBLIC void do_kcall2(void)
{
	int coreid = 0;
	int ret = -ENOSYS;

	semaphore_down(&syssem);

		/* Sweeps the cores in a circular fashion. */
		for (int i = 0; i < CORES_NUM; i++)
		{
			if (sysboard[sys_next_coreid].pending)
			{
				coreid = sys_next_coreid;
				sys_next_coreid = (sys_next_coreid + 1) % CORES_NUM;

				break;
			}

			sys_next_coreid = (sys_next_coreid + 1) % CORES_NUM;
		}

		/* Invalid system call number. */
		if (sysboard[coreid].syscall_nr >= NR_SYSCALLS)
		{
			ret = -EINVAL;
			goto out;
		}

		/* Parse system call number. */
		switch (sysboard[coreid].syscall_nr)
		{
			case NR_shutdown:
				ret = kernel_shutdown();
				break;

			case NR__exit:
				kernel_exit((int)(long) sysboard[coreid].arg0);
				break;

			case NR_write:
				ret = kernel_write(
					(int)(long) sysboard[coreid].arg0,
					(const char *)(long) sysboard[coreid].arg1,
					(size_t)(long) sysboard[coreid].arg2
				);
				break;

#if (THREAD_MAX > 1)

			case NR_thread_create:
				ret = kernel_thread_create(
					(int *)(long) sysboard[coreid].arg0,
					(void *(*)(void *))(long) sysboard[coreid].arg1,
					(void *)(long) sysboard[coreid].arg2
				);
				break;

			case NR_thread_stats:
				ret = kernel_thread_stats(
					(int)(long) sysboard[coreid].arg0,
					(uint64_t *)(long) sysboard[coreid].arg1,
					(int)(long) sysboard[coreid].arg2
				);
				break;

			case NR_wakeup:
				ret = kernel_wakeup(
					(int)(long) sysboard[coreid].arg0
				);
				break;

			case NR_thread_key_create:
				ret = kernel_thread_key_create(
					(int *)(long) sysboard[coreid].arg0,
					(void(*)(void *))(long) sysboard[coreid].arg1
				);
				break;

			case NR_thread_key_delete:
				ret = kernel_thread_key_delete(
					(int)(long) sysboard[coreid].arg0
				);
				break;

			case NR_thread_getspecific:
				ret = (word_t)kernel_thread_getspecific(
					(int)(long) sysboard[coreid].arg0,
					(int)(long) sysboard[coreid].arg1,
					(void **)(long) sysboard[coreid].arg2
				);
				break;

			case NR_thread_setspecific:
				ret = kernel_thread_setspecific(
					(int)(long) sysboard[coreid].arg0,
					(int)(long) sysboard[coreid].arg1,
					(void *)(long) sysboard[coreid].arg2
				);
				break;
			
			case NR_thread_key_exit:
				ret = kernel_thread_key_exit(
					(int)(long) sysboard[coreid].arg0,
					(int *)(long) sysboard[coreid].arg1
				);
				break;

#endif

			case NR_sigctl:
				ret = kernel_sigctl(
					(int)(long) sysboard[coreid].arg0,
					(struct ksigaction *)(long) sysboard[coreid].arg1
				);
				break;

			case NR_node_get_num:
				ret = kernel_node_get_num();
				break;

			case NR_cluster_get_num:
				ret = kernel_cluster_get_num();
				break;

#if __TARGET_HAS_SYNC
			case NR_sync_create:
				ret = kernel_sync_create(
					(const int *)(long) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1,
					(int) sysboard[coreid].arg2
				);
				break;

			case NR_sync_open:
				ret = kernel_sync_open(
					(const int *)(long) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1,
					(int) sysboard[coreid].arg2
				);
				break;

			case NR_sync_unlink:
				ret = kernel_sync_unlink(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_sync_close:
				ret = kernel_sync_close(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_sync_signal:
				ret = kernel_sync_signal(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_sync_ioctl:
				ret = kernel_sync_ioctl(
					(int) sysboard[coreid].arg0,
					(unsigned) sysboard[coreid].arg1,
					(va_list *) sysboard[coreid].arg2
				);
				break;
#endif /* __TARGET_HAS_SYNC */

#if __TARGET_HAS_MAILBOX
			case NR_mailbox_create:
				ret = kernel_mailbox_create(
					(int) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1
				);
				break;

			case NR_mailbox_open:
				ret = kernel_mailbox_open(
					(int) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1
				);
				break;

			case NR_mailbox_unlink:
				ret = kernel_mailbox_unlink(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_mailbox_close:
				ret = kernel_mailbox_close(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_mailbox_aread:
				ret = kernel_mailbox_aread(
					(int) sysboard[coreid].arg0,
					(void *)(long) sysboard[coreid].arg1,
					(size_t) sysboard[coreid].arg2
				);
				break;

			case NR_mailbox_awrite:
				ret = kernel_mailbox_awrite(
					(int) sysboard[coreid].arg0,
					(const void *)(long) sysboard[coreid].arg1,
					(size_t) sysboard[coreid].arg2
				);
				break;

			case NR_mailbox_ioctl:
				ret = kernel_mailbox_ioctl(
					(int) sysboard[coreid].arg0,
					(unsigned) sysboard[coreid].arg1,
					(va_list *) sysboard[coreid].arg2
				);
				break;
#endif /* __TARGET_HAS_MAILBOX */

#if __TARGET_HAS_PORTAL
			case NR_portal_create:
				ret = kernel_portal_create(
					(int) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1
				);
				break;

			case NR_portal_allow:
				ret = kernel_portal_allow(
					(int) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1,
					(int) sysboard[coreid].arg2
				);
				break;

			case NR_portal_open:
				ret = kernel_portal_open(
					(int) sysboard[coreid].arg0,
					(int) sysboard[coreid].arg1,
					(int) sysboard[coreid].arg2
				);
				break;

			case NR_portal_unlink:
				ret = kernel_portal_unlink(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_portal_close:
				ret = kernel_portal_close(
					(int) sysboard[coreid].arg0
				);
				break;

			case NR_portal_aread:
				ret = kernel_portal_aread(
					(int) sysboard[coreid].arg0,
					(void *)(long) sysboard[coreid].arg1,
					(size_t) sysboard[coreid].arg2
				);
				break;

			case NR_portal_awrite:
				ret = kernel_portal_awrite(
					(int) sysboard[coreid].arg0,
					(const void *)(long) sysboard[coreid].arg1,
					(size_t) sysboard[coreid].arg2
				);
				break;

			case NR_portal_ioctl:
				ret = kernel_portal_ioctl(
					(int) sysboard[coreid].arg0,
					(unsigned) sysboard[coreid].arg1,
					(va_list *) sysboard[coreid].arg2
				);
				break;
#endif /* __TARGET_HAS_PORTAL */

		case NR_stats:
			kernel_stats(
				(uint64_t *) sysboard[coreid].arg0,
				(int) sysboard[coreid].arg1
			);
			break;

		case NR_frame_alloc:
			ret = kernel_frame_alloc();
			break;

		case NR_frame_free:
			ret = kernel_frame_free(
				(vaddr_t) sysboard[coreid].arg0
			);
			break;

		case NR_upage_alloc:
			ret = kernel_upage_alloc(
				(vaddr_t) sysboard[coreid].arg0
			);
			break;

		case NR_upage_free:
			ret = kernel_upage_free(
				(vaddr_t) sysboard[coreid].arg0
			);
			break;

		case NR_upage_map:
			ret = kernel_upage_map(
				(vaddr_t) sysboard[coreid].arg0,
				(frame_t) sysboard[coreid].arg1
			);
			break;

		case NR_upage_unmap:
			ret = kernel_upage_unmap(
				(vaddr_t) sysboard[coreid].arg0
			);
			break;

		case NR_upage_link:
			ret = kernel_upage_link(
				(vaddr_t) sysboard[coreid].arg0,
				(vaddr_t) sysboard[coreid].arg1
			);
			break;

#if (THREAD_MAX > 1)

		case NR_excp_ctrl:
			ret = kernel_excp_ctrl(
				(int) sysboard[coreid].arg0,
				(int) sysboard[coreid].arg1
			);
			break;

		case NR_excp_resume:
			ret = kernel_excp_resume();
			break;

#endif

			default:
				break;
		}

	out:

		sysboard[coreid].pending = 0;
		sysboard[coreid].ret = ret;

	semaphore_up(&sysboard[coreid].syssem);
}

/**
 * @brief System call dispatcher.
 *
 * @param arg0       First system call argument.
 * @param arg1       Second system call argument.
 * @param arg2       Third system call argument.
 * @param arg3       Fourth system call argument.
 * @param arg4       Fifth system call argument.
 * @param arg5       Sixth system call argument.
 * @param arg6       Seventh system call argument.
 * @param syscall_nr System call number.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
PUBLIC int do_kcall(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4,
	word_t syscall_nr)
{
	int coreid;
	int affinity;
	int ret = -EINVAL;

	coreid = core_get_id();

	UNUSED(affinity);

	/* Parse system call number. */
	switch (syscall_nr)
	{
		case NR_thread_get_id:
			ret = kernel_thread_get_id();
			break;

#if (THREAD_MAX > 1)

		case NR_thread_set_affinity:
		{
			affinity = (int) arg0;

			if (KTHREAD_AFFINITY_IS_VALID(affinity))
			{
				ret = thread_set_curr_affinity(affinity);

				/* Lose affinity? */
				if (KTHREAD_AFFINITY_MATCH(affinity, (1 << coreid)) == 0)
					kernel_thread_yield();
			}
		} break;

		case NR_thread_exit:
			kernel_thread_exit((void *)(long) arg0);
			break;

		case NR_thread_join:
			ret = kernel_thread_join(
				(int) arg0,
				(void **)(long) arg1
			);
			break;

		case NR_thread_yield:
			ret = kernel_thread_yield();
			break;

		case NR_sleep:
			ret = kernel_sleep();
			break;

#endif

		case NR_alarm:
			ret = kernel_alarm((int) arg0);
			break;

		case NR_sigsend:
			ret = kernel_sigsend(
				(int) arg0,
				(int) arg1
			);
			break;

		case NR_sigwait:
			ret = kernel_sigwait((int) arg0);
			break;

		case NR_sigreturn:
			kernel_sigreturn();
			ret = 0;
			break;

		case NR_comm_get_port:
			ret = kernel_comm_get_port(
				(int) arg0,
				(int) arg1
			);
			break;

#if __TARGET_HAS_SYNC
		case NR_sync_wait:
			ret = kernel_sync_wait(
				(int) arg0
			);
			break;
#endif

#if __TARGET_HAS_MAILBOX
		case NR_mailbox_wait:
			ret = kernel_mailbox_wait(
				(int) arg0
			);
			break;
#endif

#if __TARGET_HAS_PORTAL
		case NR_portal_wait:
			ret = kernel_portal_wait(
				(int) arg0
			);
			break;
#endif

		case NR_clock:
			ret = kernel_clock(
				(uint64_t *) arg0
			);
			break;

#if (THREAD_MAX > 1)

		case NR_excp_pause:
			ret = kernel_excp_pause(
				(struct exception *) arg0
			);
			break;

#endif

		/* Forward system call. */
		default:
		{
			/* Reserve sysboard. */
			mutex_lock(&sysboard[coreid].sysmutex);

				/* Fillup system call board. */
				sysboard[coreid].arg0 = arg0;
				sysboard[coreid].arg1 = arg1;
				sysboard[coreid].arg2 = arg2;
				sysboard[coreid].arg3 = arg3;
				sysboard[coreid].arg4 = arg4;
				sysboard[coreid].syscall_nr = syscall_nr;
				sysboard[coreid].pending = 1;

				semaphore_up(&syssem);
				semaphore_down(&sysboard[coreid].syssem);

				ret = sysboard[coreid].ret;

				switch (syscall_nr)
				{
					case NR_upage_alloc:
					case NR_upage_free:
					case NR_upage_map:
					case NR_upage_unmap:
					case NR_upage_link:
						upage_inval(arg0);

					default:
						break;
				}

			/* Release sysboard. */
			mutex_unlock(&sysboard[coreid].sysmutex);
		} break;
	}

	return (ret);
}

/**
 * @brief Initializes the Syscall system.
 */
PUBLIC void syscall_init(void)
{
	semaphore_init(&syssem, 0);

	for (unsigned coreid = 0; coreid < CORES_NUM; ++coreid)
	{
		sysboard[coreid].syscall_nr = -1;
		sysboard[coreid].pending    =  0;
		semaphore_init(&sysboard[coreid].syssem, 0);
		mutex_init(&sysboard[coreid].sysmutex);
	}
}
