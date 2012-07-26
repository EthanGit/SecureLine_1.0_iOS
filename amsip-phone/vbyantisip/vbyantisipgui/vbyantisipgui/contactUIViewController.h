//
//  contactUIViewController.h
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#import "CellOwnerDefault.h"
#import "SqliteContactHelper.h"

@interface contactUIViewController:UIViewController <UITableViewDelegate, UIActionSheetDelegate>
{
   // IBOutlet UITableView *contactTable;
    
    IBOutlet CellOwnerDefault *cellOwnerLoadContact;
    

    BOOL isReadOnly; //for callController, by arthur
    
@private 
    
    SqliteContactHelper *myContactDb;
    
    NSMutableArray *myFavoriteList;//tableData
    NSMutableArray *dataSource; //will be storing all the data
    //NSMutableArray *tableData;//will be storing data that will be displayed in table
    NSMutableArray *searchedData;//will be storing data matching with the search string    
   
    NSMutableDictionary *sections;
    //NSMutableArray *myHistoryList;
}




//@property(nonatomic,retain)NSMutableArray *dataSource;
@property (unsafe_unretained, nonatomic) IBOutlet UISearchBar *contactUISearchBar;
@property (unsafe_unretained, nonatomic) IBOutlet UITableView *contactUITableView;

@property BOOL isReadyOnly;

-(void) addNewContact;

-(IBAction)closesearchtext:(id)sender;

@end
