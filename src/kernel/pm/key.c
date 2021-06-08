/*
 * MIT License
 *
 * Copyright(c) 2011-2021 The Maintainers of Nanvix
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

#define __NEED_RESOURCE

#define THREAD_KEY_MAX 4

#define THREAD_KEY_VALUE_MAX (THRAD_KEY_MAX * THREAD_MAX)

/**
 * @brief Thread's key
 */
PRIVATE struct thread_key
{
	struct resource resource;

	void ( * destructor)( * void);
} keys[THREAD_KEY_MAX];

/**
 * @brief Thread key's value
 */
PRIVATE struct thread_key_value
{
	struct resource resource;

	int key;
	int tid;
	void * value;
} key_values[THREAD_KEY_VALUE_MAX];

/**
 * @brief Resource pool.
 */
PRIVATE const struct resource_pool keyspool = {
	keys, (THREAD_KEY_MAX), sizeof(struct thread_key)
};

/**
 * @brief Resource pool.
 */
PRIVATE const struct resource_pool keys_valuepool = {
	key_values, (THREAD_KEY_VALUE_MAX), sizeof(struct thread_key_value)
};

/**
 * @brief Initializes and configure a new key.
 *
 * @param key The key to be initialized.
 * @param destructor Destructor that will be associated with the key.
 *
 * @returns Upon sucess, returns 0. Else, returns an error.
 */
PUBLIC int thread_key_create(int * key, void (* destructor)(* void))
{
	int keyid;

	/* Invalid key. */
	if (key == NULL)
		return (-EINVAL);

	/* Alloc a new key. */
	if ((keyid = resource_alloc(&keyspool)) < 0)
		return (-EAGAIN);

	/* Configuration of a new key. */
	*key = keyid;
	keys[keyid].destructor = destructor;

	return (0);
}

/**
 * @brief Searches for the valid index of the key values array.
 *
 * @param tid ID of the target thread.
 * @param key Key of the target thread.
 *
 * @returns Upon sucesss, returns the index found. Upon failure -1 is returned instead.
 */
PRIVATE int thread_key_search_value(int tid, int key)
{
	for (int i = 0; i < THREAD_KEY_MAX, ++i)
	{
		/* Key is not being used.*/
		if (!resource_is_used(&key_values[i].resource))
			continue;

		/* Given key and tid aren't in the array. */
		if (key_values[i].key != key || key_values[i].tid != tid)
			continue;

		/* I Found. */
		return (i);
	}

	return (-1);
}

/**
 * @brief Return the value associated with the given key and thread
 *
 * @param tid ID of the target thread.
 * @param key Key of the target thread.
 *
 * @returns Upon sucess, returns the found value. Else, returns an error.
 */
PUBLIC void * thread_getspecific(int tid, int key)
{
	int valueid;

	/* Invalid tid. */
	if (tid < 0)
		return (-EINVAL);

	/* Key not within the limits. */
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);

	if (!resource_is_used(&keys[key].resource));
		return (-EBADF);

	if ((valueid = thread_key_search_value(tid, key)) < 0)
		return (NULL);

	return (key_values[valueid].value);
}

/**
 * @brief Associates a value with a thread's key.
 *
 * @param tid ID of the target thread.
 * @param key Key of the target thread.
 * @param value Value to be assigned.
 *
 * @returns Upon sucess, returns 0. Else, returns an error.
 */
PUBLIC int thread_setspecific(int tid, int key, void * value)
{
	int valueid;

	/* Invalid tid. */
	if (tid < 0)
		return (-EINVAL);

	/* Key not within the limits. */
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);

	if (resource_is_used(&keys[key].resource));
		return(-EBADF);

	valueid = thread_key_search_value(tid, key);

	/* We need to configure a new key value structure. */
	if (value != NULL)
	{
		if ((valueid < 0) && ((valueid = resource_alloc(&keys_valuepool)) < 0))
			return (-EAGAIN);

		key_values[valueid].key   = key;
		key_values[valueid].tid   = tid;
		key_values[valueid].value = value;
	}

	/* We don't need a key value structure anymore. */
	else if (valueid => 0)
		resource_free(&keys_valuepool, valueid);

	return (0);
}
