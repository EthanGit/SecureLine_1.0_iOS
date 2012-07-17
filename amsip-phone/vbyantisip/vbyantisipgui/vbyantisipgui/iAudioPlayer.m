//
//  iAudioPlayer.h
//  AppEngine
//
//  Created by Aymeric MOIZARD on 30/10/09.
//  Copyright 2009 antisip. All rights reserved.
//

#import "iAudioPlayer.h"


@implementation iAudioPlayer

@synthesize _player;

- (id)init{
	_player = nil;
	return self;
}

- (void) playOnce:(NSString *)filename {
	if (self._player != nil)
	{
		[_player stop];
	}
	
	NSURL *fileURL = [[NSURL alloc] initFileURLWithPath: filename];
	self._player = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:nil];	
	if (self._player)
	{
		[self._player setNumberOfLoops:1];
		if ([self._player play])
		{
			self._player.delegate = self;
		}
		else
			NSLog(@"Could not play %@\n", self._player.url);
	}
	[fileURL release];
}

- (void) play:(NSString *)filename {
	if (self._player != nil)
	{
		[_player stop];
	}
	
	NSURL *fileURL = [[NSURL alloc] initFileURLWithPath: filename];
	self._player = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:nil];	
	if (self._player)
	{
		[self._player setNumberOfLoops:-1];
		if ([self._player play])
		{
			self._player.delegate = self;
		}
		else
			NSLog(@"Could not play %@\n", self._player.url);
	}
	[fileURL release];
}

- (void) stop {
	if (self._player != nil)
	{
		[_player release];
		_player = nil;
	}
}

- (void)dealloc
{
	[super dealloc];
	[_player release];
}

#pragma mark AudioSession methods
void RouteChangeListener(	void *                  inClientData,
												 AudioSessionPropertyID	inID,
												 UInt32                  inDataSize,
												 const void *            inData);

void RouteChangeListener(	void *                  inClientData,
						 AudioSessionPropertyID	inID,
						 UInt32                  inDataSize,
						 const void *            inData)
{
	iAudioPlayer* This = (iAudioPlayer*)inClientData;
	
	if (inID == kAudioSessionProperty_AudioRouteChange) {
		
		CFDictionaryRef routeDict = (CFDictionaryRef)inData;
		NSNumber* reasonValue = (NSNumber*)CFDictionaryGetValue(routeDict, CFSTR(kAudioSession_AudioRouteChangeKey_Reason));
		
		int reason = [reasonValue intValue];
		
		if (reason == kAudioSessionRouteChangeReason_OldDeviceUnavailable) {
			
			[[This _player] pause];
		}
	}
}

- (void)setupAudioSession
{
	AVAudioSession *session = [AVAudioSession sharedInstance];
	NSError *error = nil;
	
	[session setCategory: AVAudioSessionCategoryPlayback error: &error];
	if (error != nil)
		NSLog(@"Failed to set category on AVAudioSession");
	
	// AudioSession and AVAudioSession calls can be used interchangeably
	OSStatus result = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, RouteChangeListener, self);
	if (result) NSLog(@"Could not add property listener! %li\n", result);
	
	BOOL active = [session setActive: YES error: nil];
	if (!active)
		NSLog(@"Failed to set category on AVAudioSession");
	
}

#pragma mark AVAudioPlayer delegate methods

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
	if (flag == NO)
		NSLog(@"Playback finished unsuccessfully");
	
	[player setCurrentTime:0.];
}

- (void)playerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error
{
	NSLog(@"ERROR IN DECODE: %@\n", error); 
}

// we will only get these notifications if playback was interrupted
- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player
{
	// the object has already been paused,	we just need to update UI
}

- (void)audioPlayerEndInterruption:(AVAudioPlayer *)player
{
	if ([self._player play])
	{
		self._player.delegate = self;
	}
	else
		NSLog(@"Could not play %@\n", self._player.url);
}

@end
