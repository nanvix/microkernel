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

#include <nanvix/kernel/excp.h>
#include <nanvix/hal.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/const.h>
#include <nanvix/hlib.h>
#include <posix/errno.h>

#ifndef __unix64__
#if ((PGDIR_LENGTH*PDE_SIZE) > PAGE_SIZE)
#error "page size too small"
#endif

#if ((PGTAB_LENGTH*PTE_SIZE) > PAGE_SIZE)
#error "page size too small"
#endif
#endif

/*============================================================================*
 * pgtab_map()                                                                *
 *============================================================================*/

/**
 * @brief Maps a page table.
 *
 * The pgtab_map() function maps a page table in the page directory
 * pointed to by @p pgdir at the virtual address @p vaddr. The target
 * address should lie in user space.
 *
 * This function fails either if a page table is already mapped for
 * the target virtual address, or a kernel page could not be allocated
 * to accommodate the page table.
 *
 * @param pgdir Target page directory.
 * @param vaddr Target virtual address.
 *
 * @returns Upon successful completion, a pointer to the page
 * directory entry of the mapped page table is returned. Upon failure,
 * @p NULL is returned instead.
 *
 * @retval NULL Target address is already in use.
 * @retval NULL Page table cannot be allocated.
 *
 * @note No error checking is performed on the parameters, because
 * this is an private function of the User Page Pool subsystem, and we
 * trust higher level routines.
 *
 * @bug Properly flush TLB in multicores.
 *
 * @see upage_map() and pgtab_unmap().
 *
 * @author Pedro Henrique Penna
 */
PRIVATE inline struct pde *pgtab_map(struct pde *pgdir, vaddr_t vaddr)
{
	frame_t frame;     /* Working page frame.           */
	struct pde *pde;   /* Working page table directory. */
	struct pte *pgtab; /* Page table.                   */

	/*
	 * Align virtual address
	 * to page page table boundary.
	 */
	vaddr &= PGTAB_MASK;

	pde = pde_get(pgdir, vaddr);

	/* Busy page directory entry. */
	if (pde_is_present(pde))
	{
		kprintf("[kernel][mm] busy page table directory entry");
		return (NULL);
	}

	/*
	 * Allocate a kernel page to
	 * accommodate the page table.
	 */
	if ((pgtab = kpage_get(false)) == NULL)
	{
		kprintf("[kernel][mm] cannot allocate page table");
		return (NULL);
	}

	/*
	 * Map kernel page.
	 *
	 * FIXME: in a multicore platform, we should
	 * flush the TLB of each affected core.
	 */
	frame = kpool_addr_to_frame(VADDR(pgtab));
	mmu_pgtab_map(
		pgdir,
		frame << PAGE_SHIFT,
		vaddr
	);
	KASSERT(pde_is_present(pde));

	dcache_invalidate();
	tlb_flush();

	/*
	 * Clear the page table.
	 * Note that we do not kmemset(),
	 * because the actual way of cleaning
	 * PTEs is platform dependent.
	 */
	for (unsigned long i = 0; i < PAGE_SIZE/PDE_SIZE; i++)
		pte_clear(&pgtab[i]);

	return (pde);
}

/*============================================================================*
 * pgtab_unmap()                                                              *
 *============================================================================*/

/**
 * @brief Releases a unmaps table.
 *
 * The pgtab_unmap() function unmaps the page table in the page
 * directory pointed to by @p pgdir at the virtual address @p vaddr.
 *
 * This function failes either if any page is yet in use in the target
 * page table, or the kernel page that accommodates the page table
 * itself cannot be released.
 *
 * @returns Upon successful completion, zero is returned. Upon
 * failure, a negative error code is returned instead.
 *
 * @note No error checking is performed on the parameters, because
 * this is an private function of the User Page Pool subsystem, and we
 * trust higher level routines.
 *
 * @retval -EBUSY  Page table in use.
 * @retval -EIO    Cannot release underlying kernel page.
 *
 * @see upage_unmap() and pgtab_map().
 *
 * @bug Properly flush TLB in multicores.
 *
 * @author Pedro Henrique Penna
 */
