//
//  addContactViewController.h
//  vbyantisipgui
//
//  Created by  on 2012/6/21.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "Contacts.h"
//#import "ContactsEntry.h"
#import "SqliteContactHelper.h"
//@class Contacts;

//@protocol addContactViewControllerDelegate;

@interface addContactViewController : UITableViewController<UITextFieldDelegate,UITextViewDelegate,UITableViewDelegate>
{
    //NSFetchedResultsControllerDelegate,
    ContactsEntry *contact; 
    SqliteContactHelper *sqlContactDB;
   // id <addContactViewControllerDelegate> delegate;
    
    NSString *default_secureid;
    
}

@property (retain, nonatomic) IBOutlet UITableViewCell *cell_name;
@property (retain, nonatomic) IBOutlet UITableView *tableView;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_company;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_secureid;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_other;
@property (retain, nonatomic) IBOutlet UITextField *company_field;
@property (retain, nonatomic) IBOutlet UITextField *secureid_field;
@property (retain, nonatomic) IBOutlet UITextView *other_field;
@property (retain, nonatomic) IBOutlet UITextField *lastname;
@property (retain, nonatomic) IBOutlet UITextField *firstname_field;
@property (retain, nonatomic) NSString *default_secureid;
@property (retain, nonatomic) SqliteContactHelper *sqlContactDB;
//@property (nonatomic, assign) id <addContactViewControllerDelegate> delegate;



@property (nonatomic, retain) ContactsEntry *contact;
-(void)cancel;
-(void)save;

@end
/*
@protocol addContactViewControllerDelegate
- (void)addContactViewController:(addContactViewController *)controller didFinishWithSave:(BOOL)save;
@end
*/