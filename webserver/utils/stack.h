
#ifndef _STACK_H_
#define _STACK_H_

typedef struct _Stack* Stack;

//
// Create a new stack
//
// Is a retainable
//
Stack StackCreate();

//
// Push a object on the stack.
//
void StackPush(Stack stack, void* object);

//
// Pop a object from the stack
//
void* StackPop(Stack stack);

#endif /* _STACK_H_ */
