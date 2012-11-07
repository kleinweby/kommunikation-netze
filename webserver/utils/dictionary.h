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

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include "utils/object.h"

#include <stdbool.h>

DECLARE_CLASS(Dictionary);
DECLARE_CLASS(DictionaryIterator);

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
