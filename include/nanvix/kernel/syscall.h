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

#ifndef NANVIX_SYSCALL_H_
#define NANVIX_SYSCALL_H_

	#include <nanvix/kernel/mm.h>
#ifdef __NANVIX_MICROKERNEL
	#include <nanvix/kernel/excp.h>
	#include <nanvix/kernel/thread.h>
	#include <nanvix/kernel/sync.h>
	#include <nanvix/kernel/mailbox.h>
	#include <nanvix/kernel/portal.h>
	#include <nanvix/kernel/signal.h>
	#include <nanvix/const.h>
#endif /* __NANVIX_MICROKERNEL */

/**
 * @addtogroup kernel-syscalls System Calls
 * @ingroup kernel
 */
/**@{*/

	/**
	 * @brief Number of system calls.
	 *
	 * @note This should be set to the highest system call number.
	 */
	#define NR_SYSCALLS NR_last_kcall

	/**
	 * @name System Call Numbers
	 */
	/**@{*/
	#define NR__exit                1 /**< kernel_exit()                */
	#define NR_write                2 /**< kernel_write()               */
	#define NR_thread_get_id        3 /**< kernel_thread_get_id()       */
	#define NR_thread_create        4 /**< kernel_thread_create()       */
	#define NR_thread_exit          5 /**< kernel_thread_exit()         */
	#define NR_thread_join          6 /**< kernel_thread_join()         */
	#define NR_thread_yield         7 /**< kernel_thread_yield()        */
	#define NR_thread_set_affinity  8 /**< kernel_thread_set_affinity() */
	#define NR_thread_stats         9 /**< kernel_thread_stats()        */
	#define NR_sleep               10 /**< kernel_sleep()               */
	#define NR_wakeup              11 /**< kernel_wakeup()              */
	#define NR_shutdown            12 /**< kernel_shutdown()            */
	#define NR_sigctl              13 /**< kernel_sigctl()              */
	#define NR_alarm               14 /**< kernel_alarm()               */
	#define NR_sigsend             15 /**< kernel_sigsend()             */
	#define NR_sigwait             16 /**< kernel_sigwait()             */
	#define NR_sigreturn           17 /**< kernel_sigreturn()           */
	#define NR_node_get_num        18 /**< kernel_node_get_num()        */
	#define NR_sync_create         19 /**< kernel_sync_create()         */
	#define NR_sync_open           20 /**< kernel_sync_open()           */
	#define NR_sync_unlink         21 /**< kernel_sync_unlink()         */
	#define NR_sync_close          22 /**< kernel_sync_close()          */
	#define NR_sync_wait           23 /**< kernel_sync_wait()           */
	#define NR_sync_signal         24 /**< kernel_sync_signal()         */
	#define NR_sync_ioctl          25 /**< kernel_sync_ioctl()          */
	#define NR_mailbox_create      26 /**< kernel_mailbox_create()      */
	#define NR_mailbox_open        27 /**< kernel_mailbox_open()        */
	#define NR_mailbox_unlink      28 /**< kernel_mailbox_unlink()      */
	#define NR_mailbox_close       29 /**< kernel_mailbox_close()       */
	#define NR_mailbox_awrite      30 /**< kernel_mailbox_awrite()      */
	#define NR_mailbox_aread       31 /**< kernel_mailbox_aread()       */
	#define NR_mailbox_wait        32 /**< kernel_mailbox_wait()        */
	#define NR_mailbox_ioctl       33 /**< kernel_mailbox_ioctl()       */
	#define NR_portal_create       34 /**< kernel_portal_create()       */
	#define NR_portal_allow        35 /**< kernel_portal_allow()        */
	#define NR_portal_open         36 /**< kernel_portal_open()         */
	#define NR_portal_unlink       37 /**< kernel_portal_unlink()       */
	#define NR_portal_close        38 /**< kernel_portal_close()        */
	#define NR_portal_awrite       39 /**< kernel_portal_awrite()       */
	#define NR_portal_aread        40 /**< kernel_portal_aread()        */
	#define NR_portal_wait         42 /**< kernel_portal_wait()         */
	#define NR_portal_ioctl        43 /**< kernel_portal_ioctl()        */
	#define NR_clock               44 /**< kernel_clock()               */
	#define NR_stats               45 /**< kernel_stats()               */
	#define NR_frame_alloc         46 /**< kernel_frame_alloc()         */
	#define NR_frame_free          47 /**< kernel_frame_free()          */
	#define NR_upage_alloc         48 /**< kernel_upage_alloc()         */
	#define NR_upage_free          49 /**< kernel_upage_free()          */
	#define NR_upage_map           50 /**< kernel_upage_map()           */
	#define NR_upage_link          51 /**< kernel_upage_link()          */
	#define NR_upage_unlink        52 /**< kernel_upage_unlink()        */
	#define NR_upage_unmap         53 /**< kernel_upage_unmap()         */
	#define NR_excp_ctrl           54 /**< kernel_excp_ctrl()           */
	#define NR_excp_pause          55 /**< kernel_excp_pause()          */
	#define NR_excp_resume         56 /**< kernel_excp_resume()         */
	#define NR_cluster_get_num     57 /**< kernel_cluster_get_num()     */
	#define NR_comm_get_port       58 /**< kernel_comm_get_port()       */
	#define NR_thread_key_create   59 /**< kernel_thread_key_create()   */
	#define NR_thread_key_delete   60 /**< kernel_thread_key_delete()   */
	#define NR_thread_getspecific  61 /**< kernel_thread_getspecific()  */
	#define NR_thread_setspecific  62 /**< kernel_thread_setspecific()  */

	#define NR_last_kcall          63 /**< NR_SYSCALLS definer          */
	/**@}*/

