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

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <stdbool.h>
#include <pthread.h>

//
// Internal opaque object structure. Do NOT use
// it in any way.
//
struct _Object {
	//
	// A magic to look if its really an object
	//
	char magic[4];
	
	//
	// Count how many objects hold a reference to this
	// retainable.
	//
	int retainCount;
	//
	// Is called when retain Count reaches zero
	//
	void (*Dealloc)(void* ptr);
	
	//
	// A mutex to lock the whole object gobally
	//
	pthread_mutex_t mutex;
};

//
// Call this to declare that a class with the name A
// exists
//
#define DECLARE_CLASS(A) typedef struct _##A* A __attribute__((NSObject))

//
// Call this (usually in the implementation) to define your
// class and this member variables
//
#define DEFINE_CLASS(A, x)\
struct _##A {\
	struct _Object object;\
	x\
}

//
// Some helper methods to declare ownership, so
// that it can be automaticly tested.
//

//
// Attributes an function that returns an object
// with rc +1
//
// Object creation methods should have this
//
#define OBJECT_RETURNS_RETAINED __attribute__((ns_returns_retained))

//
// Attributes an function that returns an object with rc -1
//
#define OBJECT_CONSUME __attribute__((ns_consumed))

//
// Declares a gerneral object class
//
DECLARE_CLASS(Object);

//
// Call this before you use any object, this will
// initialize the runtime elements of the object
// structure (mainly the blocks integration).
//
bool ObjectRuntimeInit();

//
// Call this at the beginning of the object creation
// it will set the retaincount to 1.
//
bool ObjectInit(void* object, void (*Dealloc)(void* ptr));

//
// Increases the retain count by 1.
//
// Note: this will crash when the retain count is < 1
//
OBJECT_RETURNS_RETAINED
Object _Retain(Object object);

//
// Releases the reference to this object.
//
void _Release(Object OBJECT_CONSUME object);

//
// Typecorrect retain and release
//
#define Retain(...) ((__typeof(__VA_ARGS__)) _Retain((void*)(__VA_ARGS__)))
#define Release(...) _Release((void *)(__VA_ARGS__))

//
// Locks the given object
//
void Lock(void* object);

//
// Unlock the given object
//
void Unlock(void* object);


#endif
