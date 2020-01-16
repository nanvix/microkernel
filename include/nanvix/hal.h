/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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

#ifndef NANVIX_HAL_H_
#define NANVIX_HAL_H_

	/* Must come first. */
	#undef  __NEED_HAL
	#define __NEED_HAL

	#include <nanvix/hal/hal.h>
	#include <nanvix/kernel/config.h>

#ifdef __NANVIX_HAS_NETWORK
	#include <net/mailbox.h>

	#if (PROCESSOR_IS_MULTICLUSTER == 0)
		#include <net/clusters.h>
	#endif

	#if (PROCESSOR_HAS_NOC == 0)
		#include <net/noc.h>
	#endif

	#if defined(NUMBER_OF_GUESTS) && (NUMBER_OF_GUESTS > 1) \
		 && (PROCESSOR_HAS_NOC == 0)
		#undef PROCESSOR_HAS_NOC
		#define PROCESSOR_HAS_NOC 1
	#endif
#endif

#endif /* NANVIX_HAL_H_ */
