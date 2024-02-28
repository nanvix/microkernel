/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef KERNEL_KCALL_MOD_H_
#define KERNEL_KCALL_MOD_H_

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/kmod.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/pm.h>
#include <stddef.h>
#include <stdnoreturn.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @brief Issues a void kernel call that takes no arguments.
 *
 * @return Always returns zero.
 */
extern int kcall_void0(void);

/**
 * @brief Issues a void kernel call that takes one argument.
 *
 * @param arg0 First argument.
 *
 * @return Always returns @p arg0.
 */
extern int kcall_void1(int arg0);

/**
 * @brief Issues a void kernel call that takes two arguments.
 *
 * @param arg0 First argument.
 * @param arg1 Second argument.
 *
 * @return Always returns @p arg0 + @p arg1.
 */
extern int kcall_void2(int arg0, int arg1);

/**
 * @brief Issues a void kernel call that takes three arguments.
 *
 * @param arg0 First argument.
 * @param arg1 Second argument.
 * @param arg2 Third argument.
 *
 * @return Always returns @p arg0 + @p arg1 + @p arg2.
 */
extern int kcall_void3(int arg0, int arg1, int arg2);

/**
 * @brief Issues a void kernel call that takes four arguments.
 *
 * @param arg0 First argument.
 * @param arg1 Second argument.
 * @param arg2 Third argument.
 * @param arg3 Fourth argument.
 *
 * @return Always returns @p arg0 + @p arg1 + @p arg2 + @p arg3.
 */
extern int kcall_void4(int arg0, int arg1, int arg2, int arg3);

/**
 * @brief Issues a void kernel call that takes five arguments.
 *
 * @param arg0 First argument.
 * @param arg1 Second argument.
 * @param arg2 Third argument.
 * @param arg3 Fourth argument.
 * @param arg4 Fifth argument.
 *
 * @return Always returns @p arg0 + @p arg1 + @p arg2 + @p arg3 + @p arg4.
 */
extern int kcall_void5(int arg0, int arg1, int arg2, int arg3, int arg4);

/**
 * @brief Shutdowns the system
 */
extern noreturn void kcall_shutdown(void);

/**
 * @brief Writes a buffer to a file descriptor.
 *
 * @param fd  Target file descriptor.
 * @param buf Target buffer.
 * @param n   Number of bytes to write.
 *
 * @returns The number of bytes written is returned.
 */
extern size_t kcall_write(int fd, const char *buf, size_t n);

/**
 * @brief Attempts to allocate a page frame.
 *
 * @return On success, the number of the allocated page frame is returned.
 * On failure, @p FRAME_NULL is returned instead.
 */
extern frame_t kcall_fralloc(void);

/**
 * @brief Frees a page frame.
 *
 * @param frame Number of the target page frame.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
extern int kcall_frfree(frame_t frame);

/**
 * @brief Creates a virtual memory space.
 *
 * @return Upon successful completion, a handle to the newly created virtual
 * memory space is returned. Upon failure, @p VMEM_NULL is returned instead.
 */
extern vmem_t kcall_vmcreate(void);

/**
 * @brief Removes a virtual memory space.
 *
 * @param vmem Handle to the target virtual memory space.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int kcall_vmremove(vmem_t vmem);

/**
 * @brief Maps a page frame at a virtual address.
 *
 * @param vmem Handle to the target virtual memory space.
 * @param vaddr Handle to the virtual address.
 * @param frame Target page frame
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int kcall_vmmap(vmem_t vmem, vaddr_t vaddr, frame_t frame);

/**
 * @brief Unmaps a page frame from a virtual address.
 *
 * @param vmem Handle to the target virtual memory space.
 * @param vaddr Handle to the target virtual address.
 *
 * @return Upon successful completion, a handle to the unmapped page frame is
 * returned. Upon failure, @p FRAME_NULL is returned instead.
 */
extern frame_t kcall_vmunmap(vmem_t vmem, vaddr_t vaddr);

/**
 * @brief Manipulates various parameters from a virtual memory space.
 *
 * @param vmem    Handle to the target virtual memory space.
 * @param request Request.
 * @param vaddr   Target virtual address.
 * @param mode    Access permissions mode.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int kcall_vmctrl(vmem_t vmem, unsigned request, vaddr_t vaddr,
                        mode_t mode);

/**
 * @brief Gets information on a page.
 *
 * @param vmem Handle to the target virtual memory space.
 * @param vaddr Target virtual address.
 * @param buf Storage location for page information.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int kcall_vminfo(vmem_t vmem, vaddr_t vaddr, struct pageinfo *buf);

/**
 * @brief Gets information on a kernel module.
 *
 * @param kmod Storage location for kernel module information.
 * @param index Index of target kernel module.
 *
 * @returns Upon successful completion, zero is returned. Upon failure,
 * a negative number is returned instead.
 */
