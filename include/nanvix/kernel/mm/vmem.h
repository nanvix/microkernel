/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef NANVIX_KERNEL_MM_VMEM_H_
#define NANVIX_KERNEL_MM_VMEM_H_

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/hal.h>
#include <nanvix/types.h>

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @brief Null virtual memory space.
 *
 * @note This is set so as it falls beyond the valid range of indexes in the
 * table of virtual memory spaces.
 */
#define VMEM_NULL ((vmem_t)-1)

/*============================================================================*
 * Type Definitions                                                           *
 *============================================================================*/

/**
 * @brief Virtual memory space handle.
 */
typedef int vmem_t;

/*============================================================================*
 * Functions                                                                  *
 *============================================================================*/

/**
 * @brief Returns the underlying page directory of a virtual memory space.
 *
 * @deprecated
 */
extern const struct pde *vmem_pgdir_get(vmem_t vmem);

/**
 * @brief Creates a virtual memory space.
 *
 * @returns Upon successful completion, a pointer to the newly allocated virtual
 * memory space is returned. On failure, a null pointer is returned instead.
 */
extern vmem_t vmem_create(void);

/**
 * @brief Destroys a virtual memory space.
 *
 * @param vmem Target virtual memory space.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int vmem_destroy(vmem_t vmem);

/**
 * @brief Changes access permissions of a user page.
 *
 * @param vmem  Target virtual memory space.
 * @param vaddr Target virtual address.
 * @param mode  Access mode permissions.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int vmem_ctrl(vmem_t vmem, vaddr_t vaddr, mode_t mode);

/**
 * @brief Gets information on a user page.
 *
 * @param vmem  Target virtual memory space.
 * @param vaddr Target virtual address.
 * @param buf   Storage location for page information.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
extern int vmem_info(vmem_t vmem, vaddr_t vaddr, struct pageinfo *buf);

/**
 * @brief Attaches a virtual address range to a virtual memory space.
 *
 * @param vmem Target virtual memory space.
 * @param addr Address where the stack should be attached.
 * @param size Size of virtual address range.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int vmem_attach(vmem_t vmem, vaddr_t addr, size_t size);

/**
 * @brief Maps a virtual address range into a virtual memory space.
 *
 * @param vmem Target virtual memory space.
 * @param vaddr Target virtual address.
 * @param frame Target page frame.
 * @param size Size of virtual address range.
 * @param w Write permission.
 * @param x Execute permission.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int vmem_map(vmem_t vmem, vaddr_t vaddr, frame_t frame, size_t size,
                    bool w, bool x);

/**
 * @brief Maps a virtual address range into a virtual memory space.
 *
 * @param vmem Target virtual memory space.
 * @param vaddr Target virtual address.
 *
 * @returns Upon successful completion, a handle to the unmapped page frame is
 * returned. Upon failure, @p FRAME_NULL is returned instead.
 */
extern frame_t vmem_unmap(vmem_t vmem, vaddr_t vaddr);

/**
 * @brief Prints a virtual memory space.
 *
 * @param vmem Target virtual memory space.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
extern int vmem_print(vmem_t vmem);

/**
 * @brief Initializes the virtual memory manager.
 *
 * @param root_pgdir Root page directory.
 *
 * @returns A pointer to the root virtual memory space is returned. If this
 * function does not succeed, the kernel panics.
 */
extern vmem_t vmem_init(const struct pde *root_pgdir);

/*============================================================================*/

#endif /* NANVIX_KERNEL_MM_VMEM_H_ */
