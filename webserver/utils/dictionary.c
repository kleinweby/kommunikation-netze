#include "dictionary.h"
#include "retainable.h"

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
