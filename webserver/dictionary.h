
typedef struct _Dictionary* Dictionary;

//
// Creates a new empty dictionary;
//
Dictionary DictionaryCreate();

//
// Gets the value associated with key
//
void* DictionaryGet(Dictionary dict, const char* key);

//
// Sets the value associated with key.
//
// Note you mus ensure that the key and the value
// is keept valid.
//
void DictionarySet(Dictionary dict, const char* key, const void* value);
