#define __NEED_RESOURCE

#include <nanvix/kernel/thread.h>

#define THREAD_KEY_MAX 32


struct thread_key
{
	struct resource resource;

	void (*destructor)(*void);
} keys[THREAD_KEY_MAX];

struct thread_key_value 
{
	struct resource resource;

	int key;
	int tid;
	void *value;
} key_values[THREAD_KEY_MAX];

PRIVATE const struct resource_pool keyspool = {
	keys, (THREAD_KEY_MAX), sizeof(struct thread_key)
};

PRIVATE const struct resource_pool keys_valuepool = {
	key_values, (THREAD_KEY_MAX), sizeof(struct thread_key_value)
};

int thread_key_create(int *key, void (*destructor)(*void)) 
{
	
	if (key == NULL)
		return (-EINVAL);
	
	if (keyid = resource_alloc(&keyspool) < 0)
		return (-EAGAIN);

	*key = keyid;
	keys[keyid].destructor = destructor;
	
	return (0);
}

void *thread_getspecific(int tid, int key)
{
	
	if (tid < 0)
		return (-EINVAL);
	
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);
	
	if (!resource_is_used(&keys[key].resource));
		return(-EBADF);

	if (valueid = key_returnindex(tid, key) < 0)
		return(NULL);
	
	return (key_values[valueid].value); 
}
int thread_setspecific(int tid, int key, void *value)
{
	if (tid < 0)
		return (-EINVAL);
	
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);
	
	if (resource_is_used(&keys[key].resource));
		return(-EBADF);

	if (valueid = key_returnindex(tid, key) < 0)
		return(NULL);
		
	
	key_values[valueid].key = key;	
	key_values[valueid].tid = tid;
	key_values[valueid].value = value;

	return (0);
}

int key_returnvalid(int tid, int key)
{
	for (int i = 0; i < THREAD_KEY_MAX, i++)

		if (key_values[i].key == key || key_values[i].tid == tid)
			break;
			return(i);
	
	return(-1);
}

