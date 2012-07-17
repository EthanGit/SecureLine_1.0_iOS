//
//  SqliteHistoryHelper.h
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import "Recents.h"
#import "RecentsEntry.h"
#import "Recents.h"
#import "sqlite3.h"

@class Recents;

@interface SqliteRecentsHelper : NSObject {

    @private
    sqlite3 *myFavoriteContactDb;
    NSString *databasePath;
    Recents *recent_m;

}

@property (nonatomic, retain) Recents *recent_m;



- (int) open_database;
//- (int) load_history:(NSMutableArray*)arrayHistory;
//- (int) load_lost_history:(NSMutableArray*)arrayHistory;
- (int) insert_history:(RecentsEntry *)historyEntry;
- (int) insert_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key intValue:(int)val;
- (int) insert_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key textValue:(NSString*)val;
- (int) findint_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key;
- (NSString *) findtext_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key;
- (int) remove_history;
- (int) find_callid:(NSMutableArray*)arrayHistory:(NSString *)keyword;
@end



