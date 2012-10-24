
//
// Initialize the worker threads subsystem
//
void WorkerThreadsInitialize(int maxNumThreads);

//
// Enqueue an operation
//
typedef void(*Worker)(void* userInfo);
void WorkerThreadsEnqueue(Worker worker, void* userInfo);

