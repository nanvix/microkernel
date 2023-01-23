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

#include <nanvix/hal.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/const.h>
#include <nanvix/hlib.h>
#include <posix/errno.h>

/**
 * @brief Reference count for kernel pages.
 */
PRIVATE int kpages[NUM_KPAGES] = { 0,  };

PUBLIC int *kpages_alias = &kpages[0];

/*============================================================================*
 * kpage_get()                                                                *
 *============================================================================*/

/**
 * The kpage_get() function allocates a page of the kernel page pool.
 * If @p clean is non zero, the page is filled with zeros before it is
 * handled to the caller. The call fails if there are no free kernel
 * pages available in the pool.
 *
 * @retval NULL Cannot allocate a kernel page.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC void *kpage_get(int clean)
{
	void *kpg;  /* Kernel page. */
	unsigned i; /* Loop index.  */

	/* Search for a free kernel page. */
	for (i = 0; i < NUM_KPAGES; i++)
	{
		/* Found it. */
		if (kpages[i] == 0)
			goto found;
	}

	kprintf("[kernel][mm] kernel page pool overflow");

	return (NULL);

found:

	/* Set page as used. */
	kpg = (void *) kpool_id_to_addr(i);
	kpages[i]++;
	dcache_invalidate();

	/* Clean page. */
	if (clean)
		kmemset(kpg, 0, PAGE_SIZE);

	return (kpg);
}

/*============================================================================*
 * kpage_put()                                                                *
 *============================================================================*/

/**
 * The kpage_put() function releases the kernel page pointed to by @p
 * kpg. If the @p kpg does not point to a valid kernel page nor it
 * reffers to a page that is currently used, the call fails.
 *
 * @retval -EVAINL Invalid kernel page.
 * @retval -EFAULT Target kernel page not allocated.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC int kpage_put(void *kpg)
{
	int i;

	/* Invalid address. */
	if (!kpool_is_kpage(VADDR(kpg)))
	{
		kprintf("mm: not a kernel page");
		return (-EINVAL);
	}

	i = kpool_addr_to_id(VADDR(kpg));

	/* Double free. */
	if (kpages[i] == 0)
	{
		kprintf("[kernel][mm] double free on kernel page");
		return (-EFAULT);
	}

	kpages[i]--;
	dcache_invalidate();

	return (0);
}

/*============================================================================*
 * kpool_init()                                                               *
 *============================================================================*/

/**
 * The kpool_init() function initializes internal structures of the
 * kernel page allocator. Additionally, if the kernel is compiled
 * without the @p NDEBUG build option, unit tests on the kernel page
 * allocator are launched once its initialization is completed.
 *
 * @author Pedro Henrique Penna
 */
PUBLIC void kpool_init(void)
{
	kprintf("[kernel][mm] initializing the kernel page allocator");

#ifndef __NANVIX_FAST_BOOT
	for (unsigned i = 0; i < NUM_KPAGES; i++)
		kpages[i] = 0;
#endif

	#ifndef __SUPPRESS_TESTS
		kprintf("[kernel][mm] running tests on the kernel page allocator");
		kpool_test_driver();
	#endif
}
