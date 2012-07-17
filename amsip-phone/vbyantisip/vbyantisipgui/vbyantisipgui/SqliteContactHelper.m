//
//  SqliteContactHelper.m
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//
#import "vbyantisipAppDelegate.h"
#import "SqliteContactHelper.h"


@implementation SqliteContactHelper

#ifdef GENTRICE
//@synthesize contact;
#endif

- (int) open_database
{
  NSString *docsDir;
  NSArray *dirPaths;
  
  // Get the documents directory
  dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  
  docsDir = [dirPaths objectAtIndex:0];
  
  // Build the path to the database file
#ifdef GENTRICE
  databasePath = [[NSString alloc] initWithString: [docsDir stringByAppendingPathComponent: @"appdata.sqlite"]];    
#else    
  databasePath = [[NSString alloc] initWithString: [docsDir stringByAppendingPathComponent: @"contacts.db"]];
#endif  
  NSFileManager *filemgr = [NSFileManager defaultManager];
  
  if ([filemgr fileExistsAtPath: databasePath ] == NO)
  {
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
      NSLog(@"sqlite: database not opened");
      return -1;
    }
#ifdef GENTRICE
      NSLog(@"sqlite: database not created");      
#else      
    char *errMsg;
    const char *sql_stmt = "CREATE TABLE IF NOT EXISTS CONTACTS (ID INTEGER PRIMARY KEY AUTOINCREMENT, FIRSTNAME TEXT, LASTNAME TEXT, COMPANY TEXT, OTHER TEXT, SECUREID TEXT)";
    
    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database CONTACTS not created");
      return -1;
    }
      
    
    sql_stmt = "CREATE TABLE IF NOT EXISTS NUMBERS (ID INTEGER PRIMARY KEY AUTOINCREMENT, CONTACTID ID, NUMBER TEXT, PHONETYPE TEXT)";

    if (sqlite3_exec(myFavoriteContactDb, sql_stmt, NULL, NULL, &errMsg) != SQLITE_OK)
    {
      //status.text = @"Failed to create table";
      NSLog(@"sqlite: database NUMBERS not created");
      return -1;
    }
#endif      
    sqlite3_close(myFavoriteContactDb);
  }
  
  [filemgr release];

  return 0;
}

#ifdef GENTRICE
- (NSString *)find_contact_name:(NSString *)secureid{

    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    

    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        return nil;
    }
    
   // NSString *querySQL = [NSString stringWithFormat: @"SELECT ZFIRSTNAME || ' ' || ZLASTNAME as name  FROM ZCONTACTS WHERE ZSECUREID = '?'  LIMIT 1"];
    
   // const char *query_stmt = [querySQL UTF8String];

    
    NSString *querySQL = [NSString stringWithFormat: @"SELECT ZLASTNAME || ' ' || ZFIRSTNAME as name  FROM ZCONTACTS WHERE ZSECUREID = '%@'  LIMIT 1;",secureid];//查詢條件式
    
    
    NSLog(@"######### %@",querySQL);
    const char *query_stmt = [querySQL UTF8String];
    
    if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
    {
        NSLog(@"sqlite: database error");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        return nil;
    }
    
/*
    
    sqlite3_bind_text(statement, 1, [secureid UTF8String], -1, SQLITE_STATIC);
    
        NSLog(@"sql: %@",querySQL);
*/    
    
    NSString *result=nil;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const char *name = (const char *) sqlite3_column_text(statement, 0);
        result = [[NSString stringWithUTF8String:name] retain];
        
        NSLog(@"XXXXXXXXXXXX name = %s  / %@",name,result);
        break;
    }
    
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return result;

}


- (NSMutableDictionary *) find_all_contact_name
{
    NSLog(@"######### find_all_contact_name");
    
    //NSMutableArray* tmparray =[[NSMutableArray alloc]init];//存放查詣結果
    
    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        //return -1;
    }
    
    //NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname FROM CONTACTS where "];
    NSString *querySQL = [NSString stringWithFormat: @"SELECT ZSECUREID , ZLASTNAME || ' ' || ZFIRSTNAME as name FROM ZCONTACTS ;"];//查詢條件式
    
    
    NSLog(@"######### %@",querySQL);
    const char *query_stmt = [querySQL UTF8String];
    
    if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
    {
        NSLog(@"sqlite: database error");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        //return -1;
    }
    NSMutableDictionary* dict = [[NSMutableDictionary alloc]init];
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        
        //ContactEntry *contact = [[ContactEntry alloc] init];

        const char *name = (const char *) sqlite3_column_text(statement, 1);
        const char *secureid = (const char *) sqlite3_column_text(statement, 0);           
        NSLog(@"########### secureid  = %@ / name = %@",[NSString stringWithUTF8String:secureid],[NSString stringWithUTF8String:name]); 
        
        
        //one record
        [dict setObject:[NSString stringWithCString:name encoding:NSUTF8StringEncoding] forKey:[NSString stringWithCString:secureid encoding:NSUTF8StringEncoding]];
           
        //[dict setObject:load_numbers:contact withId:sqlite3_column_int(statement, 0)];
        //[tmparray addObject:dict];
        //[dict release];
    }
    //[tmparray release];
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    
    return [dict autorelease];//返回查询结果数组
    //return 0;
}

