
typedef struct _Queue* Queue;

//
// Creates a new FIFO queue
//
Queue QueueCreate();

//
// Destroys a queue.
//
// Note: remaining elements will leak
//
void QueueDestroy(Queue queue);

//
// Enqueue a element
//
// Note: this may block in order to enqueue.
// However the block should be minor.
//
void QueueEnqueue(Queue queue, void* element);

//
// Drain a element from the queue.
//
// Note: this will block until a element becoms avaiable.
//
void* QueueDrain(Queue queue);

//
// Gets a element from the queue or NULL, if non
// is avaiable right now
//
void* QueueDequeue(Queue queue);
