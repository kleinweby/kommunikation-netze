
//
// Initialize the worker threads subsystem
//
void WorkerThreadsInitialize(int maxNumThreads);

//
// Enqueue an operation
//
typedef void(^WorkerBlock)();
void WorkerThreadsEnqueue(WorkerBlock block);