/*============================================================================*
 * Thread Kernel Calls                                                        *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	EXTERN void kernel_exit(int);
	EXTERN ssize_t kernel_write(int, const char *, size_t);
	EXTERN int kernel_thread_get_id(void);
	EXTERN int kernel_thread_create(int *, void*(*)(void*), void *);
	EXTERN void kernel_thread_exit(void *);
	EXTERN int kernel_thread_join(int, void **);
	EXTERN int kernel_sleep(void);
	EXTERN int kernel_wakeup(int);
	EXTERN int kernel_thread_yield(void);
	EXTERN int kernel_thread_stats(int, uint64_t *, int);
	EXTERN int kernel_thread_key_create(int *, void (*)(void *));
	EXTERN int kernel_thread_key_delete(int);
	EXTERN int kernel_thread_getspecific(int, int, void **);
	EXTERN int kernel_thread_setspecific(int, int, void *);

	/**
	 * @brief Shutdowns the kernel.
	 *
	 * @returns Upon successful completion, this function does not
	 * return.Upon failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_shutdown(void);

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Thread Kernel Calls                                                        *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Controls the behavior of a signal.
	 *
	 * @param signum Signal ID.
	 * @param sigact Behavior descriptor.
	 *
	 * @returns Zero if successfully changes the behavior, non zero otherwise.
	 */
	EXTERN int kernel_sigctl(int signum, struct ksigaction *sigact);

	/**
	 * @brief Schedules an alarm signal.
	 *
	 * @param seconds Time in seconds.
	 *
	 * @returns Zero if successfully register the alarm, non zero otherwise.
	 */
	EXTERN int kernel_alarm(int seconds);

	/**
	 * @brief Sends a signal.
	 *
	 * @param signum Signal ID.
	 * @param tid    Thread ID.
	 *
	 * @returns Zero if successfully sends the signal, non zero otherwise.
	 */
	EXTERN int kernel_sigsend(int signum, int tid);

	/**
	 * @brief Waits for the receipt of a signal.
	 *
	 * @param signum Signal ID.
	 *
	 * @returns Zero if successfully receives the signal, non zero otherwise.
	 */
	EXTERN int kernel_sigwait(int signum);

	/**
	 * @brief Returns from a signal handler.
	 */
	EXTERN void kernel_sigreturn(void);

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Memory Management Kernel Calls                                             *
 *============================================================================*/

	/**
	 * @brief Allocates a page frame.
	 *
	 * @returns Upon successful completion, the number of the
	 * allocated page frame is returned. Upon failure, @p FRAME_NULL
	 * is returned instead.
	 */
	EXTERN frame_t kernel_frame_alloc(void);

	/**
	 * @brief Frees a page frame.
	 *
	 * @param frame Number of the target page frame.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_frame_free(frame_t frame);

	/**
	 * @brief Allocates a user page.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_free().
	 */
	EXTERN int kernel_upage_alloc(vaddr_t vaddr);

	/**
	 * @brief Releases a user page.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_alloc().
	 */
	EXTERN int kernel_upage_free(vaddr_t vaddr);

	/**
	 * @brief Maps a page frame into a page.
	 *
	 * @param vaddr Target virtual address.
	 * @param frame Target page frame.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_upage_map(vaddr_t vaddr, frame_t frame);

	/**
	 * @brief Unmaps a page frame.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_map().
	 */
	EXTERN int kernel_upage_unmap(vaddr_t vaddr);

	/**
	 * @brief Links two pages.
	 *
	 * @param vaddr Virtual address of source page.
	 * @param vaddr Virtual address of target page.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_upage_link(vaddr_t vaddr1, vaddr_t vaddr2);

/*============================================================================*
 * NoC Kernel Calls                                                           *
 *============================================================================*/

	EXTERN int kernel_node_get_num(void);
	EXTERN int kernel_cluster_get_num(void);
	EXTERN int kernel_comm_get_port(int, int);

