#include "retainable.h"

#include <stdlib.h>
#include <assert.h>

void RetainableInitialize(void* ptr, void (*Dealloc)(void* ptr)) {
	Retainable* retainable = ptr;
	
	retainable->retainCount = 1;
	retainable->Dealloc = Dealloc;
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
	if (ptr == NULL)
		return;
	
	Retainable* retainable = ptr;
	int rc;
	
	assert(retainable->retainCount > 0);
	
	rc = __sync_sub_and_fetch(&retainable->retainCount, 1);
	
	assert(rc >= 0);
	
	if (rc == 0 && retainable->Dealloc) {
		retainable->Dealloc(ptr);
	}
}
