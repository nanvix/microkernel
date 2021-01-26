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

#include <nanvix/kernel/mm.h>

#if CLUSTER_HAS_SW_TLB_SHOOTDOWN

#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>

/**
 * @brief Shootdown local TLB entry.
 */
PRIVATE int __tlb_shootdown(struct task_args * args)
{
	if ((args->ret = tlb_shootdown_local((vaddr_t) args->arg0)) < 0)
	{
		args->ret = (-EAGAIN);
		return (TASK_RET_ERROR);
	}

	args->ret = 0;
	return (TASK_RET_SUCCESS);
}

/**
 * @brief Invalidates the TLB entry that encodes the virtual address @p
 * addr in all cores.
 *
 * @param addr Virtual address that represents the TLB entry.
 *
 * @returns Upon successful completion, zero is returned. Upon failure,
 * negative numver is returned instead.
 */
PRIVATE int tlb_sw_shootdown(vaddr_t vaddr)
{
	int mycoreid;
	struct task_args args;
	struct task shoots[CORES_NUM];

	mycoreid  = core_get_id();
	args.arg0 = (word_t) vaddr;
	args.ret  = 0;

	/* Creates and emits shoots to other cores. */
	for (int coreid = 0; coreid < CORES_NUM; ++coreid)
	{
		/* Creates task. */
		KASSERT(task_create(&shoots[coreid], __tlb_shootdown, &args, 0) == 0);

		if (UNLIKELY(coreid == mycoreid))
			continue;

		/* Emits task. */
		KASSERT(task_emit(&shoots[coreid], coreid) == 0);
	}

	/* Executes locally. */
	KASSERT(task_emit(&shoots[mycoreid], mycoreid) == 0);

	/* Waits tasks. */
	for (int coreid = 0; coreid < CORES_NUM; ++coreid)
	{
		KASSERT(task_wait(&shoots[coreid]) == 0);

		/* Return continues with success ? (Verify next return) : (Keep previous error) */
		if (args.ret == 0)
			args.ret = task_get_return(&shoots[coreid]);
	}

	return (args.ret);
}

/**
 * @brief Software TLB Shootdown configuration.
 */
PUBLIC void tlb_shootdown_config(void)
{
	KASSERT(tlb_set_shootdown_function(tlb_sw_shootdown) == 0);
}

#else

/**
 * @brief Software TLB Shootdown configuration.
 */
PUBLIC void tlb_shootdown_config(void)
{
	/* Already configured. */
}

#endif