extern int kcall_kmod_get(struct kmod *kmod, unsigned index);

/**
 * @brief Spawns a new process.
 *
 * @param image Image of the process.
 *
 * @returns Upon successful completion, the PID of the spawned process
 * is returned. Upon failure, a negative error code is returned
 * instead.
 */
extern pid_t kcall_spawn(void *image);

/**
 * @brief Get a semaphore.
 *
 * @param key Key for allow semaphore access.
 *
 * @returns Upon successful, Semaphore ID. Upon Failure, a negative
 * error code is returned instead.
 */
extern int kcall_semget(unsigned key);

/**
 * @brief Get mailbox tag.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful, mailbox tag. Upon Failure, a negative
 * error code is returned instead.
 */
extern int kcall_mailbox_tag(int mbxid);

/**
 * @brief Checks whether a mailbox is assigned.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful (mailbox is assigned), returns 1. Upon Failure,
 * returns 0.
 */
extern int kcall_mailbox_is_assigned(int mbxid);

/**
 * @brief Gets the owner of a mailbox.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful, returns the owner. Upon Failure,
 * returns a negative error code, -EBADF(-8).
 */
extern pid_t kcall_mailbox_owner(int mbxid);

/**
 * @brief Initializes a mailbox with the default values.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code. -EBADF(-8).
 */
extern int kcall_mailbox_default(int mbxid);

/**
 * @brief Assigns a mailbox.
 *
 * @param mbxid mailbox ID.
 * @param owner owner of box
 * @param tag tag of box
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code. -EBADF(-8).
 */
extern int kcall_mailbox_assign(int mbxid, pid_t owner, int tag);

/**
 * @brief Links a mailbox.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code. -EBADF(-8).
 */
extern int kcall_mailbox_link(int mbxid);

/**
 * @brief Unlinks a mailbox.
 *
 * @param mbxid mailbox ID.
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code. -EBADF(-8).
 */
extern int kcall_mailbox_unlink(int mbxid);

/**
 * @brief Pushes a message into a mailbox.
 *
 * @param mbxid mailbox ID.
 * @param msg message
 * @param sz size of message
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code.
 */
extern int kcall_mailbox_push(int mbxid, void *msg, size_t sz);

/**
 * @brief Pop a message from a mailbox.
 *
 * @param mbxid mailbox ID.
 * @param msg message
 * @param sz size of message
 *
 * @returns Upon successful, returns the 0. Upon Failure,
 * returns a negative error code.
 */
extern int kcall_mailbox_pop(int mbxid, void *msg, size_t sz);

/**
 * @brief Verify if process can operate in semaphore (id), and operate (op):
 *
 * @param id Semaphore Identifier.
 *
 * @param op Operation.
 *
 * @returns Upon successful, zero code. Upon Failure, a negative
 * error code is returned instead.
 */
extern int kcall_semop(int id, int op);

/**
 * @brief Verify if process can control the semaphore (id), and exec (cmd).
 *
 * @param id Semaphore Identifier.
 *
 * @param cmd Command.
 *
 * @param val Value.
 *
 * @returns Upon successful, zero code or semaphore value. Upon Failure,
 * a negative error code is returned instead.
 */
extern int kcall_semctl(int id, int cmd, int val);

/**
 * @brief Gets the ID of the calling thread.
 *
 * @returns The ID of the calling thread is returned.
 */
extern tid_t kcall_thread_get_id(void);

/**
 * @brief Creates a new thread.
 *
 * @param pid PID of the target process.
 *
 * @returns Upon successful completion, the ID of the newly created
 * thread is returned. Upon failure, a negative error code is
 * returned instead.
 */
extern tid_t kcall_thread_create(pid_t pid);

/**
 * @brief Exits the calling thread.
 */
extern noreturn void kcall_thread_exit(void);

/**
 * @brief Yields the processor to another thread.
 *
 * @returns Upon successful completion, zero is returned. Upon failure,
 * a negative error code is returned instead.
 */
extern noreturn void kcall_thread_yield(void);

/*============================================================================*/

#endif /* KERNEL_KCALL_MOD_H_ */
