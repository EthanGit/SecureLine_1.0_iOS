//
//  creatAccountUIViewController.h
//  vbyantisipgui
//
//  Created by Arthur on 2012/5/28.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface creatAccountUIViewController : UIViewController <UITextFieldDelegate>

@property (retain, nonatomic) IBOutlet UITextField *firstNameUITextField;
@property (retain, nonatomic) IBOutlet UITextField *lastNameUITextField;
@property (retain, nonatomic) IBOutlet UITextField *loginUITextField;
@property (retain, nonatomic) IBOutlet UITextField *passwdUITextField;
@property (retain, nonatomic) IBOutlet UITextField *confirmPasswdUITextField;


- (void)doWebLoginProcess;
- (void)returnToLastPage;

@end
