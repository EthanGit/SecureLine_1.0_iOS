//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//


#import <Foundation/Foundation.h>

/*
 Software License Agreement (BSD License)
 
 Copyright (c) 2007, Deusty Designs, LLC
 All rights reserved.
 
 Redistribution and use of this software in source and binary forms,
 with or without modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the
 following disclaimer.
 
 * Neither the name of Desuty Designs nor the names of its
 contributors may be used to endorse or promote products
 derived from this software without specific prior
 written permission of Deusty Designs, LLC.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

@class MulticastDelegateEnumerator;

struct MulticastDelegateListNode {
	id delegate;
	struct MulticastDelegateListNode * prev;
    struct MulticastDelegateListNode * next;
    NSUInteger retainCount;
};
typedef struct MulticastDelegateListNode MulticastDelegateListNode;


@interface MulticastDelegate : NSObject
{
	MulticastDelegateListNode *delegateList;
}

- (void)addDelegate:(id)delegate;
- (void)removeDelegate:(id)delegate;

- (void)removeAllDelegates;

- (NSUInteger)count;

- (MulticastDelegateEnumerator *)delegateEnumerator;

@end

@interface MulticastDelegateEnumerator : NSObject
{
	NSUInteger numDelegates;
	NSUInteger currentDelegateIndex;
	MulticastDelegateListNode **delegates;
}

- (id)nextDelegate;
- (id)nextDelegateOfClass:(Class)aClass;
- (id)nextDelegateForSelector:(SEL)aSelector;

@end
