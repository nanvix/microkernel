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
 * @defgroup kernel-portal Portal Facility
 * @ingroup kernel
 *
 * @brief Portal Facility
 */

#ifndef NANVIX_KPORTAL_H_
#define NANVIX_KPORTAL_H_

	#include <nanvix/kernel/portal.h>

	#include "active.h"

#if __TARGET_HAS_PORTAL

	EXTERN int do_portal_alloc(int, int, int, int);
	EXTERN int do_portal_release(int);
	EXTERN ssize_t do_portal_aread(int, const struct active_config *, struct pstats *);
	EXTERN ssize_t do_portal_awrite(int, const struct active_config *, struct pstats *);
	EXTERN int do_portal_wait(int, const struct active_config *, struct pstats *);
	EXTERN int portal_laddress_calc(int, int);

	EXTERN void do_portal_init(void);

#endif /* __TARGET_HAS_PORTAL */

#endif /* NANVIX_KPORTAL_H_ */

/**@}*/