- (int) insert_contact:(ContactsEntry *)newcontact
{
    
	//NSManagedObjectContext *addingContext = [[NSManagedObjectContext alloc] init];
    
    
	//NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:@"Recents" inManagedObjectContext:addingContext];

    
    NSManagedObjectContext *managedObjectContext = [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] managedObjectContext];
	
    NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:@"Contacts" inManagedObjectContext:managedObjectContext];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.section_key] forKey:@"section_key"];     
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.firstname] forKey:@"firstname"];     
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.lastname] forKey:@"lastname"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.company] forKey:@"company"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.other] forKey:@"other"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", newcontact.secureid] forKey:@"secureid"];
    NSLog(@">>>> %@ / %@ / %@ / %@ / %@",newcontact.firstname,newcontact.lastname,newcontact.company,newcontact.secureid,newcontact.other);
    
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] saveContext];
    return 0;
} 

#else
- (NSMutableArray *) load_keyword:(NSString *)keyword
{
    NSLog(@"######### load_keyword");
    
    NSMutableArray* tmparray =[[NSMutableArray alloc]init];//存放查詣結果
    
    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        //return -1;
    }
    
    //NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname FROM CONTACTS where "];
    NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname, company, other, secureid FROM CONTACTS where lastname like '%%%@%%' or firstname like '%%%@%%';",keyword,keyword];//查詢條件式
   
    
    NSLog(@"######### %@",querySQL);
    const char *query_stmt = [querySQL UTF8String];
    
    if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
    {
        NSLog(@"sqlite: database error");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        //return -1;
    }
    
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        
        //ContactEntry *contact = [[ContactEntry alloc] init];
     
        const char *firstname = (const char *) sqlite3_column_text(statement, 2);
        const char *lastname = (const char *) sqlite3_column_text(statement, 1);
        const char *other = (const char *) sqlite3_column_text(statement, 4);
        const char *company = (const char *) sqlite3_column_text(statement, 3); 
        const char *secureid = (const char *) sqlite3_column_text(statement, 5); 
        const char *cid = (const char *) sqlite3_column_text(statement, 0);           
        NSLog(@"########### firstname = %@",[NSString stringWithUTF8String:firstname]); 
        
        
        NSMutableDictionary* dict = [[NSMutableDictionary alloc]init];//one record
        [dict setObject:[NSString stringWithCString:firstname encoding:NSUTF8StringEncoding] forKey:@"firstname"];
        [dict setObject:[NSString stringWithCString:lastname encoding:NSUTF8StringEncoding] forKey:@"lastname"];
        [dict setObject:[NSString stringWithCString:other encoding:NSUTF8StringEncoding] forKey:@"company"];
        [dict setObject:[NSString stringWithCString:company encoding:NSUTF8StringEncoding] forKey:@"other"];
        [dict setObject:[NSString stringWithCString:secureid encoding:NSUTF8StringEncoding] forKey:@"secureid"];
        [dict setObject:[NSString stringWithCString:cid encoding:NSUTF8StringEncoding] forKey:@"cid"];         
        //[dict setObject:load_numbers:contact withId:sqlite3_column_int(statement, 0)];
        [tmparray addObject:dict];
        [dict release];
    }
    //[tmparray release];
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    
    return [tmparray autorelease];//返回查询结果数组
    //return 0;
}

