//
//  newContactUIViewController.h
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CellOwnerDefault.h"
#import "ContactEntry.h"
#import "SqliteContactHelper.h"

@interface newContactUIViewController : UIViewController<UITextFieldDelegate,UITableViewDelegate, UIActionSheetDelegate,UINavigationControllerDelegate>  {
    
    IBOutlet UILabel *contact_name;
    IBOutlet UIButton *contactbutton;
    IBOutlet UITableView *phonelist;
    IBOutlet CellOwnerDefault *cellOwnerLoadFavorites;
    
    
@private
    
    SqliteContactHelper *myContactDb;
//    NSMutableArray *myFavoriteList;
    
    
    ContactEntry *current_contact;
}


@property (unsafe_unretained, nonatomic) IBOutlet UIImageView *headUIImageView;
@property (unsafe_unretained, nonatomic) IBOutlet UITextField *firstNameUITextField;
@property (unsafe_unretained, nonatomic) IBOutlet UITextField *lastNameUITextField;
@property (unsafe_unretained, nonatomic) IBOutlet UITextField *companyUITextField;
@property (unsafe_unretained, nonatomic) IBOutlet UITextField *securePhoneIDUITextField;
@property (unsafe_unretained, nonatomic) IBOutlet UITextView *othersUITextView;

@end
