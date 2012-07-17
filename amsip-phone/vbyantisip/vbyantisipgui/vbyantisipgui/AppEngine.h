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
#import "gentriceGlobal.h"
#import "Call.h"
#import "Registration.h"


#ifdef GENTRICE	
#import "SqliteRecentsHelper.h"
#else
#import "SqliteHistoryHelper.h"
#endif

@interface AppEngine : NSObject {

  @private
  
	NSString *userAgent;
	id engineDelegates;
	id registrationDelegates;

	Registration *registration;
	NSMutableDictionary *call_list;

	iAudioPlayer *iAudioPlayerApp;
	
	BOOL thread_start;
	BOOL thread_started;
#ifdef GENTRICE	
    SqliteRecentsHelper *myHistoryDb;
#else    
    SqliteHistoryHelper *myHistoryDb;
#endif
}

@property (nonatomic, retain) NSString *userAgent;
@property (nonatomic, assign) id engineDelegates;
@property (nonatomic, assign) id registrationDelegates;

- (void) addCallDelegate:(id)_adelegate;
- (void) removeCallDelegate:(id)_adelegate;

- (void) removeRegistrationDelegate:(id)_adelegate;
- (void) addRegistrationDelegate:(id)_adelegate;

- (int) getNumberOfActiveCalls;

-(int)amsip_start:(NSString*)target_user withReferedby_did:(int)referedby_did;
-(int)amsip_start_g:(NSString*)target_user withReferedby_did:(int)referedby_did;

-(int)amsip_answer:(int)code forCall:(Call*)pCall;
-(int)amsip_stop:(int)code forCall:(Call*)pCall;

- (NSString *)checkOutProxy:(NSString *)string;
- (NSString *)checkOutPort:(NSString *)string;

-(void)initialize;
-(BOOL)isConfigured;
-(BOOL)isStarted;
-(void)stop;
-(void)start;
-(void)refreshRegistration;

@end

extern AppEngine *gAppEngine;


@protocol EngineDelegate
@optional
- (void)onCallNew:(Call *)call;
- (void)onCallRemove:(Call *)call;

- (void)onCallExist:(Call *)call;

@end
