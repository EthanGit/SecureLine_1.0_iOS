//
//  iAudioPlayer.h
//  AppEngine
//
//  Created by Aymeric MOIZARD on 30/10/09.
//  Copyright 2009 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>


@interface iAudioPlayer : NSObject <AVAudioPlayerDelegate> {

	AVAudioPlayer *_player;
	
}

@property (nonatomic, assign)	AVAudioPlayer*	_player;

- (void) play:(NSString *)filename;
- (void) stop;

@end
