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
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/excp.h>
#include <nanvix/const.h>
#include <nanvix/hal.h>
#include <posix/errno.h>

#if (THREAD_MAX > 1)

/**
 * @brief Lock for this module.
 */
PRIVATE spinlock_t lock;

/**
 * @brief Lock for user-space handler.
 */
PRIVATE struct semaphore ulock;

/**
 * @brief Current exception being handled.
 */
PRIVATE struct
{
	int excpnum; /**< Numer of the exception.              */
	int coreid;  /**< Core ID where the thread is running. */
} handling = { -1, - 1 };

/**
 * @brief Action on kernel exceptions.
 */
PRIVATE struct
{
	/**
	 * @brief Action on exception.
	 */
	int action;

	/**
	 * @brief Waiting threads.
	 */
	struct
	{
		struct thread *thread;        /**< Thread                          */
		const struct exception *excp; /**< Information About the Exception */
		struct semaphore lock;        /**< Kernel-Space Lock               */
	} waiting[THREAD_MAX] ALIGN(sizeof(dword_t));
} kexceptions[EXCEPTIONS_NUM];

/**
 * @todo TODO Provide a detailed description for this function.
 */
PUBLIC int exception_control(int excpnum, int action)
{
	/* Sanity check. */
	KASSERT(exception_is_valid(excpnum));
	KASSERT((action == EXCP_ACTION_IGNORE) || (action == EXCP_ACTION_HANDLE));

	spinlock_lock(&lock);

		/* Are there any blocked threads? */
		if (action == EXCP_ACTION_IGNORE)
		{
			for (int i = 0; i < THREAD_MAX; i++)
			{
				/* Yes, so we cannot proceed. */
				if (kexceptions[excpnum].waiting[i].thread != NULL)
				{
					kprintf("[kernel][excp] cannot igore exception");
					spinlock_unlock(&lock);
					return (-EPERM);
				}
			}
		}

		kexceptions[excpnum].action = action;

	spinlock_unlock(&lock);

	return (0);
}

/**
 * @todo TODO: Provide a detailed description for this function.
 */
PRIVATE int exception_grab(void)
{
	/* Grab an unhandled exception. */
	for (int i = 0; i < EXCEPTIONS_NUM; i++)
	{
		if (kexceptions[i].action == EXCP_ACTION_HANDLE)
		{
			for (int j = 0; j < THREAD_MAX; j++)
			{
				if (kexceptions[i].waiting[j].thread != NULL)
				{
					handling.excpnum = i;
					handling.coreid = j;
					return (1);
				}
			}
		}
	}

	return (0);
}

/**
 * @todo TODO Provide a detailed description for this function.
 */
PUBLIC int exception_pause(struct exception *excp)
{
	spinlock_lock(&lock);

		/* Check if any exception is being handled.*/
		for (int i = 0; i < EXCEPTIONS_NUM; i++)
		{
			if (kexceptions[i].action == EXCP_ACTION_HANDLE)
				goto found;
		}

	spinlock_unlock(&lock);

	return (-EPERM);

found:

		/* Wait for any kernel-space handler. */
		while (!exception_grab())
		{
			spinlock_unlock(&lock);
				semaphore_down(&ulock);
			spinlock_lock(&lock);
		}

	if (excp != NULL)
	{
		kmemcpy(excp,
			kexceptions[handling.excpnum].waiting[handling.coreid].excp,
			sizeof(struct exception)
		);
	}

	spinlock_unlock(&lock);

	return (0);
}

/**
 * @todo TODO Provide a detailed description for this function.
 */
PUBLIC int exception_resume(void)
{
	/* No current exception. */
	if ((handling.excpnum < 0) || (handling.coreid < 0))
		return (-EINVAL);

	spinlock_lock(&lock);

		/* Check if exception is being ignored. */
		if (kexceptions[handling.excpnum].action == EXCP_ACTION_IGNORE)
		{
			spinlock_unlock(&lock);
			return (-EINVAL);
		}

		/* Resume thread. */
		if (kexceptions[handling.excpnum].waiting[handling.coreid].thread == NULL)
		{
			spinlock_unlock(&lock);
			return (-EINVAL);
		}

		kexceptions[handling.excpnum].waiting[handling.coreid].thread = NULL;
		kexceptions[handling.excpnum].waiting[handling.coreid].excp = NULL;
		semaphore_up(&kexceptions[handling.excpnum].waiting[handling.coreid].lock);

		handling.excpnum = -1;
		handling.coreid = -1;

	spinlock_unlock(&lock);

	return (0);
}

/**
 * @todo TODO Provide a detailed description for this function.
 */
PUBLIC int exception_wait(int excpnum, const struct exception *excp)
{
	int coreid;
	struct thread *t;

	UNUSED(excp);

	/* Invalid exception. */
	KASSERT(exception_is_valid(excpnum));

	t = thread_get_curr();
	coreid = thread_get_coreid(t);

	spinlock_lock(&lock);

		/* Chek if exception is being ignored. */
		if (kexceptions[excpnum].action == EXCP_ACTION_IGNORE)
		{
			spinlock_unlock(&lock);
			kprintf("[kernel][excp] exception %d ignored", excpnum);
			return (0);
		}

		kexceptions[excpnum].waiting[coreid].thread = t;
		kexceptions[excpnum].waiting[coreid].excp = excp;

	spinlock_unlock(&lock);

	/*
	 * XXX: Unlock user-space handler and wait. Note that here we have a
	 * race condition.  If the user-space handler executes faster than
	 * the current thread, we may lost the unlock() operation. Hopefully
	 * it will take more time, because the handler should return to
	 * user-space from exception_pause() and then come back by calling
	 * exception_resume().
	 */
	semaphore_up(&ulock);
	semaphore_down(&kexceptions[excpnum].waiting[coreid].lock);

	return (0);
}

/**
 * @todo TODO Provide a detailed description for this function.
 */
PUBLIC void exception_init(void)
{
	static bool initialized = false;

	KASSERT(!initialized);

	spinlock_init(&lock);

	for (int i = 0; i < EXCEPTIONS_NUM; i++)
	{
		kexceptions[i].action = EXCP_ACTION_IGNORE;
		for (int j = 0; j < THREAD_MAX; j++)
		{
			kexceptions[i].waiting[j].thread = NULL;
			semaphore_init(&kexceptions[i].waiting[j].lock, 0);
		}
	}

	semaphore_init(&ulock, 0);
	handling.excpnum = -1;
	handling.coreid = -1;

	initialized = true;
}

#else
extern int make_iso_compilers_happy;
#endif /* THREAD_MAX > 1*/
