//
//  SqliteHistoryHelper.m
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//
#import "vbyantisipAppDelegate.h"
#import "SqliteRecentsHelper.h"
#import "NSString+SECUREID.h"
#import "vbyantisipAppDelegate.h"
#import "RecentsViewController.h"

@implementation SqliteRecentsHelper
@synthesize recent_m;

- (int) open_database
{
  NSString *docsDir;
  NSArray *dirPaths;
  
  // Get the documents directory
  dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  
  docsDir = [dirPaths objectAtIndex:0];
  
  // Build the path to the database file
  databasePath = [[NSString alloc] initWithString: [docsDir stringByAppendingPathComponent: @"appdata.sqlite"]];
  
  NSFileManager *filemgr = [NSFileManager defaultManager];
  
  if ([filemgr fileExistsAtPath: databasePath ] == NO)
  {
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
      NSLog(@"sqlite: database not opened");
      return -1;
    }
      NSLog(@"sqlite: database CALLINFO not created");
    /*
    char *errMsg;
    const char *sql_stmt = "CREATE TABLE IF NOT EXISTS Recents (ID INTEGER PRIMARY KEY AUTOINCREMENT, callid TEXT, remoteuri TEXT, pid TEXT)";
    
    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database Recents not created");
      return -1;
    }
    
    sql_stmt = "CREATE TABLE IF NOT EXISTS callinfo (ID INTEGER PRIMARY KEY AUTOINCREMENT, callid TEXT, description TEXT, text_val TEXT, int_val TEXT)";
    
    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database CALLINFO not created");
      return -1;
    }
    */  
    sqlite3_close(myFavoriteContactDb);
  }
  
  //[filemgr release];
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
    /*
    @dynamic callid;
    @dynamic direction;
    @dynamic duration;
    @dynamic end_date;
    @dynamic remoteuri;
    @dynamic section_key;
    @dynamic sip_code;
    @dynamic start_date;
    @dynamic secureid;
    @dynamic sip_reason; 
    */    
    
    
    
    
    //callid,remoteuri,secureid,start_date,end_date,duration,direction,sip_code,sip_reason
    NSString *querySQL = [NSString stringWithFormat: @"SELECT ZCALLID ,ZREMOTEURI, ZSECUREID ,   ZSTART_DATE , ZEND_DATE , ZDURATION  , ZDIRECTION  , ZSIP_CODE , ZSIP_REASON ,ZSECTION_KEY FROM main.ZRECENTS WHERE ZSECUREID = '%@' ORDER BY ZSTART_DATE DESC",keyword];
    /*
    NSString *querySQL = [NSString stringWithFormat: @"select name,sql from main.sqlite_master where type='table'"];
    */
        NSLog(@"##### querySQL = %@",querySQL);
        const char *query_stmt = [querySQL UTF8String];
        
        if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
        {
            NSLog(@"sqlite: database error");
            sqlite3_finalize(statement);
            sqlite3_close(myFavoriteContactDb);
            return -1;
        }
    /*
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
      
        const char *name = (const char *) sqlite3_column_text(statement, 0);
        const char *sql = (const char *) sqlite3_column_text(statement, 1);
        NSLog(@"name = %@ \n",[NSString stringWithUTF8String:name]);
        NSLog(@"sql = %@",[NSString stringWithUTF8String:sql]);
    }*/
        
        while (sqlite3_step(statement) == SQLITE_ROW)
        {
            RecentsEntry *recent = [[RecentsEntry alloc] init];
            const char *callid = (const char *) sqlite3_column_text(statement, 0);
            const char *remoteuri = (const char *) sqlite3_column_text(statement, 1);
            const char *secureid = (const char *) sqlite3_column_text(statement, 2);            
            const char *start_date = (const char *) sqlite3_column_text(statement, 3); 
            const char *end_date = (const char *) sqlite3_column_text(statement, 4);
            const char *duration = (const char *) sqlite3_column_text(statement, 5);            
            const char *direction = (const char *) sqlite3_column_text(statement, 6);             
            const char *sip_code = (const char *) sqlite3_column_text(statement, 7);
            const char *sip_reason = (const char *) sqlite3_column_text(statement, 8);
            //const char *section_key = (const char *) sqlite3_column_text(statement, 9);
            if (callid)
                [recent setCallid: [NSString stringWithUTF8String:callid]];
            if (remoteuri)
                [recent setRemoteuri: [NSString stringWithUTF8String:remoteuri]];
            if (secureid)
                [recent setSecureid: [NSString stringWithUTF8String:secureid]];
            if (start_date)
                [recent setStart_date: [NSString stringWithUTF8String:start_date]];
            if (end_date)
                [recent setEnd_date: [NSString stringWithUTF8String:end_date]];
            if (duration)
                [recent setDuration: [NSString stringWithUTF8String:duration]];
            if (direction)
                [recent setDirection: [NSString stringWithUTF8String:direction]];
            if (sip_code)
                [recent setSip_code: [NSString stringWithUTF8String:sip_code]];
            if (sip_reason)
                [recent setSip_reason: [NSString stringWithUTF8String:sip_reason]];            
            //[self load_data:historyEntry withId:callid];
            [arrayHistory addObject:recent];
            
            //  NSLog(@"########## %@ - %@ - %@ - %@ - %@ ",[NSString stringWithUTF8String:callid], [NSString stringWithUTF8String:remoteuri], [NSString stringWithUTF8String:int_val] , [NSString stringWithUTF8String:text_val], [NSString stringWithUTF8String:hid]);
            NSLog(@"########## %@ - %@ - %@  ",recent.callid, recent.remoteuri,recent.start_date);
            [recent release];
        }
        
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        return 0;
}

