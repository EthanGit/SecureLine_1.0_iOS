//
//  SqliteContactHelper.h
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "gentriceGlobal.h"

#import "ContactsEntry.h"
#import "ContactEntry.h"
#import "sqlite3.h"

@interface SqliteContactHelper : NSObject  {
  
#ifdef GENTRICE
   // Contacts *contact;
#endif    
@private
  sqlite3 *myFavoriteContactDb;
  NSString *databasePath;

}
#ifdef GENTRICE
//@property (nonatomic, retain) Contacts *contact;
- (int) open_database;
- (NSString *)find_contact_name:(NSString *)secureid;
- (NSMutableDictionary *) find_all_contact_name;
- (int) insert_contact:(ContactsEntry *)newcontact;
#else
- (int) open_database;
- (int) load_contacts:(NSMutableArray*)arrayContact;
- (int) insert_contact:(ContactEntry *)contact;
- (int) remove_contact:(ContactEntry *)contact;
- (NSMutableArray *) load_keyword:(NSString *)keyword;
- (NSMutableArray *) load_data;



#endif
@end
