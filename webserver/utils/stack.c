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

#include "stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

DEFINE_CLASS(Stack,	
	void** stack;
	int32_t uppermostObject;
	uint32_t slots;
);

static void StackDealloc(void* ptr);

Stack StackCreate()
{
	Stack stack = malloc(sizeof(struct _Stack));
	
	if (stack == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(stack, 0, sizeof(struct _Stack));
	ObjectInit(stack, StackDealloc);
	
	stack->uppermostObject = -1;
	stack->slots = 2;
	stack->stack = malloc(sizeof(void*) * stack->slots);
	
	if (stack->stack == NULL) {
		perror("malloc");
		Release(stack);
		return NULL;
	}
	
	return stack;
}

void StackPush(Stack stack, void* object)
{
	// Expand the stack
	if (stack->uppermostObject == (int32_t)stack->slots) {
		stack->slots *= 2;
		stack->stack = realloc(stack->stack, sizeof(void*) * stack->slots);
		assert(stack->stack != NULL);
	}
	
	stack->stack[++stack->uppermostObject] = object;
}

void* StackPop(Stack stack)
{
	if (stack->uppermostObject < 0)
		return NULL;
	
	return stack->stack[stack->uppermostObject--];
}

static void StackDealloc(void* ptr)
{
	Stack stack = ptr;
	
	if (stack->stack)
		free(stack->stack);
	free(stack);
}