PRIVATE int pgtab_unmap(struct pde *pgdir, vaddr_t vaddr)
{
	frame_t frame;     /* Working page frame.           */
	struct pde *pde;   /* Working page directory entry. */
	struct pte *pgtab; /* Page table.                   */

	pde = pde_get(pgdir, vaddr);

	/* Nothing to do. */
	if (!pde_is_present(pde))
		return (0);

	/* Retrieve page table. */
	frame = pde_frame_get(pde);
	pgtab = (struct pte *) kpool_frame_to_addr(frame);

	/* Check if the page table is yet in use. */
	for (unsigned long i = 0; i < PAGE_SIZE/PDE_SIZE; i++)
	{
		/* Yes. */
		if (pte_is_present(&pgtab[i]))
			return (-EBUSY);
	}

	/*
	 * Unmap page table before releasing the
	 * kernel page, because we may fail bellow.
	 *
	 * FIXME: in a multicore platform, we should
	 * flush the TLB of each affected core.
	 */
	pde_present_set(pde, 0);
	dcache_invalidate();
	tlb_flush();

	/* Cannot release kernel page. */
	if (kpage_put(pgtab) < 0)
	{
		kprintf("[kernel][mm] kernel page leak");
		return (-EIO);
	}

	return (0);
}

/*============================================================================*
 * upage_inval()                                                              *
 *============================================================================*/

/**
 * @todo Provide a detailed description for this function.
 */
PUBLIC int upage_inval(vaddr_t vaddr)
{
	vaddr &= PAGE_MASK;

	/* Bad virtual address. */
	if (!mm_is_uaddr(vaddr))
		return (-EINVAL);

#if (!CORE_HAS_TLB_HW)
	KASSERT(tlb_inval(TLB_INSTRUCTION, vaddr) == 0);
	KASSERT(tlb_inval(TLB_DATA, vaddr) == 0);
#endif

#if (CLUSTER_HAS_TLB_SHOOTDOWN)
	KASSERT(tlb_shootdown(vaddr) == 0);
#elif !defined(__unix64__)
	kprintf("[kernel][mm] cannot shootdown %x", vaddr);
#endif

	tlb_flush();

	return (0);
}

/*============================================================================*
 * upage_map()                                                                *
 *============================================================================*/

/**
 * The upage_map() function maps the user page frame @p frame in the
 * page directory pointed to by @p pgdir at virtual address @p vaddr.
 *
 * This function fails either if @p pgdir does not point to a valid
 * page directory, @p vaddr does not lie in user space, @p vaddr is
 * not aligned at a page boundary, a page is already allocated at
 * address @p vaddr, or @p frame does not refer to a user page frame.
 *
 * @retval -EADDRINUSE Target address is already in use.
 * @retval -EINVAL     Target address is not properly aligned.
 * @retval -EFAULT     Target address does not lie in user space.
 * @retval -EINVAL     Invalid page directory.
 * @retval -EINVAL     Frame is not a user page frame.
 *
 * @todo Check for bad page directory.
 *
 * @bug Properly flush TLB in multicores.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int upage_map(struct pde *pgdir, vaddr_t vaddr, frame_t frame)
{
	struct pte *pte;   /* Working page table table entry. */
	struct pde *pde;   /* Working page directory entry.   */
	struct pte *pgtab; /* Working page table.             */

	/* Invalid page directory. */
	if (pgdir == NULL)
		return (-EINVAL);

	/* TODO: check for bad page directory. */

	/* Bad virtual address. */
	if (!mm_is_uaddr(vaddr))
		return (-EFAULT);

	/* Misaligned target address. */
	if (vaddr & (~PAGE_MASK))
		return (-EINVAL);

	/* Check for bad page frame. */
	if (!frame_is_valid_num(frame))
		return (-EINVAL);

	/*
	 * Retrieve page directory entry
	 * of target page.
	 */
	pde = pde_get(pgdir, vaddr);
	if (!pde_is_present(pde))
	{
		if ((pde = pgtab_map(pgdir, vaddr)) == NULL)
			return (-EAGAIN);
	}

	/*
	 * Retrieve the page table entry
	 * of the target page.
	 */
	pgtab = (struct pte *)(kpool_frame_to_addr(pde_frame_get(pde)));
	pte = pte_get(pgtab, vaddr);

	/*
	 * The target page is already mapped.
	 * Fortunately, we have nothing to rollback.
	 * If the page is already present,
	 * it means that the page table was
	 * previously allocated (hopefully no one
	 * has tempered the memory).
	 */
	if (pte_is_present(pte))
		return (-EADDRINUSE);

	/*
	 * Map page.
	 *
	 * FIXME: in a multicore platform, we should
	 * flush the TLB of each affected core core.
	 */
	mmu_page_map(
		pgtab,
		frame << PAGE_SHIFT,
		vaddr,
		true,
		false
	);
	KASSERT(pte_is_present(pte));

	dcache_invalidate();
	tlb_flush();

	return (0);
}

