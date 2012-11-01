#include "stack.h"

#include "retainable.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct _Stack {
	Retainable retainable;
	
	void** stack;
	int32_t uppermostObject;
	uint32_t slots;
};

static void StackDealloc(void* ptr);

Stack StackCreate()
{
	Stack stack = malloc(sizeof(struct _Stack));
	
	memset(stack, 0, sizeof(struct _Stack));
	RetainableInitialize(&stack->retainable, StackDealloc);
	
	stack->uppermostObject = -1;
	stack->slots = 2;
	stack->stack = malloc(sizeof(void*) * stack->slots);
	
	return stack;
}

void StackPush(Stack stack, void* object)
{
	// Expand the stack
	if (stack->uppermostObject == (int32_t)stack->slots) {
		stack->slots *= 2;
		stack->stack = realloc(stack->stack, sizeof(void*) * stack->slots);
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
	
	free(stack->stack);
	free(stack);
}
