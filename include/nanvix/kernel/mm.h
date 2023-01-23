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

/**
 * @defgroup kernel-mm MM
 * @ingroup kernel
 *
 * @brief Memory Management
 */

#ifndef NANVIX_MM_H_
#define NANVIX_MM_H_

	#include <nanvix/hal.h>

/**
 * @addtogroup kernel-mm
 */
/**@{*/

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @name Memory area identification
	 */
	/**@{*/
	#define KMEM_AREA 0 /**< Kernel memory area. */
	#define UMEM_AREA 1 /**< User memory area.   */
	/**@}*/

	/**
	 * @brief Initializes the Memory Management (MM) system.
	 */
	EXTERN void mm_init(void);

	/**
	 * @brief Asserts a user virtual address.
	 *
	 * The mm_is_uaddr() function asserts whether or not the virtual
	 * address @p vaddr lies in user space.
	 *
	 * @returns If @p vaddr lies in user space, non zero is returned.
	 * Otherwise, zero is returned instead.
	 */
	static inline int mm_is_uaddr(vaddr_t vaddr)
	{
		return ((vaddr >= UBASE_VIRT) && (vaddr < (UBASE_VIRT + UMEM_SIZE)));
	}

	/**
	 * @brief Asserts a kernel virtual address.
	 *
	 * The mm_is_kaddr() function asserts whether or not the virtual
	 * address @p vaddr lies in kernel space.
	 *
	 * @returns If @p vaddr lies in kernel space, non zero is
	 * returned. Otherwise, zero is returned instead.
	 */
	static inline int mm_is_kaddr(vaddr_t vaddr)
	{
	#if !defined(__qemu_x86__)
		return (
			((vaddr >= KBASE_VIRT) && (vaddr < (KBASE_VIRT + KMEM_SIZE)))  ||
			((vaddr >= KPOOL_VIRT) && (vaddr < (KPOOL_VIRT + KPOOL_SIZE)))
		);
	#else
		UNUSED(vaddr);
		return (1);
	#endif
	}

	/**
	 * @brief Checks access permissions to a memory area.
	 *
	 * @param addr Address to be checked.
	 * @param size Size of memory area.
	 * @param area User memory area.
	 *
	 * @returns Non-zero if access is authorized, and zero otherwise.
	 */
	static inline int mm_check_area(vaddr_t vaddr, uint64_t size, int area)
	{
	#if defined(__qemu_x86__)

		return (
			(area == UMEM_AREA) ?
				mm_is_uaddr(vaddr) && mm_is_uaddr(vaddr + size) :
				mm_is_kaddr(vaddr) && mm_is_kaddr(vaddr + size)
		);

	#elif defined(__optimsoc__)    || \
		defined(__qemu_openrisc__)

		UNUSED(area);
		return (mm_is_kaddr(vaddr) && mm_is_kaddr(vaddr + size));

	#else

		UNUSED(vaddr);
		UNUSED(size);
		UNUSED(area);

		return (1);

	#endif
	}

#endif

/**@}*/

/*============================================================================*
 * Page Frame Allocator                                                       *
 *============================================================================*/

/**
 * @addtogroup kernel-mm-frame Frame Allocator
 * @ingroup kernel-mm
 *
 * @brief Page Frame Allocator
 */
