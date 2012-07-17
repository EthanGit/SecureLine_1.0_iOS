//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

enum CallState {
	NOTSTARTED,
	TRYING,
	RINGING,
	CONNECTED,
	DISCONNECTED
};

@interface Call : NSObject {
	
	NSString *from;
	NSString *to;
	NSString *oppositeNumber;
	
	bool isMuted;
	bool isOnhold;
	enum CallState callState;
	bool isIncomingCall;
	bool hasMedia;

	//private members
	id callDelegates; // A delegate that wants to act on events in this view
	int tid;
	int cid;
	int did;
	
	int referedby_did;
}

@property (nonatomic, retain) NSString *from;
@property (nonatomic, retain) NSString *to;
@property (nonatomic, retain) NSString *oppositeNumber;
@property(readwrite) bool isMuted;
@property(readwrite) bool isOnhold;
@property(readwrite) enum CallState callState;
@property(readwrite) bool isIncomingCall;
@property(readwrite) bool hasMedia;

- (int) hangup;
- (int) accept:(int)code;
- (int) decline;
- (int) hold;
- (int) unhold;
- (int) record:(NSString*)file;
- (int) record_stop;
- (int) mute;
- (int) unmute;
- (int) addvideo;
- (int) sendDtmf:(char)dtmf_number;

- (float) getUploadBandwidth;
- (float) getDownloadBandwidth;
- (float) getPacketLossIn;
- (float) getPacketLossOut;

- (int) transfer:(NSString*)numberOrsipId;
- (int) connect:(Call*)pCall;

- (void) addCallStateChangeListener:(id)_adelegate;
- (void) removeCallStateChangeListener:(id)_adelegate;

//private members
@property (nonatomic, assign) id callDelegates;
@property(readwrite) int tid;
@property(readwrite) int cid;
@property(readwrite) int did;

@property(readwrite) int referedby_did;

- (void) onCallUpdate;

@end

@protocol CallDelegate
@optional
- (void)onCallUpdate:(Call *)call;

@end

