//
//  UIViewControllerDTMFControl.h
//  vbyantisipgui
//
//  Created by  on 2012/3/26.
//  Copyright (c) 2012年 antisip. All rights reserved.
//


#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "AppEngine.h"


@interface UIViewControllerDTMFControl : UIViewController <UIActionSheetDelegate, EngineDelegate, CallDelegate>{
    IBOutlet UIButton *dtmf1;
    IBOutlet UIButton *dtmf2;
    IBOutlet UIButton *dtmf3;
    IBOutlet UIButton *dtmf4;
    IBOutlet UIButton *dtmf5;
    IBOutlet UIButton *dtmf6;
    IBOutlet UIButton *dtmf7;
    IBOutlet UIButton *dtmf8;
    IBOutlet UIButton *dtmf9;
    IBOutlet UIButton *dtmf0;

	Call *current_call_dtmf;   
};

//- (void)setCurrentCall:(Call*)call;

@property (retain,nonatomic) Call *current_call_dtmf;

- (IBAction)actionDTMF1:(id)sender;
- (IBAction)actionDTMF2:(id)sender;
- (IBAction)actionDTMF3:(id)sender;
- (IBAction)actionDTMF4:(id)sender;
- (IBAction)actionDTMF5:(id)sender;
- (IBAction)actionDTMF6:(id)sender;
- (IBAction)actionDTMF7:(id)sender;
- (IBAction)actionDTMF8:(id)sender;
- (IBAction)actionDTMF9:(id)sender;
- (IBAction)actionDTMF0:(id)sender;

- (IBAction)actionDTMFpound:(id)sender;
- (IBAction)actionDTMFstar:(id)sender;
@end
