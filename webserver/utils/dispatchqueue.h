
#ifndef _DISPATCH_QUEUE_H_
#define _DISPATCH_QUEUE_H_

typedef struct _DispatchQueue* DispatchQueue;

typedef enum {
	//
	// Instead of multiple worker threads the
	// queue will use only one thread and therefor
	// ensures the correct order of execution
	//
	kDispatchQueueSerial = (1 << 1)
} DispatchQueueFlags;

//
// Creates a new dispatch queue.
//
// Is a Retainable
//	
DispatchQueue DispatchQueueCreate(DispatchQueueFlags flags);

//
// Enqueue a block to be dispatched.
//
void Dispatch(DispatchQueue queue, void(^block)());

#endif /* _DISPATCH_QUEUE_H_ */