/*============================================================================*
 * upage_unmap()                                                              *
 *============================================================================*/

/**
 * The upage_unmap() function unmaps the user page that was previously
 * mapped by a call to upage_map() in the page directory pointed to by
 * @p pgdir at virtual address @p vaddr.
 *
 * This function fails either if @p pgdir points to an invalid page
 * directory, @p vaddr does not lie in user space, @p vaddr is not
 * aligned at a page boundary, or no page is allocated at address @p
 * vaddr.
 *
 * @note If the kernel is compiled with @p NANVIX_FAST_MEMORY option,
 * the kernel does not try to release underlying page tables.
 *
 * @retval FRAME_NULL Invalid page directory.
 * @retval FRAME_NULL Target address does not lie in user space.
 * @retval FRAME_NULL Target address is not properly aligned.
 * @retval FRAME_NULL Target address is not in use.
 *
 * @todo Check for bad page directory.
 *
 * @bug Properly flush TLB in multicores.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC frame_t upage_unmap(struct pde *pgdir, vaddr_t vaddr)
{
	frame_t frame;     /* Frame.                                 */
	struct pte *pte;   /* Page table table entry of target page. */
	struct pde *pde;   /* Page directory entry of target page.   */
	struct pte *pgtab; /* Page table of target page.             */

	/* Invalid page directory. */
	if (pgdir == NULL)
		return (FRAME_NULL);

	/* TODO: check for bad page directory. */

	/* Bad virtual address. */
	if (!mm_is_uaddr(vaddr))
		return (FRAME_NULL);

	/* Misaligned target address. */
	if (vaddr & (~PAGE_MASK))
		return (FRAME_NULL);

	/* Look up page table. */
	pde = pde_get(pgdir, vaddr);

	/* Target address not in use. */
	if (!pde_is_present(pde))
		return (FRAME_NULL);

	/*
	 * Retrieve page table entry
	 * of target page.
	 */
	pgtab = (struct pte *)(kpool_frame_to_addr(pde_frame_get(pde)));
	pte = pte_get(pgtab, vaddr);

	/* Target address not in use. */
	if (!pte_is_present(pte))
		return (FRAME_NULL);

	/*
	 * Unmap page.
	 *
	 * FIXME: in a multicore platform, we should
	 * flush the TLB of each affected core core.
	 */
	frame = pte_frame_get(pte);
	pte_present_set(pte, 0);

	upage_inval(vaddr);

	/* Free underlying page tables. */
#ifndef NANVIX_FAST_MEMORY
	pgtab_unmap(pgdir, vaddr);
#endif

	return (frame);
}

/*============================================================================*
 * upage_alloc()                                                              *
 *============================================================================*/

