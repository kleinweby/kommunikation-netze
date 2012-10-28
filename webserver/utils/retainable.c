#include "retainable.h"

#include <assert.h>

void RetainableInitialize(void* ptr, void (*Destroy)(void* ptr)) {
	Retainable* retainable = ptr;
	
	retainable->retainCount = 1;
	retainable->Destroy = Destroy;
}

void* Retain(void* ptr) {
	Retainable* retainable = ptr;
	int rc;
	
	assert(retainable->retainCount > 0);
	
	rc = __sync_add_and_fetch(&retainable->retainCount, 1);
	
	assert(rc > 0);
	
	return ptr;
}

void Release(void* ptr) {
	Retainable* retainable = ptr;
	int rc;
	
	assert(retainable->retainCount > 0);
	
	rc = __sync_sub_and_fetch(&retainable->retainCount, 1);
	
	assert(rc >= 0);
	
	if (rc == 0 && retainable->Destroy) {
		retainable->Destroy(ptr);
	}
}
