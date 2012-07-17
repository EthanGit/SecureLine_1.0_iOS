//
//  loginUIViewController.h
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface loginUIViewController : UIViewController <UIAlertViewDelegate, UITextFieldDelegate,NSURLConnectionDelegate> {
    
    UITextField *loginName, *passwd;
    UIAlertView *loginAlertView;
    NSString *tmp_password ,*tmp_data,*tmp_keystone;
    BOOL connect_finished;
}

//@property (unsafe_unretained, nonatomic) IBOutlet UITextField *loginIDTextField;
//@property (unsafe_unretained, nonatomic) IBOutlet UITextField *loginPasswdTextField;
@property (unsafe_unretained, nonatomic) IBOutlet UIButton *createAccountButton;

//@property (retain, nonatomic) IBOutlet UINavigationBar *LoginNavigationBar;
//@property (retain, nonatomic) IBOutlet UIBarButtonItem *loginBarButton;
@property (retain, nonatomic) IBOutlet UIButton *loginUIButton;
@property (retain, nonatomic) IBOutlet UITextField *user_account;

@property (retain, nonatomic) IBOutlet UITextField *user_password;

- (IBAction)createAccountButtonPressed:(id)sender;
- (IBAction)loginButtonPressed:(id)sender;


@property (retain, nonatomic) NSString *tmp_password;
@property (retain, nonatomic) NSString *tmp_data;
@property (retain, nonatomic) NSString *tmp_keystone;
@property BOOL connect_finished;

- (void) doLoginProcess;
- (void)login_keystone_api:(NSData*)json;
- (void)get_voip_password:(NSString *)username;
//- (NSString *)MD5;


@end
