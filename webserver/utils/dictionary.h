#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <stdbool.h>

typedef struct _Dictionary* Dictionary;
typedef struct _DictionaryIterator* DictionaryIterator;

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

//
// Gets a iterator. The order which will be iterated
// is implemation detail.
//
DictionaryIterator DictionaryGetIterator(Dictionary dict);

//
// Get the key of the current object
//
char* DictionaryIteratorGetKey(DictionaryIterator iter);

//
// Get the value of the current object
//
void* DictionaryIteratorGetValue(DictionaryIterator iter);

//
// Advance to the next object
//
// Returns false when the end is reached
//
bool DictionaryIteratorNext(DictionaryIterator iter);

#endif /* _DICTIONARY_H_ */
