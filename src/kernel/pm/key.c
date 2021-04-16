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
	UNUSED(destructor);
	if (key == NULL)
		return(-EINVAL);
	
	if (keyid = resource_alloc(&keyspool) < 0)
		return (-EAGAIN);

	if ((keyid >= 0)
		*key = keyid;
	
	return (0);
}

void *thread_getspecific(int tid, int key)
{
	for (int i = 0; i < THREAD_KEY_MAX; ++1)
	{
		if (key->tid = keys[i]->tid || key->key->value == keys[i]->key->value)
		{
			return (value);
			break;	
		}
		else 
		{
			return (NULL);
		}	
	}

}
int thread_setspecific(int tid, int key, void *value)
{
	for (int i = 0; i < THREAD_KEY_MAX; ++i)
	{
		if (keys[i]->tid == -1 || keys[i]->key->id == -1)
			{
			keys[i]->key->id = 0;	
			keys[i]->tid = thread_get_curr_id();
			keys[i]->key->value = value;
			}
	}
	return (0);
}

int key_isvalid(int key) 
{

