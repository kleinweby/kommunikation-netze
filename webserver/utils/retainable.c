// Copyright (c) 2012, Christian Speich <christian@spei.ch>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
