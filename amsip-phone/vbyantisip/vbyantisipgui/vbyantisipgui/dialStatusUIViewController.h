//
//  dialStatusUIViewController.h
//  vbyantisipgui
//
//  Created by Arthur Tseng on 2012/5/30.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppEngine.h"

@interface dialStatusUIViewController : UIViewController <EngineDelegate, CallDelegate> {
    Call *current_call;
}

@property (retain, nonatomic) IBOutlet UIButton *endCallUIButton;
@property (retain, nonatomic) IBOutlet UILabel *actionStateUILabel;
@property (retain, nonatomic) IBOutlet UILabel *callToUILabel;
@property (retain, nonatomic) IBOutlet UILabel *UILabelEndCall;
@property (retain, nonatomic) IBOutlet UILabel *lStatusUILabel;
@property (retain, nonatomic) IBOutlet UILabel *lineQualityUILabel;
@property (retain, nonatomic) IBOutlet UIButton *redialUIButton;
@property (retain, nonatomic) IBOutlet UILabel *redialUILabel;


- (IBAction)endCallButtonPressed:(id)sender;
- (IBAction)redialButtonPressed:(id)sender;

- (void)setCurrentCall:(Call*)call;

- (NSString*)stripSecureIDfromURL:(NSString*) targetString;
- (void)setRemoteIdentity:(NSString*)str;
- (NSString*) lookupDisplayName:(NSString*) caller;

@end
