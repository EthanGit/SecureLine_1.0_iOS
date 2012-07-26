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

#if 0
    AVAudioPlayer *_player;
#endif
}

#if 0
@property (nonatomic, assign)	AVAudioPlayer*	_player;
#endif

- (void) play:(NSString *)filename;
- (void) stop;

@end
