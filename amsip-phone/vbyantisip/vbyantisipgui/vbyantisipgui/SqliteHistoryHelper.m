//
//  SqliteHistoryHelper.m
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import "SqliteHistoryHelper.h"

@implementation SqliteHistoryHelper

- (int) open_database
{
  NSString *docsDir;
  NSArray *dirPaths;
  
  // Get the documents directory
  dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  
  docsDir = [dirPaths objectAtIndex:0];
  
  // Build the path to the database file
  databasePath = [[NSString alloc] initWithString: [docsDir stringByAppendingPathComponent: @"history.db"]];
  
  NSFileManager *filemgr = [NSFileManager defaultManager];
  
  if ([filemgr fileExistsAtPath: databasePath ] == NO)
  {
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
      NSLog(@"sqlite: database not opened");
      return -1;
    }
    
    char *errMsg;
    const char *sql_stmt = "CREATE TABLE IF NOT EXISTS history (ID INTEGER PRIMARY KEY AUTOINCREMENT, callid TEXT, remoteuri TEXT, pid TEXT)";
    
    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database HISTORY not created");
      return -1;
    }
    
    sql_stmt = "CREATE TABLE IF NOT EXISTS callinfo (ID INTEGER PRIMARY KEY AUTOINCREMENT, callid TEXT, description TEXT, text_val TEXT, int_val TEXT)";
    
    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database CALLINFO not created");
      return -1;
    }
      
    sqlite3_close(myFavoriteContactDb);
  }
  
  [filemgr release];
  return 0;
}


- (int) find_callid:(NSMutableArray*)arrayHistory:(NSString *)keyword{
        sqlite3_stmt    *statement;
        
        const char *dbpath = [databasePath UTF8String];
        
        if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
        {
            NSLog(@"sqlite: database not opened");
            return -1;
        }
    
    
        
        NSString *querySQL = [NSString stringWithFormat: @"SELECT a.callid, a.remoteuri , b.text_val as start_date FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid and b.description = 'start_date' where a.remoteuri like '%%sip:%@@%%'   order by   b.text_val desc",keyword];// a left join CALLINFO b on a.id = b.callid where b.int_val !=200  
        //@"SELECT a.callid, a.remoteuri, b.callid, b.int_val ,b.text_val, a.id , b.description FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid and description =\"sip_code\" where b.int_val <>\"200\" "
        //@"SELECT a.callid, a.remoteuri, b.callid, b.int_val ,b.text_val, a.id , b.description FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid where b.id is null or ( b.description = 'sip_code' and b.int_val!=\"200\") "
       // NSLog(@"##### querySQL = %@",querySQL);
        const char *query_stmt = [querySQL UTF8String];
        
        if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
        {
            NSLog(@"sqlite: database error");
            sqlite3_finalize(statement);
            sqlite3_close(myFavoriteContactDb);
            return -1;
        }
        
        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            HistoryEntry *historyEntry = [[HistoryEntry alloc] init];
            const char *callid = (const char *) sqlite3_column_text(statement, 0);
            const char *remoteuri = (const char *) sqlite3_column_text(statement, 1);
            const char *start_date = (const char *) sqlite3_column_text(statement, 2);        
            if (callid)
                [historyEntry setCallid: [NSString stringWithUTF8String:callid]];
            if (remoteuri)
                [historyEntry setRemoteuri: [NSString stringWithUTF8String:remoteuri]];
            
            //[self load_data:historyEntry withId:callid];
            [arrayHistory addObject:historyEntry];
            
            //  NSLog(@"########## %@ - %@ - %@ - %@ - %@ ",[NSString stringWithUTF8String:callid], [NSString stringWithUTF8String:remoteuri], [NSString stringWithUTF8String:int_val] , [NSString stringWithUTF8String:text_val], [NSString stringWithUTF8String:hid]);
            //NSLog(@"########## %s - %s - %s  ",callid, remoteuri,start_date); 
        }
        
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        return 0;
}


