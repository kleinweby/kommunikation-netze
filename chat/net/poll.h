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

#ifndef _POLL_H_
#define _POLL_H_

#include <poll.h>

#include "utils/object.h"
#include "utils/dispatchqueue.h"

DECLARE_CLASS(Poll);
DECLARE_CLASS(PollDescriptor);

//
// Creates a new Poll object
//
// Is a Retainable
//
OBJECT_RETURNS_RETAINED
Poll PollCreate();

//
// Registers a new listener block
// with the specified flags
//
// The block will be dispatched on the given queue.
// Null means that it is called inplace.
//
// Returns a poll descriptor. You can use to manipulate
// the registered block. WHen the returned descriptor
// gets released it will be removed from poll.
//
OBJECT_RETURNS_RETAINED
PollDescriptor PollRegister(Poll poll, int fd, short events, DispatchQueue queue, void (^block)(short revents));

//
// Adds a given event to the poll descriptor
//
void PollDescriptorAddEvent(PollDescriptor pd, short event);

//
// Removes a given event from a descriptor (if set)
//
void PollDescriptorRemoveEvent(PollDescriptor pd, short event);

//
// Removes the descriptor from the poll
//
void PollDescritptorRemove(PollDescriptor pd);

#endif /* _POLL_H_ */
