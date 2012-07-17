//
//  RecentsViewController.m
//  vbyantisipgui
//
//  Created by  on 2012/7/2.
//  Copyright (c) 2012年 antisip. All rights reserved.
//
#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "vbyantisipAppDelegate.h"
#import "UIViewControllerDialpad.h"
#import "RecentsCellTableView.h"
#import "detailHistoryViewController.h"
#import "RecentsViewController.h"
#import "NSString+SECUREID.h"
#import "NSDate-Utilities.h"
//#import "SqliteRecentsHelper.h"
//#import "SqliteContactHelper.h"

@interface RecentsViewController ()
 - (void)configureCell:(RecentsCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath;
- (void)configureCellDelete:(RecentsCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath atIndexButton:(BOOL)button;
@end

@implementation RecentsViewController

@synthesize tableView,searchIsLost,DBActive,name_dir;

@synthesize fetchedResultsController;// = __fetchedResultsController;
@synthesize managedObjectContext;
//@synthesize filteredListContent = _filteredListContent;
@synthesize addingManagedObjectContext;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
   // NSLog(@"########## viewDidLoad");
    searchIsLost = NO;
    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release];   
    
    //self.title = @"Contacts";
    self.navigationItem.title = NSLocalizedString(@"tabContacts",nil);
    
    
    if (self.managedObjectContext == nil) 
    { 
        self.managedObjectContext = [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] managedObjectContext]; 
        NSLog(@"After managedObjectContext: %@",  self.managedObjectContext);
    }
    

    
	NSError *error;
	if (![[self fetchedResultsController] performFetch:&error]) {
		NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
		//exit(-1);
        //self.filteredListContent = [NSMutableArray arrayWithCapacity:[[[self fetchedResultsController] fetchedObjects] count]];        
	}
    NSLog(@"After fetchedResultsController: %@",  self.fetchedResultsController);    
    self.navigationItem.title = NSLocalizedString(@"tabRecent",nil);
    /*
    UIBarButtonItem *rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemTrash target:self action:@selector(deleteAllItems)];
    
    self.navigationItem.rightBarButtonItem = rightBarButtonItem;
    
    [rightBarButtonItem release];
    */
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
    // NSMutableArray *searchButtons = [[NSMutableArray alloc] initWithCapacity:2];  
    
    
    UISegmentedControl *segmentedControl = [ [UISegmentedControl alloc] initWithItems:[NSArray arrayWithObjects:NSLocalizedString(@"btnAllCall", nil), NSLocalizedString(@"btnLostCall", nil),nil]]; 
    //segmentedControl.segmentedControlStyle = 
    segmentedControl.segmentedControlStyle = UISegmentedControlStyleBar; 
    segmentedControl.selectedSegmentIndex = 0; 
    [segmentedControl addTarget:self action:@selector(segmentAction:) forControlEvents:UIControlEventValueChanged]; 

    
    UIBarButtonItem *seg = [[UIBarButtonItem alloc] initWithCustomView:segmentedControl];
    self.navigationItem.leftBarButtonItem = seg; 
    self.name_dir = nil;
    self.name_dir = [self get_contact_name_array];
    
    [segmentedControl release];
    
}

- (NSMutableDictionary *)get_contact_name_array{
    myContactDb = [[SqliteContactHelper alloc] autorelease];
    [myContactDb open_database];
    NSMutableDictionary *dir = [[[NSMutableDictionary alloc] init] autorelease];
    dir = [myContactDb find_all_contact_name];
    
    return dir;
}

- (void)viewWillAppear:(BOOL)animated 
{

   // NSLog(@"####### RecentsViewController viewWillAppear ");
    //[self fetchedResultsController] = nil;
    
    [super viewWillAppear:animated];
    //[self.tableView reloadData];
    self.editing = NO;
    
    self.fetchedResultsController.delegate = self;
    
    if(searchIsLost==NO){
        [self searchAll];
    }
    else [self searchLost];
  
    self.name_dir = nil;
    self.name_dir = [self get_contact_name_array];
    
    
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
		return 50;
}


