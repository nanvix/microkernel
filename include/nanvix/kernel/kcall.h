/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_KCALL_H_
#define NANVIX_KERNEL_KCALL_H_

/**
 * @brief Number of system calls.
 *
 * @note This should be set to the highest system call number.
 */
#define NR_SYSCALLS NR_last_kcall

/**
 * @brief Requests for vmctrl()
 */
/**@{*/
#define VMEM_CHMOD 0 /** Change permissions. */
/**@}*/

/**
 * @brief Access permissions on a virtual memory space.
 */
#define VMEM_READ (1 << 0)  /** Read permission.    */
#define VMEM_WRITE (1 << 1) /** Write permission.   */
#define VMEM_EXEC (1 << 2)  /** Execute permission. */

/**
 * @name System Call Numbers
 */
/**@{*/
#define NR_void0 0                /** kernel_void0()            */
#define NR_void1 1                /** kernel_void1()            */
#define NR_void2 2                /** kernel_void2()            */
#define NR_void3 3                /** kernel_void3()            */
#define NR_void4 4                /** kernel_void4()            */
#define NR_void5 5                /** kernel_void5()            */
#define NR_shutdown 6             /** kernel_shutdown()         */
#define NR_write 7                /** kernel_write()            */
#define NR_fralloc 8              /** kernel_fralloc()          */
#define NR_frfree 9               /** kernel_frfree()           */
#define NR_vmcreate 10            /** kernel_vmcreate()         */
#define NR_vmremove 11            /** kernel_vmremove()         */
#define NR_vmmap 12               /** kernel_vmmap()            */
#define NR_vmunmap 13             /** kernel_vmunmap()          */
#define NR_vmctrl 14              /** kernel_vmctrl()           */
#define NR_vminfo 15              /** kernel_vminfo()           */
#define NR_kmod_get 16            /** kernel_kmod_get()         */
#define NR_spawn 17               /** kernel_spawn()            */
#define NR_semget 18              /** kernel_semget()           */
#define NR_semop 19               /** kernel_semop()            */
#define NR_semctl 20              /** kernel_semctl()           */
#define NR_thread_get_id 21       /** kernel_thread_get_id()    */
#define NR_thread_create 22       /** kernel_thread_create()    */
#define NR_thread_exit 23         /** kernel_thread_exit()      */
#define NR_thread_yield 24        /** kernel_thread_yield()     */
#define NR_mailbox_tag 25         /** kernel_mailbox_tag        */
#define NR_mailbox_is_assigned 26 /** kernel_mailbox_is_assigned()*/
#define NR_mailbox_owner 27       /** kernel_mailbox_owner()    */
#define NR_mailbox_default 28     /** kernel_mailbox_default()  */
#define NR_mailbox_assign 29      /** kernel_mailbox_assign()   */
#define NR_mailbox_link 30        /** kernel_mailbox_link()     */
#define NR_mailbox_unlink 31      /** kernel_mailbox_unlink()   */
#define NR_mailbox_push 32        /** kernel_mailbox_push()     */
#define NR_mailbox_pop 33         /** kernel_mailbox_pop()      */
#define NR_last_kcall 34          /** NR_SYSCALLS definer       */
#define NR__exit                  /** kernel_exit()             */
#define NR_process_get_id         /** kernel_process_get_id()   */
#define NR_process_create         /** kernel_process_create()   */
#define NR_process_exit           /** kernel_process_exit()     */
#define NR_process_join           /** kernel_process_join()     */
#define NR_process_yield          /** kernel_process_yield()    */
#define NR_sleep                  /** kernel_sleep()            */
#define NR_wakeup                 /** kernel_wakeup()           */
#define NR_sigctl                 /** kernel_sigctl()           */
#define NR_alarm                  /** kernel_alarm()            */
#define NR_sigsend                /** kernel_sigsend()          */
#define NR_sigwait                /** kernel_sigwait()          */
#define NR_sigreturn              /** kernel_sigreturn()        */
#define NR_clock                  /** kernel_clock()            */
#define NR_stats                  /** kernel_stats()            */
#define NR_upage_alloc            /** kernel_upage_alloc()      */
#define NR_upage_free             /** kernel_upage_free()       */
#define NR_upage_map              /** kernel_upage_map()        */
#define NR_upage_link             /** kernel_upage_link()       */
#define NR_upage_unlink           /** kernel_upage_unlink()     */
#define NR_upage_unmap            /** kernel_upage_unmap()      */
#define NR_excp_ctrl              /** kernel_excp_ctrl()        */
#define NR_excp_pause             /** kernel_excp_pause()       */
#define NR_excp_resume            /** kernel_excp_resume()      */
/**@}*/

#endif /* NANVIX_KERNEL_KCALL_H_ */