/*
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
    NSLog(@"##### querySQL = %@",querySQL);
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
        NSLog(@"########## %s - %s - %s - %s - %s - %s - %s ",callid, remoteuri,callid_b, int_val , text_val, hid,description); 
    }
    
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return 0;
}
*/
- (int) insert_history:(RecentsEntry *)historyEntry
{
    


    
	//NSManagedObjectContext *addingContext = [[NSManagedObjectContext alloc] init];


	//NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:@"Recents" inManagedObjectContext:addingContext];
    RecentsViewController *table = [[RecentsViewController alloc ] init];


    
    NSManagedObjectContext *managedObjectContext = [(RecentsViewController *)[[UIApplication sharedApplication] delegate] managedObjectContext];
	
    NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:@"Recents" inManagedObjectContext:managedObjectContext];
    
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.callid] forKey:@"callid"];
    
    [newManagedObject setValue:[NSString stringWithFormat:@"%@",  [[NSString stringWithFormat:@"%@", historyEntry.remoteuri] SECUREID]] forKey:@"secureid"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.start_date] forKey:@"start_date"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.end_date] forKey:@"end_date"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.sip_code] forKey:@"sip_code"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.sip_reason] forKey:@"sip_reason"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.remoteuri] forKey:@"remoteuri"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.duration] forKey:@"duration"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.direction] forKey:@"direction"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.section_key] forKey:@"section_key"]; 
    
    [(RecentsViewController *)[[UIApplication sharedApplication] delegate] saveContext];
    
    [table.tableView reloadData];
    [table release];
   // [(RecentsViewController *)[[UIApplication sharedApplication] delegate]
     /*
    NSError *error;
    if (![managedObjectContext save:&error]) {
        // Update to handle the error appropriately.
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        exit(-1);  // Fail
    }  
      */
    /*
    NSError *error = nil;
    if (![managedObjectContext save:&error]) {
        
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    } */    
    /*
    recent_m.callid = historyEntry.callid;
    recent_m.direction = historyEntry.direction;
    recent_m.duration = historyEntry.duration;
    recent_m.end_date = historyEntry.end_date;
    
    recent_m.remoteuri = historyEntry.remoteuri;   
    
    recent_m.section_key = historyEntry.section_key; 
    recent_m.sip_code = historyEntry.sip_code; 
    recent_m.start_date = historyEntry.start_date; 
    recent_m.secureid = historyEntry.secureid; 
    recent_m.sip_reason = historyEntry.sip_reason; 
    */
    
	//[delegate addContactViewController:self didFinishWithSave:YES];
    //[delegate addSqliteRecentsHelper:self didFinishWithSave:YES];
    //[addingContext release];
    
   // NSManagedObjectContext *managedObjectContext = [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] managedObjectContext]; 
    
    
    /*
    NSManagedObjectContext *managedObjectContext = [(RecentsViewController *)[[UIApplication sharedApplication] delegate] managedObjectContext];
    //vbyantisipAppDelegate *delegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
   // [(RecentsViewController *)NSFetchedResultsChangeDelete;
    
    NSLog(@"After managedObjectContext: %@",  managedObjectContext);

    //NSManagedObjectContext *context = [fetchedResultsController managedObjectContext];
    //NSEntityDescription *entity = [[fetchedResultsController fetchRequest] entity];
    //NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:[entity name] inManagedObjectContext:context];
    NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:@"Recents" inManagedObjectContext:managedObjectContext];
    
    
    // If appropriate, configure the new managed object.
    // Normally you should use accessor methods, but using KVC here avoids the need to add a custom class to the template.
    
    
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.callid] forKey:@"callid"];

    [newManagedObject setValue:[NSString stringWithFormat:@"%@",  [[NSString stringWithFormat:@"%@", historyEntry.remoteuri] SECUREID]] forKey:@"secureid"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.start_date] forKey:@"start_date"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.end_date] forKey:@"end_date"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.sip_code] forKey:@"sip_code"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.sip_reason] forKey:@"sip_reason"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.remoteuri] forKey:@"remoteuri"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.duration] forKey:@"duration"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.direction] forKey:@"direction"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", historyEntry.section_key] forKey:@"section_key"];    
    
    //[delegate saveContext];
    // Save the context.
    */
    /*
    NSError *error = nil;
    if (![managedObjectContext save:&error]) {
        
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    }  
    else{
        NSLog(@"Unresolved save %@, %@",error, [error userInfo]);
    }*/
    //[[(RecentsViewController *)[[UIApplication sharedApplication] delegate] tableView]reloadData];

    
    //[(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] saveContext];
    
  /*  
  sqlite3_stmt    *statement;
  
    NSLog(@"remoteid:%@,callid:%@",historyEntry.remoteuri,historyEntry.callid); 
    
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
   // NSArray *key  = [NSArray addObjects:@""];
   // ZSTART_DATE , ZEND_DATE , ZDURATION  , ZDIRECTION  , ZSIP_CODE , ZSIP_REASON ,ZSECTION_KEY 
    
  
  NSString *insertSQL = [NSString stringWithFormat: @"INSERT INTO  main.ZRECENTS (ZCALLID, ZREMOTEURI , ZSECUREID) VALUES ('%@', '%@', '%@')", historyEntry.callid, historyEntry.remoteuri,[[NSString stringWithFormat:@"%@", historyEntry.remoteuri] SECUREID]];
   
    
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
  */ 
  return 0;
}