- (int) load_history:(NSMutableArray*)arrayHistory
{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *querySQL = [NSString stringWithFormat: @"SELECT callid, remoteuri FROM HISTORY "];
  
  const char *query_stmt = [querySQL UTF8String];
  
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  while (sqlite3_step(statement) == SQLITE_ROW)
  {
    HistoryEntry *historyEntry = [[HistoryEntry alloc] init];
    const char *callid = (const char *) sqlite3_column_text(statement, 0);
    const char *remoteuri = (const char *) sqlite3_column_text(statement, 1);
    if (callid)
      [historyEntry setCallid: [NSString stringWithUTF8String:callid]];
    if (remoteuri)
      [historyEntry setRemoteuri: [NSString stringWithUTF8String:remoteuri]];
    
    //[self load_data:historyEntry withId:callid];
    [arrayHistory addObject:historyEntry];
  }
  
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) load_lost_history:(NSMutableArray*)arrayHistory
{
    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        return -1;
    }
    
    NSString *querySQL = [NSString stringWithFormat: @"SELECT a.callid, a.remoteuri, b.callid, b.int_val ,b.text_val, a.id , b.description FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid and b.description = 'sip_code' where   b.int_val!=\"200\"  and a.callid in (select callid from callinfo where description = 'direction' and int_val = 1) "];// a left join CALLINFO b on a.id = b.callid where b.int_val !=200  
    //@"SELECT a.callid, a.remoteuri, b.callid, b.int_val ,b.text_val, a.id , b.description FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid and description =\"sip_code\" where b.int_val <>\"200\" "
    //@"SELECT a.callid, a.remoteuri, b.callid, b.int_val ,b.text_val, a.id , b.description FROM HISTORY  a left join  CALLINFO b on a.callid = b.callid where b.id is null or ( b.description = 'sip_code' and b.int_val!=\"200\") "
    //NSLog(@"##### querySQL = %@",querySQL);
    const char *query_stmt = [querySQL UTF8String];
    
    if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
    {
        NSLog(@"sqlite: database error");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        return -1;
    }
    
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        HistoryEntry *historyEntry = [[HistoryEntry alloc] init];
        const char *callid = (const char *) sqlite3_column_text(statement, 0);
        const char *remoteuri = (const char *) sqlite3_column_text(statement, 1);
        const char *callid_b = (const char *) sqlite3_column_text(statement, 2);        
        const char *int_val = (const char *) sqlite3_column_text(statement, 3);
        const char *text_val = (const char *) sqlite3_column_text(statement, 4); 
        const char *description = (const char *) sqlite3_column_text(statement, 6);  
                const char *hid = (const char *) sqlite3_column_text(statement, 5);
        if (callid)
            [historyEntry setCallid: [NSString stringWithUTF8String:callid]];
        if (remoteuri)
            [historyEntry setRemoteuri: [NSString stringWithUTF8String:remoteuri]];
        
        //[self load_data:historyEntry withId:callid];
        [arrayHistory addObject:historyEntry];
        
      //  NSLog(@"########## %@ - %@ - %@ - %@ - %@ ",[NSString stringWithUTF8String:callid], [NSString stringWithUTF8String:remoteuri], [NSString stringWithUTF8String:int_val] , [NSString stringWithUTF8String:text_val], [NSString stringWithUTF8String:hid]);
        //NSLog(@"########## %s - %s - %s - %s - %s - %s - %s ",callid, remoteuri,callid_b, int_val , text_val, hid,description); 
    }
    
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return 0;
}

- (int) insert_history:(HistoryEntry *)historyEntry
{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *insertSQL = [NSString stringWithFormat: @"INSERT INTO HISTORY (callid, remoteuri, pid) VALUES ('%@', '%@', '%@')", historyEntry.callid, historyEntry.remoteuri, historyEntry.pid];
  
  const char *insert_stmt = [insertSQL UTF8String];
    NSLog(@"########### insertSQL = %@",insertSQL);
  sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"Failed to add historyEntry");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) insert_callinfo:(HistoryEntry *)historyEntry withKey:(NSString*)key intValue:(int)val{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *insertSQL = [NSString stringWithFormat: @"INSERT INTO CALLINFO (callid, description, int_val) VALUES (\"%@\", \"%@\", %i)", historyEntry.callid, key, val];
  //NSLog(@"########### insertSQL = %@",insertSQL);
  const char *insert_stmt = [insertSQL UTF8String];
  
  sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"Failed to add historyEntry");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) insert_callinfo:(HistoryEntry *)historyEntry withKey:(NSString*)key textValue:(NSString*)val{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *insertSQL = [NSString stringWithFormat: @"INSERT INTO CALLINFO (callid, description, text_val) VALUES (\"%@\", \"%@\", \"%@\")", historyEntry.callid, key, val];
   // NSLog(@"########### insertSQL = %@",insertSQL);
  const char *insert_stmt = [insertSQL UTF8String];
  
  sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"Failed to add historyEntry");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) findint_callinfo:(HistoryEntry *)historyEntry withKey:(NSString*)key {
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *querySQL = [NSString stringWithFormat: @"SELECT int_val FROM CALLINFO WHERE callid = ? AND description = ? LIMIT 1"];
  
  const char *query_stmt = [querySQL UTF8String];
  
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  sqlite3_bind_text(statement, 1, [historyEntry.callid UTF8String], -1, SQLITE_STATIC);
  sqlite3_bind_text(statement, 2, [key UTF8String], -1, SQLITE_STATIC);
  
  int result=-1;
  while (sqlite3_step(statement) == SQLITE_ROW)
  {
    result = sqlite3_column_int(statement, 0);
    break;
  }
  
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return result;
}

- (NSString *) findtext_callinfo:(HistoryEntry *)historyEntry withKey:(NSString*)key {
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return nil;
  }
  
  NSString *querySQL = [NSString stringWithFormat: @"SELECT text_val FROM CALLINFO WHERE callid = ? AND description = ? LIMIT 1"];
  
  const char *query_stmt = [querySQL UTF8String];
  
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return nil;
  }

  sqlite3_bind_text(statement, 1, [historyEntry.callid UTF8String], -1, SQLITE_STATIC);
  sqlite3_bind_text(statement, 2, [key UTF8String], -1, SQLITE_STATIC);
  
  NSString *result=nil;
  while (sqlite3_step(statement) == SQLITE_ROW)
  {
    const char *callid = (const char *) sqlite3_column_text(statement, 0);
    result = [[NSString stringWithUTF8String:callid] retain];
    break;
  }
  
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return result;
}

- (int) remove_history
{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *querySQL = [NSString stringWithFormat: @"DELETE FROM HISTORY"];
  
  const char *query_stmt = [querySQL UTF8String];
  
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"sqlite: failed to remove history entries");
  }
  sqlite3_finalize(statement);
  
  //remove all numbers:
  querySQL = [NSString stringWithFormat: @"DELETE FROM callinfo"];
  query_stmt = [querySQL UTF8String];
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"sqlite: failed to remove callinfo entries");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);
  
  sqlite3_close(myFavoriteContactDb);
  return 0;
}


@end
