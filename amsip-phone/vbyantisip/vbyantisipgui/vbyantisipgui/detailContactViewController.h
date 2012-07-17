//
//  detailContactViewController.h
//  vbyantisipgui
//
//  Created by  on 2012/6/5.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CellOwnerDefault.h"
#import "ContactEntry.h"
#import "SqliteContactHelper.h"

@interface detailContactViewController : UIViewController<UITableViewDelegate, UIActionSheetDelegate,UITextViewDelegate> {
    
    IBOutlet CellOwnerDefault *cellOwnerLoadFavorites;
    
    BOOL isReadOnly; //for callController, by arthur
    
@private
    
    SqliteContactHelper *myContactDb;
    //    NSMutableArray *myFavoriteList;
    
    
    ContactEntry *current_contact;    
}



@property(nonatomic,copy) NSString *fnstring;

@property(nonatomic,copy) NSString *snumber;
@property(nonatomic,copy) NSString *lnstring;
@property(nonatomic,copy) NSString *cstring;
@property(nonatomic,copy) NSString *ostring;
@property(nonatomic,copy) NSString *cid;

@property (retain, nonatomic) IBOutlet UIImageView *profileImage;



@property (retain, nonatomic) IBOutlet UITextField *firstname;
@property (retain, nonatomic) IBOutlet UITextField *lastname;

@property (retain, nonatomic) IBOutlet UITextField *company;
@property (retain, nonatomic) IBOutlet UITextField *secureid;
@property (retain, nonatomic) IBOutlet UITextView *other;

@property (retain, nonatomic) IBOutlet UILabel *firstname_v;
@property (retain, nonatomic) IBOutlet UILabel *lastname_v;
@property (retain, nonatomic) IBOutlet UILabel *secureid_v;

@property (retain, nonatomic) IBOutlet UILabel *company_v;

@property (retain, nonatomic) IBOutlet UILabel *other_v;
@property (retain, nonatomic) IBOutlet UILabel *callBtnTitle;
@property (retain, nonatomic) IBOutlet UIButton *callButton;

@property BOOL isReadyOnly;

- (IBAction)callNumber:(id)sender;
@end