- (int) insert_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key intValue:(int)val{
    if([key isEqualToString:@"ZSIP_CODE"])
    {
        [recent_m setSip_code:[NSString stringWithFormat:@"%i",val]];
        
    }
    else if([key isEqualToString:@"ZDIRECTION"])
    {
        [recent_m setDirection:[NSString stringWithFormat:@"%i",val]];
        
    } 
    /* 

    if([key isEqualToString:@"sip_code"])
    {
        [recent_m setSip_code:[NSString stringWithFormat:@"%i",val]];
        
    }
    else if([key isEqualToString:@"direction"])
    {
        [recent_m setDirection:[NSString stringWithFormat:@"%i",val]];
        
    } 
    */
    
    NSError *error;
    if (![recent_m.managedObjectContext save:&error]) {
        // Update to handle the error appropriately.
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        exit(-1);  // Fail
    }
    return 0;
    /*
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  //NSString *insertSQL = [NSString stringWithFormat: @"UPDATE Recents SET %@ = '%@' WHERE callid = '%@'", key, val, historyEntry.callid];
    NSString *insertSQL = [NSString stringWithFormat: @"UPDATE main.ZRECENTS SET %@ = '%i' WHERE ZCALLID = '%@'", key, val, historyEntry.callid];
    
    
  NSLog(@"########### insertSQL = %@",insertSQL);
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
    */
}

