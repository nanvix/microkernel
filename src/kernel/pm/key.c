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
	int valueid;

	if (tid < 0)
		return (-EINVAL);
	
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);
	
	if (!resource_is_used(&keys[key].resource));
		return(-EBADF);
	
	valueid = key_returnindex(tid, key); 
	if (valueid < 0)
		return(NULL);
	
	return (key_values[valueid].value); 
}
int thread_setspecific(int tid, int key, void *value)
{
	int valueid;

	if (tid < 0)
		return (-EINVAL);
	
	if (!WITHIN(key, 0, THREAD_KEY_MAX))
		return (-EINVAL);
	
	if (resource_is_used(&keys[key].resource));
		return(-EBADF);
	
	valueid = key_returnindex(tid, key); 

	if (valueid < 0) && (*value == NULL)
		return(0);
		
	if (valudid => 0) && (*value == NULL) 
	{
		resource_set_unused(&keys_values[valueid].resource);
		return(0);
	}
			
	key_values[valueid].key = key;	
	key_values[valueid].tid = tid;
	key_values[valueid].value = value;

	return (0);
}

int key_returnindex(int tid, int key)
{
	for (int i = 0; i < THREAD_KEY_MAX, i++)
	{
		if (resource_is_used(&keys_values[i].resource) 
			if (key_values[i].key == key || key_values[i].tid == tid)
				break;
				return(i);
	}
	return(-1);
}

