/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include "mod.h"
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/kcall.h>
#include <nanvix/kernel/kmod.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/pm.h>
#include <stdnoreturn.h>

/*============================================================================*
 * Private Variables                                                          *
 *============================================================================*/

static struct semaphore kernel_semahore = SEMAPHORE_INITIALIZER(0);
static struct semaphore user_semaphore = SEMAPHORE_INITIALIZER(0);

static volatile struct {
    word_t kcall_nr; /** Kernel call number.             */
    word_t arg0;     /** First argument of kernel call.  */
    word_t arg1;     /** Second argument of kernel call. */
    word_t arg2;     /** Third argument of kernel call.  */
    word_t arg3;     /** Fourth argument of kernel call. */
    word_t arg4;     /** Fifth argument of kernel call.  */
    word_t ret;      /** Return value of kernel call.    */
} scoreboard;

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

noreturn void handle_syscall(void)
{

    while (true) {
        semaphore_down(&kernel_semahore);

        semaphore_up(&user_semaphore);
    }
}

/**
 * @brief Kernel call dispatcher.
 *
 * @param arg0     First kernel call argument.
 * @param arg1     Second kernel call argument.
 * @param arg2     Third kernel call argument.
 * @param arg3     Fourth kernel call argument.
 * @param arg4     Fifth kernel call argument.
 * @param kcall_nr Kernel call number.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 */
int do_kcall(word_t arg0, word_t arg1, word_t arg2, word_t arg3, word_t arg4,
             word_t kcall_nr)
{
    int ret = -1;

    KASSERT_SIZE_LE(sizeof(unsigned), sizeof(void *));

    switch (kcall_nr) {
        case NR_void0:
            ret = kcall_void0();
            break;
        case NR_void1:
            ret = kcall_void1((int)arg0);
            break;
        case NR_void2:
            ret = kcall_void2((int)arg0, (int)arg1);
            break;
        case NR_void3:
            ret = kcall_void3((int)arg0, (int)arg1, (int)arg2);
            break;
        case NR_void4:
            ret = kcall_void4((int)arg0, (int)arg1, (int)arg2, (int)arg3);
            break;
        case NR_void5:
            ret = kcall_void5(
                (int)arg0, (int)arg1, (int)arg2, (int)arg3, (int)arg4);
            break;
        case NR_write:
            ret = kcall_write((int)arg0, (const char *)arg1, (size_t)arg2);
            break;
        case NR_shutdown:
            kcall_shutdown();
            break;
        case NR_fralloc:
            ret = kcall_fralloc();
            break;
        case NR_frfree:
            ret = kcall_frfree((frame_t)arg0);
            break;
        case NR_vmcreate:
            ret = kcall_vmcreate();
            break;
        case NR_vmremove:
            ret = kcall_vmremove((vmem_t)arg0);
            break;
        case NR_vmmap:
            ret = kcall_vmmap((vmem_t)arg0, (vaddr_t)arg1, (frame_t)arg2);
            break;
        case NR_vmunmap:
            ret = kcall_vmunmap((vmem_t)arg0, (vaddr_t)arg1);
            break;
        case NR_vmctrl:
            ret = kcall_vmctrl(
                (vmem_t)arg0, (unsigned)arg1, (vaddr_t)arg2, (mode_t)arg3);
            break;
        case NR_vminfo:
            ret = kcall_vminfo(
                (vmem_t)arg0, (vaddr_t)arg1, (struct pageinfo *)arg2);
            break;
        case NR_kmod_get:
            ret = kcall_kmod_get((struct kmod *)arg0, (unsigned)arg1);
            break;
        case NR_spawn:
            ret = kcall_spawn((void *)arg0);
            break;
        case NR_semget:
            ret = kcall_semget((unsigned)arg0);
            break;
        case NR_semop:
            ret = kcall_semop((int)arg0, (int)arg1);
            break;
        case NR_semctl:
            ret = kcall_semctl((int)arg0, (int)arg1, (int)arg2);
            break;
        case NR_thread_get_id:
            ret = kcall_thread_get_id();
            break;
        case NR_thread_create:
            ret = kcall_thread_create((pid_t)arg0);
            break;
        case NR_thread_exit:
            kcall_thread_exit();
            break;
        case NR_thread_yield:
            kcall_thread_yield();
            break;
        case NR_mailbox_owner:
            ret = kcall_mailbox_owner((int)arg0);
            break;
        case NR_mailbox_assign:
            ret = kcall_mailbox_assign((int)arg0, (int)arg1, (int)arg2);
            break;
        case NR_mailbox_link:
            ret = kcall_mailbox_link((int)arg0);
            break;
        case NR_mailbox_unlink:
            ret = kcall_mailbox_unlink((int)arg0);
            break;
        case NR_mailbox_push:
            ret = kcall_mailbox_push((int)arg0, (void *)arg1, (int)arg2);
            break;
        case NR_mailbox_pop:
            ret = kcall_mailbox_pop((int)arg0, (void *)arg1, (int)arg2);
            break;
        default:
            // Copy kernel call parameters.
            scoreboard.kcall_nr = kcall_nr;
            scoreboard.arg0 = arg0;
            scoreboard.arg1 = arg1;
            scoreboard.arg2 = arg2;
            scoreboard.arg3 = arg3;
            scoreboard.arg4 = arg4;

            semaphore_up(&kernel_semahore);
            semaphore_down(&user_semaphore);
            break;
    };

    return (ret);
}