/*============================================================================*
 * Sync Kernel Calls                                                          *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	EXTERN int kernel_sync_create(const int *, int, int);
	EXTERN int kernel_sync_open(const int *, int, int);
	EXTERN int kernel_sync_unlink(int);
	EXTERN int kernel_sync_close(int);
	EXTERN int kernel_sync_wait(int);
	EXTERN int kernel_sync_signal(int);
	EXTERN int kernel_sync_ioctl(int, unsigned, va_list *);

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Mailbox Kernel Calls                                                       *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	EXTERN int kernel_mailbox_create(int, int);
	EXTERN int kernel_mailbox_open(int, int);
	EXTERN int kernel_mailbox_unlink(int);
	EXTERN int kernel_mailbox_close(int);
	EXTERN int kernel_mailbox_aread(int, void *, size_t);
	EXTERN int kernel_mailbox_awrite(int, const void *, size_t);
	EXTERN int kernel_mailbox_wait(int);
	EXTERN int kernel_mailbox_ioctl(int, unsigned, va_list *);

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Portal Kernel Calls                                                        *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	EXTERN int kernel_portal_create(int, int);
	EXTERN int kernel_portal_allow(int, int, int);
	EXTERN int kernel_portal_open(int, int, int);
	EXTERN int kernel_portal_unlink(int);
	EXTERN int kernel_portal_close(int);
	EXTERN int kernel_portal_aread(int, void *, size_t);
	EXTERN int kernel_portal_awrite(int, const void *, size_t);
	EXTERN int kernel_portal_wait(int);
	EXTERN int kernel_portal_ioctl(int, unsigned, va_list *);

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Miscellaneous Kernel Calls                                                 *
 *============================================================================*/

	/**
	 * @brief Gets the kernel wall clock.
	 *
	 * @param buffer Store location for wall clock.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_clock(uint64_t *buffer);

	/**
	 * @brief Gets performance statistics of the kernel.
	 *
	 * @param buffer Store location for the statistics.
	 * @param perf   Performance counter to watch.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kernel_stats(uint64_t *buffer, int perf);

/*============================================================================*
 * User-Space Exception Kernel Calls                                          *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

#if (THREAD_MAX > 1)

	/**
	 * @brief Sets a user-space exception handler.
	 *
	 * @param excpnum Number of the target exception.
	 * @param action  Action upon target exception.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	EXTERN int kernel_excp_ctrl(int excpnum, int action);

	/**
	 * @brief Pauses the user-space exception handler.
	 *
	 * @param excp Information about the exception
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	EXTERN int kernel_excp_pause(struct exception *excp);

	/**
	 * @brief Resumes a kernel-space exception handler.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	EXTERN int kernel_excp_resume(void);

#endif /* (THREAD_MAX > 1) */

#endif /* __NANVIX_MICROKERNEL */

/*============================================================================*
 * Initialization Calls                                                       *
 *============================================================================*/

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Initializes the Syscall system.
	 */
	EXTERN void syscall_init(void);

#endif /* __NANVIX_MICROKERNEL */

/**@}*/

#endif /* NANVIX_SYSCALL_H_ */

