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
}
@property (retain, nonatomic) IBOutlet UIImageView *moreTabUIImageView;
@property (retain, nonatomic) IBOutlet UISwitch *passcode;
-(void)setPasscode;
-(void)toLoginUI;
@end
