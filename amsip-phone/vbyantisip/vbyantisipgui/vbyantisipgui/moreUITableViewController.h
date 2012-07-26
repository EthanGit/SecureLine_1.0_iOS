//
//  moreUITableViewController.h
//  vbyantisipgui
//
//  Created by arthur on 2012/5/23.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>



@interface moreUITableViewController : UITableViewController  <UIAlertViewDelegate,UITextFieldDelegate> {
    
    NSMutableArray *moreItemsArray;

    UITextField *passcordString, *checkpasscordString;
    UIAlertView *settingAlertPasscode;
    UISwitch *passcode;
    BOOL showPassSetting;
}
@property (retain, nonatomic) IBOutlet UIImageView *moreTabUIImageView;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_setting_passcode;
@property (retain, nonatomic) UISwitch *passcode;

@property (retain, nonatomic)  UIImageView *set_pass_bg;
@property (retain, nonatomic)  UILabel *old_passcode_label;
@property (retain, nonatomic)  UILabel *new_passcode_label;
@property (retain, nonatomic)  UILabel *check_passcode_label;
@property (retain, nonatomic)  UITextField *old_passcode_field;
@property (retain, nonatomic)  UITextField *new_passcode_field;
@property (retain, nonatomic)  UITextField *check_passcode_field;
@property (retain, nonatomic)  UIButton *set_passcode_button;
@property (retain, nonatomic)  UIButton *set_passcode_button_2;
@property (retain, nonatomic)  UIAlertView *alert_message;
@property (retain, nonatomic)  NSString *message_string;
/*
@property (retain, nonatomic) IBOutlet UIImageView *set_pass_bg;
@property (retain, nonatomic) IBOutlet UILabel *old_passcode_label;
@property (retain, nonatomic) IBOutlet UILabel *new_passcode_label;
@property (retain, nonatomic) IBOutlet UILabel *check_passcode_label;
@property (retain, nonatomic) IBOutlet UITextField *old_passcode_field;
@property (retain, nonatomic) IBOutlet UITextField *new_passcode_field;
@property (retain, nonatomic) IBOutlet UITextField *check_passcode_field;
@property (retain, nonatomic) IBOutlet UIButton *set_passcode_button;
 */
- (IBAction)set_passcode_action:(id)sender;
@property (retain, nonatomic) IBOutlet UILabel *set_passcode_title;
@property  BOOL showPassSetting;
- (void)set_passcode_action_setting;
- (void)set_passcode_action_modify;
-(void)setPasscode;
-(void)toLoginUI;
@end
