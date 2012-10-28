
//
// Simple helper to make structs retainable and therefore
// to simplify the mmgt
//

//
// Incoperate the following struct as your first
// member of the struct you want to be retainable.
//

typedef struct {
	//
	// Count how many objects hold a reference to this
	// retainable.
	//
	int retainCount;
	//
	// Is called when retain Count reaches zero
	//
	void (*Destroy)(void* ptr);
} Retainable;

//
// Call this once when the struct you want to be reaintable
// is created. Will set rc to 1
//
void RetainableInitialize(void* ptr, void (*Destroy)(void* ptr));

//
// Will increase the rc by one.
//
void* Retain(void* ptr);

//
// Will decrease the rc and
// invoke destroy on rc=0
//
void Release(void* ptr);