/**@{*/

	/**
	 * @brief Number of page frames for user use.
	 */
	#define NUM_UFRAMES (UMEM_SIZE/PAGE_SIZE)


	/**
	 * @param Null frame.
	 */
	#define FRAME_NULL ((frame_t) -1)

	/**
	 * @brief Asserts if a frame ID is valid.
	 *
	 * The frame_is_valid_id() function asserts whether or not the
	 * frame @p ID is valid.
	 *
	 * @returns If @p id is valid, non zero is returned. Otherwise,
	 * zero is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int frame_is_valid_id(frame_t id)
	{
		return (id < NUM_UFRAMES);
	}

	/**
	 * @brief Converts an ID of a user page frame to a page frame number.
	 *
	 * @param id ID of target user page frame.
	 *
	 * @returns Frame number of target user page frame.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline frame_t frame_id_to_num(frame_t id)
	{
		/* Invalid ID. */
		if (!frame_is_valid_id(id))
			return (FRAME_NULL);

		return ((UBASE_PHYS >> PAGE_SHIFT) + id);
	}

	/**
	 * @brief Asserts if a frame number is valid.
	 *
	 * The frame_is_valid_num() function asserts whether or not the
	 * frame number @p frame is valid.
	 *
	 * @returns If @p frame is valid, non zero is returned. Otherwise,
	 * zero is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int frame_is_valid_num(frame_t frame)
	{
		return (
			(frame >= (UBASE_PHYS >> PAGE_SHIFT)) &&
			(frame < ((UBASE_PHYS >> PAGE_SHIFT) + NUM_UFRAMES))
		);
	}

	/**
	 * @brief Converts a page frame number to an ID of a user page frame.
	 *
	 * @param frame Number of the target page frame.
	 *
	 * @returns ID of target user page frame.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int frame_num_to_id(frame_t frame)
	{
		/* Invalid frame. */
		if (!frame_is_valid_num(frame))
			return (-1);

		return (frame - (UBASE_PHYS >> PAGE_SHIFT));
	}

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Asserts if a page frame is allocated.
	 *
	 * @param frame Number of the target page frame.
	 *
	 * @returns One if the target page frame is allocated and zero
	 * otherwise.
	 */
	EXTERN int frame_is_allocated(frame_t frame);

	/**
	 * @brief Allocates a page frame.
	 *
	 * @returns Upon successful completion, the number of the
	 * allocated page frame is returned. Upon failure, @p FRAME_NULL
	 * is returned instead.
	 */
	EXTERN frame_t frame_alloc(void);

	/**
	 * @brief Frees a page frame.
	 *
	 * @param frame Number of the target page frame.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int frame_free(frame_t frame);

	/**
	 * @brief Runs unit tests on the frame allocator.
	 */
	EXTERN void frame_test_driver(void);

	/**
	 * @brief Initializes the frame allocator.
	 */
	EXTERN void frame_init(void);

#endif

/**@}*/

/*============================================================================*
 * Kernel Page Pool                                                           *
 *============================================================================*/

/**
 * @addtogroup kernel-mm-kpool Kernel Pool
 * @ingroup kernel-mm
 *
 * @brief Kernel Page Pool
 *
 * The Kernel Page Pool is a subsystem of the Memory Management (MM)
 * system that maintains a pool of pages for kernel use. Overall, this
 * subsystem exposes an interface for allocating and releasing kernel
 * pages, thereby providing the bare-bones for dynamic memory
 * allocation in kernel land.
 *
 * Page frames for the Kernel Page Pool are reserved at system
 * startup. These frames are contiguously located in the physical
 * memory area, but the actual location and the size of this area  is
 * platform dependent, thus being safely accessed through the Hardware
 * Abstraction Layer (HAL).
 */
/**@{*/

	/**
	 * @brief Number of pages for kernel use.
	 */
	#define NUM_KPAGES (KPOOL_SIZE/PAGE_SIZE)

	EXTERN int *kpages_alias;

	EXTERN bitmap_t *frames_alias;

	#define FRAMES_LENGTH (NUM_UFRAMES/BITMAP_WORD_LENGTH)


	static inline vaddr_t kpool_id_to_addr(unsigned id)
	{
		return (KPOOL_VIRT + (id << PAGE_SHIFT));
	}

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Asserts a kernel page address.
	 *
	 * The kpool_is_kpage() function asserts whether or not the
	 * virtual address @p vaddr refers to a kernel page.
	 *
	 * @returns If @p vaddr refers to a kernel page, non zero is
	 * returned. Otherwise, zero is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int kpool_is_kpage(vaddr_t vaddr)
	{
		return ((vaddr >= KPOOL_VIRT) && (vaddr < KPOOL_VIRT + KPOOL_SIZE));
	}

	/**
	 * @brief Translates a kernel page ID into a virtual address.
	 *
	 * The kpool_id_to_addr() function converts the kernel page ID @p
	 * id into a virtual address.
	 *
	 * @param id ID of target kernel page.
	 *
	 * @returns The virtual address of the target kernel page.
	 *
	 * @note This function expects that @p is valid.
	 *
	 * @author Pedro Henrique Penna
	 */
	// static inline vaddr_t kpool_id_to_addr(unsigned id)
	// {
	// 	return (KPOOL_VIRT + (id << PAGE_SHIFT));
	// }

	/**
	 * @brief Translates a frame number into a virtual address.
	 *
	 * The kpool_frame_to_addr() function converts the frame number @p
	 * frame into a virtual address.
	 *
	 * @param frame Target frame.
	 *
	 * @returns The virtual address of the target frame.
	 *
	 * @note This function expects that @p frame is valid.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline vaddr_t kpool_frame_to_addr(frame_t frame)
	{
		return (kpool_id_to_addr(frame - (KPOOL_PHYS >> PAGE_SHIFT)));
	}

	/**
	 * @brief Translates a virtual address into a kernel page ID.
	 *
	 * The kpool_addr_to_id() function converts the virtual address @p
	 * vaddr into a kernel page ID.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns The kernel page ID of the target virtual address.
	 *
	 * @note This function expects that @p vaddr is valid.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline int kpool_addr_to_id(vaddr_t vaddr)
	{
		return ((vaddr - KPOOL_VIRT) >> PAGE_SHIFT);
	}

	/**
	 * @brief Translates a kernel page into a frame number.
	 *
	 * The kpool_addr_to_frame() function converts the virtual address
	 * @p vaddr into a frame number.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns The frame number of a kernel page.
	 *
	 * @note This function expects that @p vaddr is valid.
	 *
	 * @author Pedro Henrique Penna
	 */
	static inline frame_t kpool_addr_to_frame(vaddr_t vaddr)
	{
		return (kpool_addr_to_id(vaddr) + (KPOOL_PHYS >> PAGE_SHIFT));
	}

	/**
	 * @brief Allocates a kernel page.
	 *
	 * @param clean Should the page be cleaned?
	 *
	 * @returns Upon successful completion, a pointer to a kernel page
	 * is returned. Upon failure, a @p NULL pointer is returned
	 * instead.
	 */
	EXTERN void *kpage_get(int clean);

	/**
	 * @brief Releases kernel page.
	 *
	 * @param kpg Kernel page to be released.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int kpage_put(void *kpg);

	/**
	 * @brief Runs unit tests on the kernel page allocator.
	 */
	EXTERN void kpool_test_driver(void);

	/**
	 * @brief Initializes the kernel page pool.
	 */
	EXTERN void kpool_init(void);