- (NSMutableArray *) load_data
{
    NSLog(@"######### load_data");
    
    NSMutableArray* tmparray =[[NSMutableArray alloc]init];//存放查詣結果
    
    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        //return -1;
    }
    
    //NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname FROM CONTACTS where "];
    NSString *querySQL = [NSString stringWithFormat: @"SELECT id, firstname, lastname, company, secureid, other  FROM CONTACTS ORDER BY firstname ASC;"];//查詢條件式
    
    
    NSLog(@"######### %@",querySQL);
    const char *query_stmt = [querySQL UTF8String];
    
    if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
    {
        NSLog(@"sqlite: database error");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        //return -1;
    }
    
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        
        //ContactEntry *contact = [[ContactEntry alloc] init];
        //const char *firstname = (const char *) sqlite3_column_text(statement, 2);
        const char *cid = (const char *) sqlite3_column_text(statement, 0);
        const char *firstname = (const char *) sqlite3_column_text(statement, 1);
        const char *lastname = (const char *) sqlite3_column_text(statement, 2);
        const char *other = (const char *) sqlite3_column_text(statement, 5);
        const char *company = (const char *) sqlite3_column_text(statement, 3); 
        const char *secureid = (const char *) sqlite3_column_text(statement, 4);   
        NSLog(@"########### firstname = %@",[NSString stringWithUTF8String:firstname]); 
        //NSString key_tmp = [[NSString alloc] initWithFormat:@"%@",key];
        //NSString *abc = [NSString stringWithCString:key encoding:NSUTF8StringEncoding] ;
        NSMutableDictionary* dict = [[NSMutableDictionary alloc]init];//one record
        [dict setObject:[NSString stringWithCString:firstname encoding:NSUTF8StringEncoding] forKey:@"firstname"];
        [dict setObject:[NSString stringWithCString:lastname encoding:NSUTF8StringEncoding] forKey:@"lastname"];
        [dict setObject:[NSString stringWithCString:company encoding:NSUTF8StringEncoding] forKey:@"company"];
        [dict setObject:[NSString stringWithCString:other encoding:NSUTF8StringEncoding] forKey:@"other"];
        [dict setObject:[NSString stringWithCString:secureid encoding:NSUTF8StringEncoding] forKey:@"secureid"]; 
        [dict setObject:[NSString stringWithCString:cid encoding:NSUTF8StringEncoding] forKey:@"cid"];        
        //[dict setObject:[NSString stringWithCString:key encoding:NSUTF8StringEncoding] forKey:@"key"];
        
       
        //[self load_numbers:contact withId:sqlite3_column_int(statement, 0)];
        [tmparray addObject:dict];
        
        [dict release];

    }

    //[tmparray release];
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    
    return [tmparray autorelease];//返回查询结果数组
    //return 0;
}

- (int) insert_contact:(ContactEntry *)contact
{
    NSLog(@"########## insert_contact");
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *insertSQL = [NSString stringWithFormat: @"INSERT INTO CONTACTS (lastname, firstname, company, other, secureid) VALUES (\"%@\", \"%@\", \"%@\", \"%@\", \"%@\")", contact.lastname, contact.firstname,  contact.company, contact.other, contact.phone_string];
    NSLog(@"################ %@",insertSQL);
  const char *insert_stmt = [insertSQL UTF8String];
  
  sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"Failed to add contact");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);

  //get contactid
    // add mutablearray phone number
  NSString *querySQL = [NSString stringWithFormat: @"SELECT id FROM CONTACTS WHERE lastname=\"%@\" AND firstname=\"%@\" LIMIT 1", contact.lastname, contact.firstname];
  
  const char *query_stmt = [querySQL UTF8String];
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  if (sqlite3_step(statement) != SQLITE_ROW)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }

  int contactid = sqlite3_column_int(statement, 0);
  sqlite3_finalize(statement);
  
  for( SipNumber *number in contact.phone_numbers)
  {
    insertSQL = [NSString stringWithFormat: @"INSERT INTO NUMBERS (contactid, number, phonetype) VALUES (%i, \"%@\", \"%@\")", contactid, number.phone_number, number.phone_type];
      NSLog(@"################ %@",insertSQL);

    insert_stmt = [insertSQL UTF8String];
    sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
    {
      NSLog(@"Failed to add contact");
      sqlite3_finalize(statement);
      sqlite3_close(myFavoriteContactDb);
      return -1;
    }
    sqlite3_finalize(statement);
  }
  
  
  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) remove_contact:(ContactEntry *)contact
{
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  //get contactid
  NSString *querySQL = [NSString stringWithFormat: @"SELECT id FROM CONTACTS WHERE lastname=\"%@\" AND firstname=\"%@\" LIMIT 1", contact.lastname, contact.firstname];
  
  const char *query_stmt = [querySQL UTF8String];
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  if (sqlite3_step(statement) != SQLITE_ROW)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  int contactid = sqlite3_column_int(statement, 0);
  sqlite3_finalize(statement);
 
  
  //remove all numbers:
  querySQL = [NSString stringWithFormat: @"DELETE FROM NUMBERS WHERE contactid = ?"];
  query_stmt = [querySQL UTF8String];
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }

  sqlite3_bind_int(statement, 1, contactid);

  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"sqlite: failed to remove numbers");
  }
  sqlite3_finalize(statement);

  //remove all numbers:
  querySQL = [NSString stringWithFormat: @"DELETE FROM CONTACTS WHERE id = ?"];
  query_stmt = [querySQL UTF8String];
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  
  sqlite3_bind_int(statement, 1, contactid);
  
  if (sqlite3_step(statement) != SQLITE_DONE)
  {
    NSLog(@"sqlite: failed to remove contact");
    sqlite3_finalize(statement);
    sqlite3_close(myFavoriteContactDb);
    return -1;
  }
  sqlite3_finalize(statement);

  sqlite3_close(myFavoriteContactDb);
  return 0;
}

