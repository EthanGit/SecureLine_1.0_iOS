//
//  ContactDetailViewController.h
//  vbyantisipgui
//
//  Created by  on 2012/6/5.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>

//#import <CoreData/CoreData.h>
//#import "Contacts.h"

@class Contacts;

@interface ContactDetailViewController : UITableViewController<UITextViewDelegate,UITextFieldDelegate,UITableViewDelegate>{
    //NSFetchedResultsControllerDelegate,
    Contacts *contact;    
    //NSManagedObjectContext *managedObjectContext;
    //NSManagedObject *editedObject;
	//NSUndoManager *undoManager;  
    //NSFetchedResultsController *fetchedResultsController;
    BOOL isReadOnly; //for callController, by arthur    
}
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_name;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_secure;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_company;
@property (retain, nonatomic) IBOutlet UITableViewCell *cell_other;

//@property (retain, nonatomic) id detailItem;
@property (nonatomic, retain) Contacts *contact;

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
@property (retain, nonatomic) IBOutlet UIImageView *callBtnIcon;
@property (retain, nonatomic) IBOutlet UITableView *tableView;

//@property(retain, nonatomic)  NSManagedObject *newManagedObject;
//@property (strong, nonatomic) NSFetchedResultsController *fetchedResultsController;
//@property (strong, nonatomic) NSManagedObjectContext *managedObjectContext;
//@property (nonatomic, retain) NSManagedObject *editedObject;
//@property (nonatomic, retain) NSUndoManager *undoManager;
@property BOOL isReadyOnly;


- (void)cancel;
-(void)loadData;
/*
- (void)setUpUndoManager;
- (void)cleanUpUndoManager;

-(void)cleanUpUndoManager;
*/
- (void)updateRightBarButtonItemState;
- (int)dial:(NSString*)phonem;
@end
