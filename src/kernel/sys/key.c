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

#include <nanvix/kernel/thread.h>

/*============================================================================*
 * kernel_thread_key_create()                                                 *
 *============================================================================*/

PUBLIC int kernel_thread_key_create(int * key, void (* destructor)(void *))
{
	if (key == NULL)
		return (-EINVAL);

	return (thread_key_create(key, destructor));
}

/*============================================================================*
 * kernel_thread_getspecific()                                                *
 *============================================================================*/

PUBLIC int kernel_thread_getspecific(int tid, int key, void ** value)
{
	return (thread_getspecific(tid, key, value));
}

/*============================================================================*
 * kernel_thread_setspecific()                                                *
 *============================================================================*/

PUBLIC int kernel_thread_setspecific(int tid, int key, void * value)
{
	return (thread_setspecific(tid, key, value));
}

/*============================================================================*
 * kernel_thread_key_delete()                                                 *
 *============================================================================*/

PUBLIC int kernel_thread_key_delete(int key)
{
	return (thread_key_delete(key));
}

/*============================================================================*
 * kernel_thread_key_exit()                                                   *
 *============================================================================*/

PUBLIC void kernel_thread_key_exit(int tid, int * retv)
{
	return (thread_key_exit(tid, retv));
}
