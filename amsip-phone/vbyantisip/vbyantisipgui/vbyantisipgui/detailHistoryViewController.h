//
//  detailHistoryViewController.h
//  vbyantisipgui
//
//  Created by  on 2012/5/25.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SqliteRecentsHelper.h"

@interface detailHistoryViewController : UITableViewController<UITableViewDelegate, UIActionSheetDelegate> {
    
@private

    NSString *lnstring;
    //NSString *phonenum;
    SqliteRecentsHelper *myHistoryDb;
    NSMutableArray *myHistoryList;    
}


//@property (copy) HistoryEntry *historyEntry;

//@property(nonatomic,copy) NSIndexPath *h_index;
@property(nonatomic,copy) NSString *fnstring;
@property(nonatomic,copy) NSString *secureid;
@property(nonatomic,copy) NSString *phonenum;
@property BOOL showAddButton;

@property (nonatomic, retain) IBOutlet UILabel *first_name;
@property (retain, nonatomic) IBOutlet UILabel *callBtnTitle;

//@property (nonatomic, retain) IBOutlet UILabel *last_name;

@property (nonatomic, retain) IBOutlet UILabel *secure_id;

@property (nonatomic, retain) IBOutlet UIImageView *profileImage;

@property (retain, nonatomic) IBOutlet UIImageView *callflow_image;

@property (retain, nonatomic) IBOutlet UILabel *duration;

@property (retain, nonatomic) IBOutlet UILabel *call_time;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_addbutton;
@property (retain, nonatomic) IBOutlet UIButton *addContactButton;
- (IBAction)addContactAction:(id)sender;

@property (retain, nonatomic) IBOutlet UITableViewCell *cell_profile;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_record;
@property (retain, nonatomic) IBOutlet UITableView *tableView;

-(void)back;
- (IBAction)callNumber:(id)sender;
@end
