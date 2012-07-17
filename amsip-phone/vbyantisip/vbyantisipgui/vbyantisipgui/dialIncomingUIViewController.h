//
//  dialIncomingUIViewController.h
//  vbyantisipgui
//
//  Created by Arthur Tseng on 2012/5/30.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppEngine.h"

@interface dialIncomingUIViewController : UIViewController <EngineDelegate, CallDelegate> {
    Call *current_call;
    bool autoAnswer;
}
@property (retain, nonatomic) IBOutlet UILabel *callStateUILable;

@property (retain, nonatomic) IBOutlet UILabel *actionStateUILabel;

@property (retain, nonatomic) IBOutlet UIButton *answerUIButton;
@property (retain, nonatomic) IBOutlet UIButton *endCallUIButton;
@property (retain, nonatomic) IBOutlet UILabel *callerUILabel;
@property (retain, nonatomic) IBOutlet UILabel *UILabelEndCall;
@property (retain, nonatomic) IBOutlet UILabel *UILabelAnswer;
@property (retain, nonatomic) IBOutlet UILabel *lStatusUILabel;
@property (retain, nonatomic) IBOutlet UILabel *lineQualityUILabel;
@property bool autoAnswer;

- (IBAction)answerButtonPressed:(id)sender;

- (IBAction)endCallButtonPressed:(id)sender;

- (void)setCurrentCall:(Call*)call;

- (NSString*)stripSecureIDfromURL:(NSString*) targetString;
- (NSString*) lookupDisplayName:(NSString*) caller;
- (void)setRemoteIdentity:(NSString*)str;

@end
