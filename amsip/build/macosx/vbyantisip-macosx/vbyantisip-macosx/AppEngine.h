//
//  AppEngine.h
//  AppEngine
//
//  Created by Aymeric MOIZARD on 30/10/09.
//  Copyright 2009 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <AddressBook/AddressBook.h>
#import "iAudioPlayer.h"

#import "Call.h"
#import "Registration.h"

typedef void (*_on_new_image_cb)(int pin, int width, int height, int format, int size, void *pixel);

@interface AppEngine : NSObject {

	NSString *userAgent;
	id engineDelegates;
	id registrationDelegates;

	Registration *registration;
	NSMutableDictionary *call_list;

	//int rid;
	//int registration_code;

	iAudioPlayer *iAudioPlayerApp;
	
	BOOL thread_start;
	BOOL thread_started;
}

@property (nonatomic, retain) NSString *userAgent;
@property (nonatomic, assign) id engineDelegates;
@property (nonatomic, assign) id registrationDelegates;
//@property(readwrite) int rid;
//@property(readwrite) int registration_code;

- (void) addCallDelegate:(id)_adelegate;
- (void) removeCallDelegate:(id)_adelegate;

- (void) removeRegistrationDelegate:(id)_adelegate;
- (void) addRegistrationDelegate:(id)_adelegate;

- (int) getNumberOfActiveCalls;

-(int)amsip_start:(NSString*)target_user withReferedby_did:(int)referedby_did;
-(int)amsip_answer:(int)code forCall:(Call*)pCall;
-(int)amsip_stop:(int)code forCall:(Call*)pCall;

-(void)initialize;
-(BOOL)isConfigured;
-(void)stop;
-(void)start;
-(void)refreshRegistration;

- (void) setVideoCallback:(_on_new_image_cb) func;
- (void) enableVideo:(int)enable Width:(int)width Height:(int)height;

@end

extern AppEngine *gAppEngine;


@protocol EngineDelegate
@optional
- (void)onCallNew:(Call *)call;
- (void)onCallRemove:(Call *)call;

- (void)onCallExist:(Call *)call;

@end
