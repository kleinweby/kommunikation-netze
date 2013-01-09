// Copyright (c) 2012, Christian Speich <christian@spei.ch>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "dictionary.h"
#include "stack.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

DECLARE_CLASS(DictionaryEntry);

DEFINE_CLASS(DictionaryEntry,	
	const char* key;
	const void* value;
	
	DictionaryEntry leftEntry;
	DictionaryEntry rightEntry;
);

OBJECT_RETURNS_RETAINED
static DictionaryEntry DictionaryEntryCreate(const char* key, const char* value);

static void DictionaryEntrySetLeftEntry(DictionaryEntry entry, DictionaryEntry left);
static void DictionaryEntrySetRightEntry(DictionaryEntry entry, DictionaryEntry right);
static void DictionaryEntryDealloc(void* ptr);

DEFINE_CLASS(DictionaryIterator,	
	Dictionary dictionary;
	DictionaryEntry currentEntry;
	Stack stack;
);

static void DictionaryIteratorDealloc(void* ptr);

DEFINE_CLASS(Dictionary,	
	DictionaryEntry headEntry;
);

static void DictionaryDealloc(void* ptr);

Dictionary DictionaryCreate()
{
	Dictionary dict = malloc(sizeof(struct _Dictionary));
	
	if (dict == NULL) {
		perror("malloc");
		return NULL;
	}
	
	ObjectInit(dict, DictionaryDealloc);
	
	dict->headEntry = DictionaryEntryCreate("", NULL);
	
	if (dict->headEntry == NULL) {
		printf("Could not create head entry.");
		Release(dict);
		return NULL;
	}
	
	return dict;
}

void* DictionaryGet(Dictionary dict, const char* key)
{
	Lock(dict);
	
	DictionaryEntry entry = dict->headEntry;
	void* value = NULL;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0) {
			value = (void*)entry->value;
			break;
		}
		else if (cmp < 0)
			entry = entry->leftEntry;
		else
			entry = entry->rightEntry;
	}
	
	Unlock(dict);
	return value;
}

void DictionarySet(Dictionary dict, const char* key, const void* value)
{
	Lock(dict);
	printf("Set %s\n", key);
	
	DictionaryEntry entry = dict->headEntry;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0) {
			entry->value = value;
			break;
		}
		else if (cmp < 0) {
			if (entry->leftEntry)
				entry = entry->leftEntry;
			else {
				DictionaryEntry e = DictionaryEntryCreate(key, value);
				DictionaryEntrySetLeftEntry(entry, e);
				Release(e);
				break;
			}
		}
		else {
			if (entry->rightEntry)
				entry = entry->rightEntry;
			else {
				DictionaryEntry e = DictionaryEntryCreate(key, value);
				DictionaryEntrySetRightEntry(entry, e);
				Release(e);
				break;
			}
		}
	}
	
	Unlock(dict);
}

void DictionaryRemove(Dictionary dict, const char* key)
{
	Lock(dict);
	
	DictionaryEntry parent = NULL;
	DictionaryEntry entry = dict->headEntry;
	
	while(entry != NULL) {
		int cmp = strcmp(entry->key, key);
		
		if (cmp == 0) {
			// Found it!
			//
			// To keep it simple, we move the right node up,
			// and put the left not at the left most leaf of the right node
			//
			
			if (entry->leftEntry && entry->rightEntry) {
				DictionaryEntry left = Retain(entry->leftEntry);
				DictionaryEntry right = Retain(entry->rightEntry);
				
				// Move up
				if (parent->leftEntry == entry) {
					DictionaryEntrySetLeftEntry(parent, right);
					entry = right;
				}
				else {
					DictionaryEntrySetRightEntry(parent, right);
					entry = right;
				}
			
				while (entry->leftEntry) entry = entry->leftEntry;
			
				DictionaryEntrySetLeftEntry(entry, left);
				Release(left);
				Release(right);
			}
			else if (entry->leftEntry) {
				DictionaryEntry e = Retain(entry->leftEntry);
				if (parent->leftEntry == entry)
					DictionaryEntrySetLeftEntry(parent, e);
				else
					DictionaryEntrySetRightEntry(parent, e);
				Release(e);
			}
			else if (entry->rightEntry) {
				DictionaryEntry e = Retain(entry->rightEntry);
				if (parent->leftEntry == entry)
					DictionaryEntrySetLeftEntry(parent, e);
				else
					DictionaryEntrySetRightEntry(parent, e);
				Release(e);
			}
			else {
				if (parent->leftEntry == entry)
					DictionaryEntrySetLeftEntry(parent, NULL);
				else
					DictionaryEntrySetRightEntry(parent, NULL);
			}
			
			break;
		}
		else if (cmp < 0) {
			if (entry->leftEntry) {
				parent = entry;
				entry = entry->leftEntry;
			}
			else {
				// Not present
				break;
			}
		}
		else {
			if (entry->rightEntry) {
				parent = entry;
				entry = entry->rightEntry;
			}
			else {
				// Not present
				break;
			}
		}
	}
	
	Unlock(dict);
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
	
	if (entry == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(entry, 0, sizeof(struct _DictionaryEntry));
	
	ObjectInit(entry, DictionaryEntryDealloc);
	
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
	assert(dict != NULL);
	Lock(dict);
	
	DictionaryIterator iter = malloc(sizeof(struct _DictionaryIterator));
	
	if (iter == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(iter, 0, sizeof(struct _DictionaryIterator));
	ObjectInit(iter, DictionaryIteratorDealloc);
	
	iter->stack = StackCreate();
	
	if (iter->stack == NULL) {
		printf("Could not create stack.\n");
		Release(iter);
		return NULL;
	}
	
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

	Unlock(iter->dictionary);	
	Release(iter->dictionary);
	Release(iter->stack);
	free(iter);
}