- (int) load_numbers:(ContactEntry*)contact withId:(int)contactid
{
  sqlite3_stmt    *statement;
  NSString *querySQL = [NSString stringWithFormat: @"SELECT number, phonetype FROM NUMBERS WHERE contactid=\"%i\"", contactid];
  
  const char *query_stmt = [querySQL UTF8String];
  
  if (sqlite3_prepare_v2(myFavoriteContactDb, query_stmt, -1, &statement, NULL) != SQLITE_OK)
  {
    NSLog(@"sqlite: database error");
    sqlite3_finalize(statement);
    return -1;
  }
  
  while (sqlite3_step(statement) == SQLITE_ROW)
  {
    SipNumber *sip_number = [SipNumber alloc];
    const char *number = (const char *) sqlite3_column_text(statement, 0);
    const char *phonetype = (const char *) sqlite3_column_text(statement, 1);
    
    [sip_number setPhone_number: [NSString stringWithUTF8String:number]];
    [sip_number setPhone_type: [NSString stringWithUTF8String:phonetype]];
    [contact.phone_numbers addObject:sip_number];
  }
  
  sqlite3_finalize(statement);
  return 0;
}

- (int) load_contacts:(NSMutableArray*)arrayContact
{
    NSLog(@"######### load_contacts");
  sqlite3_stmt    *statement;
  
  const char *dbpath = [databasePath UTF8String];
  
  if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
  {
    NSLog(@"sqlite: database not opened");
    return -1;
  }
  
  NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname FROM CONTACTS"];
  NSLog(@"######### %@",querySQL);
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
      
    ContactEntry *contact = [[ContactEntry alloc] init];
    const char *firstname = (const char *) sqlite3_column_text(statement, 2);
    const char *lastname = (const char *) sqlite3_column_text(statement, 1);
      NSLog(@"########### firstname = %@",[NSString stringWithUTF8String:firstname]);  
    if (firstname)
      [contact setFirstname: [NSString stringWithUTF8String:firstname]];
    if (lastname)
      [contact setLastname: [NSString stringWithUTF8String:lastname]];
    
    [self load_numbers:contact withId:sqlite3_column_int(statement, 0)];
    [arrayContact addObject:contact];
  }
  
  sqlite3_finalize(statement);
  sqlite3_close(myFavoriteContactDb);
  return 0;
}


- (int *) update_contact:(ContactEntry *)contact
{
    NSLog(@"######### update_contact");
    

    sqlite3_stmt    *statement;
    
    const char *dbpath = [databasePath UTF8String];
    
    if (sqlite3_open(dbpath, &myFavoriteContactDb) != SQLITE_OK)
    {
        NSLog(@"sqlite: database not opened");
        //return -1;
    }
    
    //NSString *querySQL = [NSString stringWithFormat: @"SELECT id, lastname, firstname FROM CONTACTS where "];
    NSString *querySQL = [NSString stringWithFormat: @"UPDATE CONTACTS SET lastname = \"%@\" , firstname = \"%@\", company = \"%@\", other =\"%@\" , secureid = \"%@\" where id = \"%@\" ",contact.lastname,contact.firstname,contact.company, contact.other, contact.phone_string,contact.cid];//查詢條件式

    NSLog(@"################ %@",querySQL);
    const char *insert_stmt = [querySQL UTF8String];
    
    sqlite3_prepare_v2(myFavoriteContactDb, insert_stmt, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        NSLog(@"Failed to add contact");
        sqlite3_finalize(statement);
        sqlite3_close(myFavoriteContactDb);
        return -1;
    }
    sqlite3_finalize(statement);
    
    sqlite3_close(myFavoriteContactDb);

    //return 0;
}

#endif
@end
