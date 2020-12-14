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
#include <nanvix/kernel/dev.h>
#include <nanvix/kernel/excp.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/noc.h>
#include <nanvix/kernel/syscall.h>
#include <nanvix/kernel/event.h>
#include <nanvix/kernel/thread.h>
#include <nanvix/const.h>
#include <nanvix/hlib.h>

#include <dev/net/net.h>

EXTERN void do_kcall2(void);
EXTERN void ___start(int argc, const char *argv[], char **envp);

#if (CLUSTER_IS_MULTICORE)

/**
 * @brief Init thread.
 */
PRIVATE void *init(void *arg)
{
	int argc = 1;
	const char *argv[] = { "init", NULL };

	UNUSED(arg);

#if (CORES_NUM >= 2)

	___start(argc, argv, NULL);

#else

	UNUSED(argc);
	UNUSED(argv);

#endif

	/* Power down. */
	kernel_shutdown();
	UNREACHABLE();

	return (NULL);
}

#endif

/**
 * @brief Initializes the kernel.
 */
PUBLIC void kmain(int argc, const char *argv[])
{
#if (CLUSTER_IS_MULTICORE)
	int tid;
#endif

	UNUSED(argc);
	UNUSED(argv);

	hal_init();
	dev_init();
#if (THREAD_MAX > 1)
	exception_init();
#endif
	mm_init();
	noc_init();
	syscall_init();
	kevent_init();

#if __NANVIX_HAS_NETWORK
	network_setup();
#endif

#if __NANVIX_USE_TASKS
	task_init();
#endif

	thread_init();

	kprintf("[kernel][dev] enabling hardware interrupts");
	interrupts_enable();

#if (CLUSTER_IS_MULTICORE)
	thread_create(&tid, init, NULL);
	while (true)
		do_kcall2();
#else

	/* Power down. */
	kernel_shutdown();
	UNREACHABLE();

#endif
}
