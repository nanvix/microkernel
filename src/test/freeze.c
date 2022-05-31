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

#include <nanvix/kernel/config.h>
#include <nanvix/kernel/syscall.h>
#include "test.h"


#define DELAY 5000000

static struct task task;
int stop = 1;

static int task_work(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
);

static int task_work(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg0);
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	int delay = DELAY;

	nanvix_puts("task working");

	while(delay--);

	nanvix_puts("freezing");
	kcall0(NR_freeze);

	delay = 500;
	while(delay--)
		nanvix_puts("task");

	nanvix_puts("unfreezing");
	kcall0(NR_unfreeze);

	delay = DELAY / 2;
	while(delay--);

	nanvix_puts("exiting");
	stop = 0;

	return 0;
}

void *thread_work(void *arg) {
	UNUSED(arg);
	do { nanvix_puts("I'm alive!"); } while (stop);
	return NULL;
}

void do_test_freeze(void)
{
	// int ret;
	int tids[NTHREADS - 1];
	void **retval = NULL;
	word_t arg = NULL;

	for (int i = 0; i < NTHREADS - 1; i++)
	{
		kcall3(
			NR_thread_create,
			(word_t) &tids[i],
			(word_t) thread_work,
			(word_t) NULL
		);
		// thread_create(&tids[i], thread_work, NULL);
	}
	task_create(&task, task_work, 0, TASK_MANAGEMENT_DEFAULT);
	task_dispatch(&task, arg, arg, arg, arg, arg);

	for (int i = 0; i < NTHREADS - 1; i++)
	{
		kcall2(
			NR_thread_join,
			(word_t) tids[i],
			(word_t) retval
		);
		// thread_join(tids[i], retval);
	}
}

static struct
{
	void (*test_fn)(); /**< Test function.     */
	const char *type;  /**< Name of test type. */
	const char *name;  /**< Test Name.         */
} freeze_tests[] = {
	{ do_test_freeze, "api", "freeze/unfreeze" },
	{ NULL,        NULL,  NULL                      }
};

PUBLIC void test_freeze(void)
{
	for (int i = 0; freeze_tests[i].test_fn != NULL; i++)
	{
		freeze_tests[i].test_fn();
		kprintf("[test][%s] %s [passed]", freeze_tests[i].type, freeze_tests[i].name);
	}
}
