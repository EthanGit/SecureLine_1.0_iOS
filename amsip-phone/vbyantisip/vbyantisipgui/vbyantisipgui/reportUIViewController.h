//
//  reportUIViewController.h
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>
#import "SSTextView.h"

@interface reportUIViewController : UITableViewController <UITextViewDelegate,MFMessageComposeViewControllerDelegate,MFMailComposeViewControllerDelegate>{
    SSTextView *reportTextView;
}

@property (retain, nonatomic) IBOutlet UITextView *reportUITextView;
@property (retain, nonatomic) IBOutlet UIButton *sendReportButton;

@property (retain, nonatomic) SSTextView *reportTextView;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_sendbutton;
- (IBAction)sendReportButtonPressed:(id)sender;
@property (retain, nonatomic) IBOutlet UILabel *memo_message;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_title;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_textview;


@property (retain, nonatomic) IBOutlet UITableView *tableView;
@end