- (void)viewDidUnload
{
    [self setTableView:nil];
    [self setDBActive:NO];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
   // NSLog(@"############ numberOfSectionsInTableView");

    return [[fetchedResultsController sections] count];
    //return [[self->sections allKeys] count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
   // NSLog(@"############# tableView numberOfRowsInSection");

    NSInteger numberOfRows = 0;
    
    if ([[self.fetchedResultsController sections] count] > 0) {
        id <NSFetchedResultsSectionInfo> sectionInfo = [[self.fetchedResultsController sections] objectAtIndex:section];
        numberOfRows = [sectionInfo numberOfObjects];
    }
   // NSLog(@"############# count = %d",numberOfRows);
    return numberOfRows;    
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
   // NSLog(@"cellForRowAtIndexPath indexPath %i",indexPath.row);
    // Dequeue or if necessary create a RecipeTableViewCell, then set its recipe to the recipe for the current row.
    static NSString *CellIdentifier = @"CellContact";
    
    RecentsCellTableView *Cell = (RecentsCellTableView *)[self.tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    //if(DBActive==YES){
       // NSLog(@"########## reload cell indexPath %i",indexPath.row);
        Cell = nil;
        //DBActive = NO;
   // }  
    
    if (Cell == nil) {
        Cell = [[[RecentsCellTableView alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
		Cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        Cell.showsReorderControl = YES;
       
    }
    //NSLog(@"cellForRowAtIndexPath : %d",indexPath.row);
    //Cell.selected = YES;
	[self configureCell:Cell atIndexPath:indexPath];
    [Cell.contentView reloadInputViews];
    return Cell;
}



- (void)configureCell:(RecentsCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath {
    // Configure the cell

    //NSLog(@">>>>>>>> configureCell %i",indexPath.row);
    Recents *recent = nil;

    recent = [fetchedResultsController objectAtIndexPath:indexPath];

    cell.recent = recent;
    NSString *name = [[NSString alloc] initWithFormat:@"%@",[name_dir objectForKey:[NSString stringWithFormat:@"%@",recent.secureid ]]];
   // NSLog(@" >>>>>>>>> name : %@",name);
    if(name !=nil && ![name isEqualToString:@"(null)"]){
        cell.nameLabel.text = name;
    }
    else cell.nameLabel.text = recent.secureid;

    [name release];
    
    cell.addbutton = NO;
}


- (void)configureCellDelete:(RecentsCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath atIndexButton:(BOOL)button {
    // Configure the cell
    
    //NSLog(@">>>>>>>> configureCell %i",indexPath.row);
    Recents *recent = nil;
    
    recent = [fetchedResultsController objectAtIndexPath:indexPath];
    
    cell.recent = recent;
    NSString *name = [[NSString alloc] initWithFormat:@"%@",[name_dir objectForKey:[NSString stringWithFormat:@"%@",recent.secureid ]]];
   // NSLog(@" >>>>>>>>> name : %@",name);
    if(name !=nil && ![name isEqualToString:@"(null)"]){
        cell.nameLabel.text = name;
    }
    else cell.nameLabel.text = recent.secureid;
    
    [name release];
      //  NSLog(@"cell.addbutton = %@",cell.addbutton==YES?@"YES":@"NO");

    cell.addbutton = button;

   
    NSLog(@"cell.addbutton = %@",cell.addbutton==YES?@"YES":@"NO");    
    

}



- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    //NSLog(@"############# tableView canMoveRowAtIndexPath");
    return NO;
}


- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    // NSLog(@"############# tableView moveRowAtIndexPath");
    //handle movement of UITableViewCells here
    //UITableView cells don't just swap places; one moves directly to an index, others shift by 1 position. 
}
/*
//自定義劃動時del按鈕內容 
-(NSString *)tableView:(UITableView *)d_tableView titleForDeleteConfirmationButtonForRowAtIndexPath:(NSIndexPath *)indexPath      
{      
    NSLog(@"########## titleForDeleteConfirmationButtonForRowAtIndexPath :%i",indexPath.row);
  
    if(UITableViewCellEditingStyleDelete){

    [self configureCellDelete:[d_tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath button:YES];
    }
   // [self configureCellDelete:Cell atIndexPath:indexPath];
    return  @"Delete";      
}
*/

- (void)tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath{
    NSLog(@"willBeginEditingRowAtIndexPath:%i",indexPath.row);
    
    [self configureCellDelete:[tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath atIndexButton:YES];
    //[self configureCell:[tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath];

}
- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath{
    NSLog(@"didEndEditingRowAtIndexPath:%i",indexPath.row);
    [self configureCellDelete:[tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath atIndexButton:NO];
    //[self configureCell:[tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath];
}

//滑動列編輯
-(UITableViewCellEditingStyle)tableView:(UITableView *)d_tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
   // NSLog(@"########### editingStyleForRowAtIndexPath");
    
    UITableViewCellEditingStyle result = UITableViewCellEditingStyleNone;
    
    if([d_tableView isEqual:self.tableView] == YES){
        result = UITableViewCellEditingStyleDelete;
      //  NSLog(@"########### UITableViewCellEditingStyleDelete");
    }
    else{
    
   // NSLog(@"########### UITableViewCellEditingStyleDelete NO");
    }
    
    return result;

}



// Override to support editing the table view.//點擊編輯
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{

    if (editingStyle == UITableViewCellEditingStyleDelete) {
        
        // Delete the managed object for the given index path
        NSManagedObjectContext *context = [fetchedResultsController managedObjectContext];
        [context deleteObject:[fetchedResultsController objectAtIndexPath:indexPath]];

        // Save the context.
        NSError *error = nil;
        if (![context save:&error]) {
            /*
             Replace this implementation with code to handle the error appropriately.
             
             abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. 
             */
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
            //exit(-1);
        }
    }
    
    
}



#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    //NSLog(@"############# tableView didSelectRowAtIndexPath");

    UIViewController *nextViewController = nil;
    nextViewController = [[detailHistoryViewController alloc] initWithNibName:@"detailHistoryViewController" bundle:nil];
    

    NSMutableString *phonem = [[NSMutableString alloc] init];
    
    Recents *recent = nil;

    recent = [fetchedResultsController objectAtIndexPath:indexPath];
    
    
    //NSLog(@"############# run deselectRowAtIndexPath");
    if ([recent.remoteuri rangeOfString : @"sip:"].location == NSNotFound)
    {
        phonem = [[recent.remoteuri mutableCopy] autorelease];
        [phonem replaceOccurrencesOfString:@"-"
                                withString:@""
                                   options:NSLiteralSearch 
                                     range:NSMakeRange(0, [phonem length])];
        //[phonem initWithFormat:@"%@",phonem];
        
    }
    else{
        phonem = [[recent.remoteuri mutableCopy] autorelease];
    }
    //NSLog(@"########### set var value phonem :%@ / remoteid:%@",phonem,recent.remoteuri);
    /*
    myContactDb = [SqliteContactHelper alloc];
    [myContactDb open_database];
    NSString *name = [myContactDb find_contact_name:[NSString stringWithFormat:@"%@",recent.secureid]];
    [myContactDb release];
    */
    
    NSString *name = [[[NSString alloc] initWithFormat:@"%@",[name_dir objectForKey:[NSString stringWithFormat:@"%@",recent.secureid ]]] autorelease];
    if(name ==nil || [name isEqualToString:@"(null)"]){
        name = @"Unknown";
        //((detailHistoryViewController *)nextViewController).showAddButton = YES;
        ((detailHistoryViewController *)nextViewController).showAddButton = NO;
    }

    ((detailHistoryViewController *)nextViewController).fnstring = name;     
    ((detailHistoryViewController *)nextViewController).secureid = [NSString stringWithFormat:@"%@",recent.secureid];
    ((detailHistoryViewController *)nextViewController).phonenum = phonem;
    // ((detailHistoryViewController *)nextViewController).phonem = indexPath.row;

    //[phonem release];
    
    //[phonem1 release];
    if (nextViewController) {
        [nextViewController setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:nextViewController animated:YES];  
        
        [nextViewController release];
    }    
    
    
}



- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
   // NSLog(@"############  titleForHeaderInSection %i",section);
    /*
     if(self.searchIsActive){
     return nil;
     }
     else
     return [[[fetchedResultsController sections] objectAtIndex:section] name];
     */
    //NSDate *nowDate = [NSDate date];

    NSString *rawDateStr = [[[self.fetchedResultsController sections] objectAtIndex:section] name];

    id <NSFetchedResultsSectionInfo> sectionInfo = 
    [[self.fetchedResultsController sections] objectAtIndex:section];
    
    //convert default date string to NSDate...
    NSDateFormatter *formatter = [[[NSDateFormatter alloc] init] autorelease];
    [formatter setDateFormat:@"yyyy/MM/dd"];
    NSDate *nowdate = [NSDate date];                                                            
    
    if([[formatter stringFromDate:nowdate] isEqualToString:rawDateStr]){
        //return  @"Today";
        return   NSLocalizedString(@"Today",nil);;
    }
    else if([[formatter stringFromDate:[NSDate dateYesterday]] isEqualToString:rawDateStr]){
        return   NSLocalizedString(@"Yesterday",nil);;
        
    }   
   
    //if (!(section == 0 && [self.tableView numberOfSections] == 1)) {
    NSLog(@">>>> have section :%@",[[[self.fetchedResultsController sections] objectAtIndex:section] name]);
   /*
    id <NSFetchedResultsSectionInfo> sectionInfo = 
    [[self.fetchedResultsController sections] objectAtIndex:section];
*/
        //return [sectionInfo name];  
    return [[[self.fetchedResultsController sections] objectAtIndex:section] name];
   // }
   // return nil ;
}


#pragma mark - Fetched results controller

- (NSFetchedResultsController *)fetchedResultsController
{
    //NSLog(@"####### fetchedResultsController");
    
    if (fetchedResultsController != nil ) {
        return fetchedResultsController;
    }
    NSLog(@">>>>>>>>>>>>>> run reload data");
    // Set up the fetched results controller.
    // Create the fetch request for the entity.
    NSFetchRequest *fetchRequest = [[[NSFetchRequest alloc] init]autorelease];
    // Edit the entity name as appropriate.
    NSEntityDescription *entity = [NSEntityDescription entityForName:@"Recents" inManagedObjectContext:self.managedObjectContext];
    [fetchRequest setEntity:entity];
    

    // Set the batch size to a suitable number.
    [fetchRequest setFetchBatchSize:20];
    [NSFetchedResultsController deleteCacheWithName:@"Master"];
    if(searchIsLost==YES){
       // NSLog(@">>>>>>>>>>>>>>>> searchInLost = YES");
        NSPredicate * predicate = [NSPredicate predicateWithFormat:@"  direction ='1' and sip_code !='200'"];

        [fetchRequest setPredicate:predicate];
    }
    
    
    // Edit the sort key as appropriate.

    /*
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"start_date" ascending:NO];
    NSArray *sortDescriptors = [[NSArray alloc] initWithObjects:sortDescriptor, nil];
    
    [fetchRequest setSortDescriptors:sortDescriptors];
    */

   
    NSSortDescriptor *sortByReleaseDate = [[[NSSortDescriptor alloc] initWithKey:@"start_date" ascending:NO] autorelease];
    NSSortDescriptor *sortByName = [[[NSSortDescriptor alloc] initWithKey:@"section_key" ascending:NO] autorelease];
    NSArray *sortDescriptors = [[[NSArray alloc] initWithObjects:sortByReleaseDate,sortByName, nil] autorelease];
    [fetchRequest setSortDescriptors:sortDescriptors];


    //NSString *sectionKey = [[NSString alloc] init];
    //sectionKey = @"nameFirstLetter";
    // Edit the section name key path and cache name if appropriate.
    // nil for section name key path means "no sections".
    
    NSFetchedResultsController *aFetchedResultsController = [[[NSFetchedResultsController alloc] initWithFetchRequest:fetchRequest managedObjectContext:self.managedObjectContext sectionNameKeyPath:@"section_key" cacheName:@"Master"] autorelease];
    //@"section_key"

    
    aFetchedResultsController.delegate = self;
    self.fetchedResultsController = aFetchedResultsController;
    
	NSError *error = nil;
	if (![self.fetchedResultsController performFetch:&error]) {

	    NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
	    abort();
	}

    //[fetchRequest autorelease];
    //[sortDescriptors release];
    //[sortByName release];
    //[sortByReleaseDate release];     
    //[sortDescriptor autorelease];
    //[sortDescriptors autorelease];
    //[aFetchedResultsController autorelease];
    return fetchedResultsController;
    
    
}    

- (void)controllerWillChangeContent:(NSFetchedResultsController *)controller
{
    //NSLog(@"############ controllerWillChangeContent");
    [self.tableView beginUpdates];
    DBActive = YES;

}

- (void)controller:(NSFetchedResultsController *)controller didChangeSection:(id <NSFetchedResultsSectionInfo>)sectionInfo
           atIndex:(NSUInteger)sectionIndex forChangeType:(NSFetchedResultsChangeType)type
{
    //NSLog(@"######## controller didChangeSection");    
    
    switch(type) {
        case NSFetchedResultsChangeInsert:
            [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex]
                          withRowAnimation:UITableViewRowAnimationFade];
            break;
            
            /*
        case NSFetchedResultsChangeInsert:
            if (!((sectionIndex == 0) && ([self.tableView numberOfSections] == 1)))
             [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
            //[self.tableView selectRowAtIndexPath:sectionIndex animated:YES scrollPosition:0];
            break;            
*/
        case NSFetchedResultsChangeDelete:
            //NSLog(@"######  didChangeSection NSFetchedResultsChangeDelete");
            //if (!((sectionIndex == 0) && ([self.tableView numberOfSections] == 1) ))
                [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
         
            break;
        case NSFetchedResultsChangeMove:
            break;
        case NSFetchedResultsChangeUpdate: 
            [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
            [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex]
                          withRowAnimation:UITableViewRowAnimationFade];
            
           
            break;
        default:
            break;
    }    
    
    
    
    
    
}




- (void)controller:(NSFetchedResultsController *)controller didChangeObject:(id)anObject
       atIndexPath:(NSIndexPath *)indexPath forChangeType:(NSFetchedResultsChangeType)type
      newIndexPath:(NSIndexPath *)newIndexPath
{
 //   NSLog(@"######## controller didChangeObject");
    /*
     UITableView *tableView = self.tableView;
     
     switch(type) {
     case NSFetchedResultsChangeInsert:
     NSLog(@"######  didChangeObject NSFetchedResultsChangeInsert");
     [tableView insertRowsAtIndexPaths:[NSArray arrayWithObject:newIndexPath] withRowAnimation:UITableViewRowAnimationFade];
     break;
     
     case NSFetchedResultsChangeDelete:
     NSLog(@"######  didChangeObject NSFetchedResultsChangeDelete");
     [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
     break;
     
     case NSFetchedResultsChangeUpdate:
     [self configureCell:[tableView cellForRowAtIndexPath:indexPath] atIndexPath:indexPath];
     
     //        [self UITableViewCell:cellForRowAtIndexPath:indexPath]; 
     
     break;
     
     case NSFetchedResultsChangeMove:
     //[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
     //[tableView insertRowsAtIndexPaths:[NSArray arrayWithObject:newIndexPath]withRowAnimation:UITableViewRowAnimationFade];
     break;
     }
     
     */
    
    
    switch(type) {
        case NSFetchedResultsChangeInsert:
            NSLog(@">>>>>>>>> NSFetchedResultsChangeInsert  %i",indexPath.row);
            [self.tableView insertRowsAtIndexPaths:[NSArray arrayWithObject:newIndexPath] withRowAnimation:UITableViewRowAnimationFade];//UITableViewRowAnimationFade
            
            
            //[self.tableView deleteSections:[NSIndexSet indexSetWithIndex:[indexPath section]] withRowAnimation:UITableViewRowAnimationNone];            
           // [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationFade];            
            //[self configureCell:[tableView cellForRowAtIndexPath:newIndexPath] atIndexPath:newIndexPath];
            //[self.tableView select:newIndexPath];
            break;
        case NSFetchedResultsChangeDelete:
            NSLog(@">>>>>>>>> NSFetchedResultsChangeDelete %i",indexPath.row);            
            [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
            

            
            break;
        case NSFetchedResultsChangeUpdate:
        {
            NSLog(@">>>>>>>>> NSFetchedResultsChangeUpdate ");            
            NSString *sectionKeyPath = [controller sectionNameKeyPath];
            if (sectionKeyPath == nil)
                break;
            NSManagedObject *changedObject = [controller objectAtIndexPath:indexPath];
            NSArray *keyParts = [sectionKeyPath componentsSeparatedByString:@"."];
            id currentKeyValue = [changedObject valueForKeyPath:sectionKeyPath];
            for (int i = 0; i < [keyParts count] - 1; i++) {
                NSString *onePart = [keyParts objectAtIndex:i];
                changedObject = [changedObject valueForKey:onePart];
            }
            sectionKeyPath = [keyParts lastObject];
            NSDictionary *committedValues = [changedObject committedValuesForKeys:nil];
            
            if ([[committedValues valueForKeyPath:sectionKeyPath] isEqual:currentKeyValue])
                break;
            
            NSUInteger tableSectionCount = [self.tableView numberOfSections];
            NSUInteger frcSectionCount = [[controller sections] count];
            if (tableSectionCount != frcSectionCount) {
                // Need to insert a section
                NSArray *sections = controller.sections;
                NSInteger newSectionLocation = -1;
                for (id oneSection in sections) {
                    NSString *sectionName = [oneSection name];
                    if ([currentKeyValue isEqual:sectionName]) {
                        newSectionLocation = [sections indexOfObject:oneSection];
                        break;
                    }
                }
                if (newSectionLocation == -1)
                    return; // uh oh
                
                if (!((newSectionLocation == 0) && (tableSectionCount == 1) && ([self.tableView numberOfRowsInSection:0] == 0)))
                    [self.tableView insertSections:[NSIndexSet indexSetWithIndex:newSectionLocation] withRowAnimation:UITableViewRowAnimationFade];
                NSUInteger indices[2] = {newSectionLocation, 0};
                newIndexPath = [[[NSIndexPath alloc] initWithIndexes:indices length:2] autorelease];
            }
 
            break;
        }
        case NSFetchedResultsChangeMove:
            NSLog(@">>>>>>>>> NSFetchedResultsChangeMove ");              
            if (newIndexPath != nil) {
                
                NSUInteger tableSectionCount = [self.tableView numberOfSections];
                NSUInteger frcSectionCount = [[controller sections] count];
                if (frcSectionCount >= tableSectionCount) 
                    [self.tableView insertSections:[NSIndexSet indexSetWithIndex:[newIndexPath section]] withRowAnimation:UITableViewRowAnimationNone];
                else 
                    if (tableSectionCount > 1) 
                        [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:[indexPath section]] withRowAnimation:UITableViewRowAnimationNone];
                
                
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
                [self.tableView insertRowsAtIndexPaths: [NSArray arrayWithObject:newIndexPath]
                                      withRowAnimation: UITableViewRowAnimationRight];
                
            }
            else {
                [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:[indexPath section]] withRowAnimation:UITableViewRowAnimationFade];
            }
            break;
        default:
            break;
    } 
}

- (void)controllerDidChangeContent:(NSFetchedResultsController *)controller
{
   // NSLog(@"##### controllerDidChangeContent");
    [self.tableView endUpdates];
   // [self.tableView reloadData];
}



// add item

-(void)segmentAction:(UISegmentedControl *)Seg{
    NSInteger Index = Seg.selectedSegmentIndex;
    NSLog(@"########## Index %i", Index);
    switch (Index) {
        case 0:
            [self searchAll];
            break;
        case 1:
            [self searchLost];
            break; 
        default:
            break;
    }
}

-(void)searchAll{
    
    NSLog(@"searchAll");    
    searchIsLost = NO; 
    //fetchedResultsController = nil;    
    NSLog(@">>>>>>>>>>>>>>>> searchInLost = NO");
    NSPredicate * predicate = [NSPredicate predicateWithFormat:@" secureid !='' "];
    [NSFetchedResultsController deleteCacheWithName:@"Master"];
    [[[self fetchedResultsController] fetchRequest] setPredicate:predicate];
    
    
    //NSLog(@"After fetchedResultsController: %@",  self.fetchedResultsController);
   // NSPredicate * predicate = [NSPredicate predicateWithFormat:@" secureid!='' "];
    //[NSFetchedResultsController deleteCacheWithName:@"Master"];
    //[[[self fetchedResultsController] fetchRequest] setPredicate:predicate]; 
     
   
    NSError *error = nil;
    if (![[self fetchedResultsController] performFetch:&error]) {

        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    } 
        
    [self.tableView reloadData];
}

-(void)searchLost{
    NSLog(@"searchLost");
    //self.filteredListContent = nil;
    //fetchedResultsController = nil;
    searchIsLost = YES;
    //fetchedResultsController = nil;
    //NSLog(@"After fetchedResultsController: %@",  self.fetchedResultsController);
    NSLog(@">>>>>>>>>>>>>>>> searchInLost = YES");
    NSPredicate * predicate = [NSPredicate predicateWithFormat:@"  direction ='1' and sip_code !='200'"];
    [NSFetchedResultsController deleteCacheWithName:@"Master"];
    [[[self fetchedResultsController] fetchRequest] setPredicate:predicate];
        
       
    NSError *error = nil;
    if (![[self fetchedResultsController] performFetch:&error]) {

        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    } 
 
/*
    
    
    NSPredicate * predicate = [NSPredicate predicateWithFormat:@"direction ='1' and sip_code !='200'"];
    [[[self fetchedResultsController] fetchedObjects] filteredArrayUsingPredicate:predicate];
  
    
    fetchedResultsController = nil;
    //fetchedResultsController.delegate = nil;
    
    [self fetchedResultsController];*/
    [self.tableView reloadData];
}

- (void)deleteAllItems
{

}



- (int)dial:(NSString*)phonem
{
    int res;
    if ([gAppEngine isConfigured]==FALSE) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account Creation", @"Account Creation") 
                                                        message:NSLocalizedString(@"Please configure your settings in the iphone preference panel.", @"Please configure your settings in the iphone preference panel.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    if (![gAppEngine isStarted]) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"No Connection", @"No Connection") 
                                                        message:NSLocalizedString(@"The service is not available.", @"The service is not available.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
    if ([gAppEngine getNumberOfActiveCalls]>3) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Max Active Call Reached", @"Max Active Call Reached") 
                                                        message:NSLocalizedString(@"You already have too much active call.", @"You already have too much active call.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
    res = [gAppEngine amsip_start:phonem withReferedby_did:0];
    if (res<0) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Syntax Error", @"Syntax Error") 
                                                        message:NSLocalizedString(@"Check syntax of your callee sip url.", @"Check syntax of your callee sip url.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
    vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
    if ([appDelegate.tabBarController selectedIndex]!=1)
    {
        [appDelegate.tabBarController setSelectedIndex: 1];
    }
    UIViewControllerDialpad *_viewControllerDialpad = (UIViewControllerDialpad *)appDelegate->viewControllerDialpad;
    [_viewControllerDialpad pushCallControlList];
    return res;
}


- (void)saveContext
{
    NSLog(@">>>>>>> saveContext");
    
    NSError *error = nil;
   // NSManagedObjectContext *managedObjectContext = self.managedObjectContext;
    if (self.managedObjectContext != nil)
    {
        if ([self.managedObjectContext hasChanges] && ![self.managedObjectContext save:&error])
        {
            /*
             Replace this implementation with code to handle the error appropriately.
             
             abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. 
             */
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
        } 
    }
}


@end
