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
#include <nanvix/const.h>
#include <nanvix/hlib.h>

/*============================================================================*
 * semaphore_down()                                                           *
 *============================================================================*/

/**
 * The semaphore_down() function performs a down operation in the
 * semaphore pointed to by @p sem. It atomically checks the current
 * value of @p sem. If it is greater than one, it decrements the
 * semaphore counter by one and the calling thread continue its
 * execution, flow as usual.  Otherwise, the calling thread sleeps
 * until another thread performs a call to semaphore_up() on this
 * semaphore.
 *
 * @see SEMAPHORE_INIT(), semaphore_up()
 */
PUBLIC void semaphore_down(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_lock(&sem->lock);

		while (true)
		{
				if (sem->count > 0)
					break;

			cond_wait(&sem->cond, &sem->lock);
		}

		sem->count--;

	spinlock_unlock(&sem->lock);
}

/*============================================================================*
 * semaphore_up()                                                             *
 *============================================================================*/

/**
 * The semaphore_up() function performs an up operation in a semaphore
 * pointed to by @p sem. It atomically increments the current value of
 * @p and wakes up all threads that were sleeping in this semaphore,
 * waiting for a semaphore_up() operation.
 *
 * @see SEMAPHORE_INIT(), semaphore_down()
 */
PUBLIC void semaphore_up(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_lock(&sem->lock);
		sem->count++;
		cond_anycast(&sem->cond);
	spinlock_unlock(&sem->lock);
}
