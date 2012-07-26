//
//  Registration.h
//  iamsip
//
//  Created by Aymeric MOIZARD on 10/04/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface Registration : NSObject {
	int rid;
	int code;
  NSString *reason;
}

@property(readwrite) int rid;
@property(readwrite) int code;
@property (nonatomic, retain) NSString *reason;

@end
	
@protocol RegistrationDelegate
@optional
- (void)onRegistrationNew:(Registration*)registration;
- (void)onRegistrationRemove:(Registration*)registration;
- (void)onRegistrationUpdate:(Registration*)registration;

@end