- (int) insert_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key textValue:(NSString*)val{


    if([key isEqualToString:@"ZDURATION"])
    {
        [recent_m setDuration:[NSString stringWithFormat:@"%@",val]];
        
    }
    else if([key isEqualToString:@"ZEND_DATE"])
    {
        [recent_m setEnd_date:[NSString stringWithFormat:@"%@",val]];
        
    }
    else if([key isEqualToString:@"ZSTART_DATE"])
    {
        [recent_m setStart_date:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"ZSECTION_KEY"])
    {
        [recent_m setSection_key:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"ZSECUREID"])
    {
        [recent_m setSecureid:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"ZSIP_REASON"])
    {
        [recent_m setSip_reason:[NSString stringWithFormat:@"%@",val]];
        
    }
    /*
    if([key isEqualToString:@"duration"])
    {
        [recent_m setDuration:[NSString stringWithFormat:@"%@",val]];
        
    }
    else if([key isEqualToString:@"end_date"])
    {
        [recent_m setEnd_date:[NSString stringWithFormat:@"%@",val]];
        
    }
    else if([key isEqualToString:@"start_date"])
    {
        [recent_m setStart_date:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"section_key"])
    {
        [recent_m setSection_key:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"secureid"])
    {
        [recent_m setSecureid:[NSString stringWithFormat:@"%@",val]];
        
    } 
    else if([key isEqualToString:@"sip_reason"])
    {
        [recent_m setSip_reason:[NSString stringWithFormat:@"%@",val]];
        
    }     
    */
    /*
    
    recent_m.callid = historyEntry.callid;
    recent_m.direction = historyEntry.direction;
    recent_m.duration = historyEntry.duration;
    recent_m.end_date = historyEntry.end_date;
    
    recent_m.remoteuri = historyEntry.remoteuri;   
    
    recent_m.section_key = historyEntry.section_key; 
    recent_m.sip_code = historyEntry.sip_code; 
    recent_m.start_date = historyEntry.start_date; 
    recent_m.secureid = historyEntry.secureid; 
    recent_m.sip_reason = historyEntry.sip_reason; 

    */
     
    NSError *error;
    if (![recent_m.managedObjectContext save:&error]) {
        // Update to handle the error appropriately.
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        exit(-1);  // Fail
    }
    
    return 0;
    /*
  sqlite3_stmt    *statement;
   
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *insertSQL = [NSString stringWithFormat: @"UPDATE main.ZRECENTS SET %@ = '%@' WHERE ZCALLID = '%@'", key, val, historyEntry.callid];
    
    NSLog(@"########### insertSQL = %@",insertSQL);
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
  return 0;*/
}

- (int) findint_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key {
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

- (NSString *) findtext_callinfo:(RecentsEntry *)historyEntry withKey:(NSString*)key {
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
  
  NSString *querySQL = [NSString stringWithFormat: @"DELETE FROM main.ZRECENTS"];
  
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
   
  sqlite3_close(myFavoriteContactDb);
  return 0;
}


@end