/**
 * The upage_alloc() function allocates a user page in the page
 * directory pointed to by @p pgdir at virtual address @p vaddr.
 *
 * This function fails either if @p pgdir does not point to a valid
 * page directory, @p vaddr does not lie in user space, @p vaddr is
 * not aligned at a page boundary, a page is already allocated at
 * address @p vaddr, or a page frame cannot be allocated to
 * accommodate the new page.
 *
 * @retval -EAGAIN     Cannot allocate a page frame.
 * @retval -EADDRINUSE Target address is already in use.
 * @retval -EINVAL     Target address is not properly aligned.
 * @retval -EFAULT     Target address does not lie in user space.
 * @retval -EINVAL     Invalid page directory.
 *
 * @note If the kernel is compiled with @p NANVIX_FAST_MEMORY option,
 * no memory clean is performed after a successful page allocation.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int upage_alloc(struct pde *pgdir, vaddr_t vaddr)
{
	int err;       /* Error code. */
	frame_t frame; /* Page frame. */

	if (pgdir == NULL)
		return (-EINVAL);

	/*
	 * upage_map() checks the
	 * other necessary requirements.
	 */

	/* Allocate user frame. */
	if ((frame = frame_alloc()) == FRAME_NULL)
		return (-EAGAIN);

	/* Map user page. */
	if ((err = upage_map(pgdir, vaddr, frame)) < 0)
	{
		if (frame_free(frame) < 0)
			kprintf("[kernel][mm] page frame leak");

		return (err);
	}

#ifndef NANVIX_FAST_MEMORY
	kmemset((void *)(vaddr), 0, PAGE_SIZE);
#endif

	return (0);
}

/*============================================================================*
 * upage_free()                                                               *
 *============================================================================*/

/**
 * The upage_free() function releases a user page that was previously
 * allocated by a call to upage_alloc() in the page directory pointed
 * to by @p pgdir at virtual address @p vaddr.
 *
 * This function fails either if @p pgdir points to an invalid page
 * directory, @p vaddr does not lie in user space, @p vaddr is not
 * aligned at a page boundary, no page is allocated at address @p
 * vaddr, or the underlying page frame cannot be released.
 *
 * @retval -EFAULT Invalid page directory.
 * @retval -EFAULT Target address does not lie in user space.
 * @retval -EFAULT Target address is not properly aligned.
 * @retval -EFAULT Target address is not in use.
 * @retval -EIO    Cannot release underlying page frame.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int upage_free(struct pde *pgdir, vaddr_t vaddr)
{
	frame_t frame;

	if (pgdir == NULL)
		return (-EFAULT);

	/*
	 * upage_map() checks the
	 * other necessary requirements.
	 */

	/* Unmap user page. */
	if ((frame = upage_unmap(pgdir, vaddr)) == FRAME_NULL)
		return (-EFAULT);

	/* Release page frame. */
	if (frame_free(frame) < 0)
	{
		kprintf("[kernel][mm] page frame leak");

		return (-EIO);
	}

	return (0);
}

/*============================================================================*
 * upage_link()                                                               *
 *============================================================================*/

/**
 * @todo TODO: provide a detailed description for this function.
 */
PUBLIC int upage_link(struct pde *pgdir, vaddr_t vaddr1, vaddr_t vaddr2)
{
	frame_t frame;     /* Underlying page frame.          */
	struct pte *pte1;  /* Working page table table entry. */
	struct pde *pde1;  /* Working page directory entry.   */
	struct pte *pgtab; /* Working page table.             */

	/* Invalid page directory. */
	if (pgdir == NULL)
		return (-EINVAL);

	/* TODO: check for bad page directory. */

	/* Bad virtual address. */
	if (!mm_is_uaddr(vaddr2))
		return (-EFAULT);

	/* Misaligned target address. */
	if (vaddr2 & (~PAGE_MASK))
		return (-EINVAL);

	/*
	 * Retrieve page directory entry
	 * of target page.
	 */
	pde1 = pde_get(pgdir, vaddr1);
	if (!pde_is_present(pde1))
		return (-EFAULT);

	/*
	 * Retrieve the page table entry
	 * of the target page.
	 */
	pgtab = (struct pte *)(kpool_frame_to_addr(pde_frame_get(pde1)));
	pte1 = pte_get(pgtab, vaddr1);

	/* Unampped source page. */
	if (!pte_is_present(pte1))
		return (-EFAULT);

	frame = pte_frame_get(pte1);

	return (upage_map(pgdir, vaddr2, frame));
}

