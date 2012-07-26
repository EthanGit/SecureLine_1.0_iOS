//
//  inviteFriendUIViewController.h
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>

@interface inviteFriendUIViewController : UIViewController<MFMessageComposeViewControllerDelegate,MFMailComposeViewControllerDelegate>{} 

@property (retain, nonatomic) IBOutlet UIButton *emailInviteButton;
@property (retain, nonatomic) IBOutlet UIButton *smsInviteButton;

@property (retain, nonatomic) IBOutlet UILabel *memo_message;

- (IBAction)emailButtonPressed:(id)sender;
- (IBAction)smsButtonPressed:(id)sender;

@end
