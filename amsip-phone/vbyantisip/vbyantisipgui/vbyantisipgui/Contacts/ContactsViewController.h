//
//  contactUIViewController.h
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import "CellOwnerDefault.h"
//#import "addContactViewController.h"

//#import "SqliteContactHelper.h"

@interface ContactsViewController:UITableViewController < UIActionSheetDelegate,NSFetchedResultsControllerDelegate,UISearchDisplayDelegate,UISearchBarDelegate>//newContactUIViewControllerDelegate,addContactViewControllerDelegate
{

   // IBOutlet UITableView *contactTable;
    
    IBOutlet CellOwnerDefault *cellOwnerLoadContact;
    
	NSFetchedResultsController *fetchedResultsController;
    NSManagedObjectContext *managedObjectContext;	    
    NSManagedObjectContext *addingManagedObjectContext;	
    NSMutableArray *indexArray;
    BOOL isReadOnly; //for callController, by arthur    
}


- (NSArray *)selectedIndexPaths;

//@property(nonatomic,retain)NSMutableArray *dataSource;
@property (unsafe_unretained, nonatomic) IBOutlet UISearchBar *contactUISearchBar;
@property (unsafe_unretained, nonatomic) IBOutlet UITableView *contactUITableView;
//@property (unsafe_unretained, nonatomic) IBOutlet UITableView *tableView;
@property(retain,nonatomic) NSArray *filteredListContent;
@property (nonatomic) BOOL searchIsActive;
@property (retain, nonatomic) NSMutableArray *indexArray;

@property (nonatomic, retain) NSFetchedResultsController *fetchedResultsController;
@property (nonatomic, retain) NSManagedObjectContext *managedObjectContext;
@property (nonatomic, retain) NSManagedObjectContext *addingManagedObjectContext;
@property BOOL isReadyOnly;
@property (retain, nonatomic) IBOutlet UIActivityIndicatorView *aiv;
@property (retain, nonatomic) IBOutlet UIImageView *noTouchView;
-(void)insertNewObject;
-(void)addNewContact;

+(void)_keepAtLinkTime;
- (void)addControllerContextDidSave:(NSNotification*)saveNotification;

@end