#endif

/**@}*/

/*============================================================================*
 * User Page Allocator                                                        *
 *============================================================================*/

/**
 * @addtogroup kernel-mm-upage Page Allocator
 * @ingroup kernel-mm
 *
 * @brief User Page Allocator
 */
/**@{*/

	/**
	 * @brief Number of pages for user use
	 */
	#define NUM_UPAGES NUM_UFRAMES

	/**
	 * @brief Invalidates HW references to a user page.
	 *
	 * @param vaddr Target virtual address.
	 */
	EXTERN int upage_inval(vaddr_t vaddr);
	//is this a problem?

#ifdef __NANVIX_MICROKERNEL

	/**
	 * @brief Asserts an page address.
	 *
	 * The upool_is_upage() function asserts whether or not the
	 * virtual address @p vaddr reffers to a user page.
	 *
	 * @returns If @p vaddr reffers to a user page, non zero is
	 * returned. Otherwise, zero is returned instead.
	 */
	static inline int upool_is_upage(vaddr_t vaddr)
	{
		return (mm_is_uaddr(vaddr));
	}

	/**
	 * @brief Invalidates HW references to a user page.
	 *
	 * @param vaddr Target virtual address.
	 */
	EXTERN int upage_inval(vaddr_t vaddr);

	/**
	 * @brief Maps a page frame into a page.
	 *
	 * @param pgdir Target page directory.
	 * @param vaddr Target virtual address.
	 * @param frame Target page frame.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_unmap().
	 */
	EXTERN int upage_map(struct pde *pgdir, vaddr_t vaddr, frame_t frame);

	/**
	 * @brief Unmaps a page frame.
	 *
	 * @param pgdir Target page directory.
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, the number of the frame
	 * that was previously mapped to the target page is returned. Upon
	 * failure, @p FRAME_NULL is returned instead.
	 *
	 * @see upage_map().
	 */
	EXTERN frame_t upage_unmap(struct pde *pgdir, vaddr_t vaddr);

	/**
	 * @brief Allocates a user page.
	 *
	 * @param pgdir Target page directory.
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_free().
	 */
	EXTERN int upage_alloc(struct pde *pgdir, vaddr_t vaddr);

	/**
	 * @brief Releases a user page.
	 *
	 * @param pgdir Target page directory.
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_alloc().
	 */
	EXTERN int upage_free(struct pde *pgdir, vaddr_t vaddr);

	/**
	 * @brief Links two pages.
	 *
	 * @param pgdir Target page directory.
	 * @param vaddr Virtual address of source page.
	 * @param vaddr Virtual address of target page.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	EXTERN int upage_link(struct pde *pgdir, vaddr_t vaddr1, vaddr_t vaddr2);

	/**
	 * @brief Runs unit tests on the user page allocator.
	 */
	EXTERN void upool_test_driver(void);

	/**
	 * @brief Initializes the user page allocator.
	 */
	EXTERN void upool_init(void);

	/**
	 * @brief Idle page directory.
	 *
	 * @todo This shall be moved to the Hardware Abstraction Layer (HAL).
	 */
	EXTERN struct pde *root_pgdir;

#endif

/**@}*/

#endif /** NANVIX_MM_H_ */