/*============================================================================*
 * do_tlb_fault()                                                             *
 *============================================================================*/

#if (!CORE_HAS_TLB_HW)

/**
 * @brief Handles a TLB fault.
 *
 * The do_tlb_fault() function handles a TLB fault. It checks the
 * current page directory for a virtual-to-physical address mapping,
 * and if it finds one, it writes this mapping to the TLB. If the
 * faulting address is not currently mapped in the current page
 * directory, this exception is forwarded to the page fault handler.
 *
 * @param excp Exception information.
 * @param ctx  Interrupted execution context.
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void do_tlb_fault(
	const struct exception *excp,
	const struct context *ctx
)
{
	bool retry = true; /* Retry handler?                  */
	int tlb_type;      /* Type of TLB.                    */
	paddr_t paddr;     /* Physical address.               */
	vaddr_t vaddr;     /* Faulting address.               */
	struct pte *pte;   /* Working page table table entry. */
	struct pde *pde;   /* Working page directory entry.   */
	struct pte *pgtab; /* Working page table.             */

	UNUSED(ctx);

	/* Get page address of faulting address. */
	vaddr = exception_get_addr(excp);
	vaddr &= PAGE_MASK;

#ifdef NDEBUG

	/*
	 * The faulting address lies in kernel
	 * land and this is unlikely to happen.
	 * So, it's better to get some warning.
	 */
	if (!mm_check_area(vaddr, PAGE_SIZE, UMEM_AREA))
	{
	#ifndef __mppa256__
		kprintf("[kernel][mm] tlb fault in kernel land at %x",
			exception_get_addr(excp)
		);
	#endif
	}

#endif

again:

	if (!retry)
		return;

	/* Lookup PDE. */
	pde = pde_get(root_pgdir, vaddr);
	if (!pde_is_present(pde))
	{
		exception_forward(EXCEPTION_PAGE_FAULT, excp, ctx);
		retry = false;
		goto again;
	}

	/* Lookup PTE. */
	pgtab = (struct pte *)(pde_frame_get(pde) << PAGE_SHIFT);
	pte = pte_get(pgtab, vaddr);
	if (!pte_is_present(pte))
	{
		exception_forward(EXCEPTION_PAGE_FAULT, excp, ctx);
		retry = false;
		goto again;
	}

	/* Writing mapping to TLB. */
	tlb_type = (excp->num == EXCEPTION_DTLB_FAULT) ? TLB_DATA : TLB_INSTRUCTION;
	paddr = pte_frame_get(pte) << PAGE_SHIFT;
	if (tlb_write(tlb_type, vaddr, paddr) < 0)
		kpanic("cannot write to tlb");

	tlb_flush();
}

#endif

/*============================================================================*
 * do_page_fault()                                                            *
 *============================================================================*/

/**
 * @todo TODO Provide a detailed description for this function.
 */
PRIVATE void do_page_fault(
	const struct exception *excp,
	const struct context *ctx
)
{
	UNUSED(ctx);
	UNUSED(excp);

#if (THREAD_MAX > 1)
	exception_wait(EXCEPTION_PAGE_FAULT, excp);
#endif
}

/*============================================================================*
 * upool_init()                                                               *
 *============================================================================*/

/**
 * The upool_init() function initializes internal structures of the
 * user page allocator. Additionally, if the kernel is compiled
 * without the @p NDEBUG build option, unit tests on the user page
 * allocator are launched once its initialization is completed.
 */
PUBLIC void upool_init(void)
{
	kprintf("[kernel][mm] initializing the user page allocator");

	/* Register handlers. */
#if (!CORE_HAS_TLB_HW)
	exception_register(EXCEPTION_DTLB_FAULT, do_tlb_fault);
	exception_register(EXCEPTION_ITLB_FAULT, do_tlb_fault);
#endif

	#ifndef __SUPPRESS_TESTS
		kprintf("[kernel][mm] running tests on the user page allocator");
		upool_test_driver();
	#endif

	exception_register(EXCEPTION_PAGE_FAULT, do_page_fault);

}
