#include "dictionary.h"
#include "retainable.h"
#include "stack.h"

#include <string.h>
#include <stdlib.h>

typedef struct _DictionaryEntry* DictionaryEntry;

struct _DictionaryEntry {
	Retainable retainable;
	
	const char* key;
	const void* value;
	
	DictionaryEntry leftEntry;
	DictionaryEntry rightEntry;
};

static DictionaryEntry DictionaryEntryCreate(const char* key, const char* value);
static void DictionaryEntrySetLeftEntry(DictionaryEntry entry, DictionaryEntry left);
static void DictionaryEntrySetRightEntry(DictionaryEntry entry, DictionaryEntry right);
static void DictionaryEntryDealloc(void* ptr);

struct _DictionaryIterator {
	Retainable retainable;
	
	Dictionary dictionary;
	DictionaryEntry currentEntry;
	Stack stack;
};

static void DictionaryIteratorDealloc(void* ptr);

struct _Dictionary {
	Retainable retainable;
	
	DictionaryEntry headEntry;
};

static void DictionaryDealloc(void* ptr);

Dictionary DictionaryCreate()
{
	Dictionary dict = malloc(sizeof(struct _Dictionary));
	
	RetainableInitialize(&dict->retainable, DictionaryDealloc);
	
	dict->headEntry = DictionaryEntryCreate("", NULL);
	
	return dict;
}

void* DictionaryGet(Dictionary dict, const char* key)
{
	DictionaryEntry entry = dict->headEntry;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0)
			return (void*)entry->value;
		else if (cmp < 0)
			entry = entry->leftEntry;
		else
			entry = entry->rightEntry;
	}
	
	return NULL;
}

void DictionarySet(Dictionary dict, const char* key, const void* value)
{
	DictionaryEntry entry = dict->headEntry;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0) {
			entry->value = value;
			return;
		}
		else if (cmp < 0) {
			if (entry->leftEntry)
				entry = entry->leftEntry;
			else {
				DictionaryEntrySetLeftEntry(entry, DictionaryEntryCreate(key, value));
				return;
			}
		}
		else {
			if (entry->rightEntry)
				entry = entry->rightEntry;
			else {
				DictionaryEntrySetRightEntry(entry, DictionaryEntryCreate(key, value));
				return;
			}
		}
	}
}

void DictionaryDealloc(void* ptr)
{
	Dictionary dict = ptr;
	
	Release(dict->headEntry);
	free(dict);
}

static DictionaryEntry DictionaryEntryCreate(const char* key, const char* value)
{
	DictionaryEntry entry = malloc(sizeof(struct _DictionaryEntry));
	
	memset(entry, 0, sizeof(struct _DictionaryEntry));
	
	RetainableInitialize(&entry->retainable, DictionaryEntryDealloc);
	
	entry->key = key;
	entry->value = value;
	
	return entry;
}

static void DictionaryEntrySetLeftEntry(DictionaryEntry entry, DictionaryEntry left)
{
	Release(entry->leftEntry);
	entry->leftEntry = Retain(left);
}

static void DictionaryEntrySetRightEntry(DictionaryEntry entry, DictionaryEntry right)
{
	Release(entry->rightEntry);
	entry->rightEntry = Retain(right);
}

static void DictionaryEntryDealloc(void* ptr)
{
	DictionaryEntry entry = ptr;
	
	Release(entry->leftEntry);
	Release(entry->rightEntry);
	free(ptr);
}

DictionaryIterator DictionaryGetIterator(Dictionary dict)
{
	DictionaryIterator iter = malloc(sizeof(struct _DictionaryIterator));
	
	memset(iter, 0, sizeof(struct _DictionaryIterator));
	RetainableInitialize(&iter->retainable, DictionaryIteratorDealloc);
	
	iter->stack = StackCreate();
	iter->dictionary = Retain(dict);
	
	// Push the head dummy entry
	StackPush(iter->stack, dict->headEntry);
	
	// Now advance twice to get to the first real entry
	DictionaryIteratorNext(iter);
	DictionaryIteratorNext(iter);
	
	return iter;
}

char* DictionaryIteratorGetKey(DictionaryIterator iter)
{
	if (!iter->currentEntry)
		return NULL;
	
	return (char*)iter->currentEntry->key;
}

void* DictionaryIteratorGetValue(DictionaryIterator iter)
{
	if (!iter->currentEntry)
		return NULL;
	
	return (void*)iter->currentEntry->value;
}

bool DictionaryIteratorNext(DictionaryIterator iter)
{
	iter->currentEntry = StackPop(iter->stack);
	
	if (!iter->currentEntry)
		return false;
	
	if (iter->currentEntry->leftEntry)
		StackPush(iter->stack, iter->currentEntry->leftEntry);
	if (iter->currentEntry->rightEntry)
		StackPush(iter->stack, iter->currentEntry->rightEntry);
	
	return true;
}

static void DictionaryIteratorDealloc(void* ptr)
{
	DictionaryIterator iter = ptr;
	
	Release(iter->dictionary);
	Release(iter->stack);
	free(iter);
}
