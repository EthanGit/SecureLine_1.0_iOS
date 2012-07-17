//
//  RecentsViewController.h
//  vbyantisipgui
//
//  Created by  on 2012/7/2.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SqliteRecentsHelper.h"
#import "SqliteContactHelper.h"



@interface RecentsViewController : UITableViewController<UIApplicationDelegate,UIActionSheetDelegate,NSFetchedResultsControllerDelegate>{
	NSFetchedResultsController *fetchedResultsController;
    NSManagedObjectContext *managedObjectContext;	    
    NSManagedObjectContext *addingManagedObjectContext;	
    SqliteRecentsHelper *myHistoryDb;
    SqliteContactHelper *myContactDb;
    NSMutableDictionary *name_dir;
}

@property (strong, nonatomic) IBOutlet UITableView *tableView;
//@property(retain,nonatomic) NSArray *filteredListContent;

@property (nonatomic, retain) NSFetchedResultsController *fetchedResultsController;
@property (nonatomic, retain) NSManagedObjectContext *managedObjectContext;
@property (nonatomic, retain) NSManagedObjectContext *addingManagedObjectContext;

@property (nonatomic) BOOL searchIsLost;
@property (nonatomic) BOOL DBActive;
@property (nonatomic, retain) NSMutableDictionary *name_dir;

//+(void)_keepAtLinkTime;
-(void)searchAll;
-(void)searchLost;
-(void)segmentAction:(UISegmentedControl *)Seg;
- (int)dial:(NSString*)phonem;
- (void)deleteAllItems;
//- (NSString*) stripSecureIDfromURL:(NSString*) targetString;
//- (void)insertNewObject;
//- (void)insertNewObject111;
- (void)saveContext;
- (NSMutableDictionary *)get_contact_name_array;
//- (void)addControllerContextDidSave:(NSNotification*)saveNotification;
//- (void)configureCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

//- (void)insertNewObject:(Recents *)recent;
@end

