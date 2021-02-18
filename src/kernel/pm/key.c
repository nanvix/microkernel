#include <nanvix/kernel/thread.h>
#define THREAD_KEY_MAX 32

struct thread_key_complete
{
	struct thread_key key;
	int tid;
} keys[THREAD_KEY_MAX];

int thread_key_create(thread_key *key, void (*destructor)(*void)) 
{
	UNUSED(destructor);
	KASSERT(key != NULL);

	key->key->id = -1;
	key->tid = -1;
	key->key->value = NULL;

	return (0);
}

int thread_getspecific((struct thread_key key))
{

}

int thread_setspecific(struct thread_key key, void *value)
{

