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

#include "port.h"

#if (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL)

/*============================================================================*
 * portpool_choose_port()                                                     *
 *============================================================================*/

/**
 * @brief Searches for a free port on @p pool.
 *
 * @param pool Port pool.
 *
 * @returns Upon successful completion, the index of the available port is
 * returned. A negative number is returned instead.
 */
PUBLIC int portpool_choose_port(const struct port_pool * pool)
{
	int ret; /* Return value. */

	KASSERT(pool != NULL);
	ret = (-EINVAL);

	/* Checks if can exist an available port. */
	if (pool->used_ports < pool->nports)
	{
		/* Searches for a free port on the target mailbox. */
		for (int i = 0; i < pool->nports; ++i)
		{
			if (resource_is_used(&pool->ports[i].resource))
				continue;

			ret = i;
			break;
		}
	}

	return (ret);
}

#endif /* (__TARGET_HAS_MAILBOX || __TARGET_HAS_PORTAL) */
