#define __NEED_RESOURCE

#include <nanvix/kernel/thread.h>

#define THREAD_KEY_MAX 32

PRIVATE struct thread_key
{
	struct resource resource;

	void (* destructor)(* void);
} keys[THREAD_KEY_MAX];

PRIVATE struct thread_key_value
{
	struct resource resource;

	int key;
	int tid;
	void * value;
} key_values[THREAD_KEY_MAX];

PRIVATE const struct resource_pool keyspool = {
	keys, (THREAD_KEY_MAX), sizeof(struct thread_key)
};

PRIVATE const struct resource_pool keys_valuepool = {
	key_values, (THREAD_KEY_MAX), sizeof(struct thread_key_value)
};

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

PRIVATE int thread_key_search_value(int tid, int key)
{
	for (int i = 0; i < THREAD_KEY_MAX, ++i)
	{
		/**/
		if (!resource_is_used(&keys_values[i].resource))
			continue;

		/**/
		if (key_values[i].key != key || key_values[i].tid != tid)
			continue;

		/* I Found. */
		return (i);
	}

	return (-1);
}
		    
PUBLIC void * thread_getspecific(int tid, int key)
{
	int valueid;

	if (tid < 0)
		return (-EINVAL);

	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);

	if (!resource_is_used(&keys[key].resource));
		return (-EBADF);

	if ((valueid = thread_key_search_value(tid, key)) < 0)
		return (NULL);
	
	return (key_values[valueid].value); 
}

PUBLIC int thread_setspecific(int tid, int key, void * value)
{
	int valueid;

	if (tid < 0)
		return (-EINVAL);

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
