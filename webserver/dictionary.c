#include "dictionary.h"

#include <string.h>
#include <stdlib.h>

struct _DictionaryEntry {
	const char* key;
	void* value;
	
	struct _DictionaryEntry* leftEntry;
	struct _DictionaryEntry* rightEntry;
};

struct _Dictionary {
	struct _DictionaryEntry* headEntry;
};

static void DictionaryEntryDestroy(struct _DictionaryEntry* entry);

Dictionary DictionaryCreate(size_t sizeHint)
{
	Dictionary dict = malloc(sizeof(struct _Dictionary));
	
	dict->headEntry = malloc(sizeof(struct _DictionaryEntry));
	dict->headEntry->key = "";
	dict->headEntry->leftEntry = NULL;
	dict->headEntry->rightEntry = NULL;
	
	return dict;
}

void* DictionaryGet(Dictionary dict, const char* key)
{
	struct _DictionaryEntry* entry = dict->headEntry;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0)
			return entry->value;
		else if (cmp < 0)
			entry = entry->leftEntry;
		else
			entry = entry->rightEntry;
	}
	
	return NULL;
}

void DictionarySet(Dictionary dict, const char* key, void* value)
{
	struct _DictionaryEntry* entry = dict->headEntry;
	
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
				entry->leftEntry = malloc(sizeof(struct _DictionaryEntry));
				entry = entry->leftEntry;
				entry->key = key;
				entry->value = value;
				entry->leftEntry = NULL;
				entry->rightEntry = NULL;
				return;
			}
		}
		else {
			if (entry->rightEntry)
				entry = entry->rightEntry;
			else {
				entry->rightEntry = malloc(sizeof(struct _DictionaryEntry));
				entry = entry->rightEntry;
				entry->key = key;
				entry->value = value;
				entry->leftEntry = NULL;
				entry->rightEntry = NULL;
				return;
			}
		}
	}
}

void DictionaryDestroy(Dictionary dict)
{
	DictionaryEntryDestroy(dict->headEntry);
	free(dict);
}

static void DictionaryEntryDestroy(struct _DictionaryEntry* entry)
{
	if (!entry)
		return;
	
	DictionaryEntryDestroy(entry->leftEntry);
	DictionaryEntryDestroy(entry->rightEntry);
	free(entry);
}