//
//  contactUIViewController.m
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//
//#import "NetworkTrackingDelegate.h"
#include <amsip/am_options.h>
#import <CoreData/CoreData.h>

//#import "UIViewControllerDialpad.h"
//#import "vbyantisipAppDelegate.h"
#import "vbyantisipAppDelegate.h"
#import "AppEngine.h"
//#import "ContactEntry.h"
//#import "detailHistoryViewController.h"

#import "ContactsViewController.h"
//#import "UITableViewCellFavorites.h"
#import "ContactCellTableView.h"
#import "ContactDetailViewController.h"
//#import "newContactUIViewController.h"


#import "Contacts.h"

@interface ContactsViewController ()
- (void)configureCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

@end

@implementation ContactsViewController
@synthesize aiv;
@synthesize noTouchView;
@synthesize indexArray;

@synthesize contactUITableView;
//@synthesize tableView;



@synthesize fetchedResultsController, managedObjectContext, addingManagedObjectContext;
@synthesize contactUISearchBar,isReadyOnly;
@synthesize filteredListContent = _filteredListContent;

@synthesize searchIsActive = _searchIsActive;

//@synthesize sections;
//@synthesize dataSource;


+(void)_keepAtLinkTime
{
    return;
}

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

    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release];   
    
    //self.title = @"Contacts";
    self.navigationItem.title = NSLocalizedString(@"tabContacts",nil);

    //indexArray = [[[NSMutableArray alloc]init] autorelease];
    
    if (self.managedObjectContext == nil) 
    { 
        self.managedObjectContext = [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] managedObjectContext]; 
        //NSLog(@"After managedObjectContext: %@",  self.managedObjectContext);
    }

 
    
    
    if (isReadyOnly == NO) {    
	// Do any additional setup after loading the view, typically from a nib.
    // Set up the edit and add buttons.
    self.navigationItem.leftBarButtonItem = self.editButtonItem;
    }    

    
    
	UISearchBar *searchBar = [[UISearchBar alloc] initWithFrame:CGRectMake(0, 0, self.tableView.frame.size.width, 0)];
    
	searchBar.delegate = self;
	//searchBar.showsCancelButton = YES;
	//searchBar.scopeButtonTitles = [NSArray arrayWithObjects:@"Case Sensitive", @"Case Insensitive", nil];
	[searchBar sizeToFit];  
    
	self.tableView.tableHeaderView = searchBar;
 
    
	UISearchDisplayController *searchDisplayController = [[UISearchDisplayController alloc] initWithSearchBar:searchBar contentsController:self];  
    
    [searchBar release];
    
	[self performSelector:@selector(setSearchDisplayController:) withObject:searchDisplayController];
    
    [searchDisplayController setDelegate:self];
    [searchDisplayController setSearchResultsDataSource:self];
    [searchDisplayController setSearchResultsDelegate:self];
    [searchDisplayController release];  
    
	NSError *error;
	if (![[self fetchedResultsController] performFetch:&error]) {
		NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
		//exit(-1);
        self.filteredListContent = [NSMutableArray arrayWithCapacity:[[[self fetchedResultsController] fetchedObjects] count]];        
	}
    
    if (isReadyOnly == NO) {    
    UIBarButtonItem *addButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(addNewContact)];//
    self.navigationItem.rightBarButtonItem = addButton;
    
    [addButton release];
    
        //for activity indicator
        //[self.view addSubview:noTouchView];
        [self.view addSubview:aiv];
    
    }
}

- (void)dealloc
{
    // [_detailViewController release];
    [fetchedResultsController release];
    [managedObjectContext release];
	[addingManagedObjectContext release];    
    [aiv release];
    [noTouchView release];
    [super dealloc];
}


- (void)viewDidUnload
{
   // NSLog(@"####### viewDidUnload");
    [contactUISearchBar resignFirstResponder];
    [self setContactUISearchBar:nil];
    [self setContactUITableView:nil];
    [self setIsReadyOnly:NO]; 
    [self setFilteredListContent:nil];
  //  [dataSource release];
    [self setAiv:nil];
    [self setNoTouchView:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}



- (void)viewWillAppear:(BOOL)animated 
{
    //NSLog(@">>>> viewWillAppear");    
    [super viewWillAppear:animated];
   // NSLog(@"########## viewWillAppear");   

    self.filteredListContent = nil;
    fetchedResultsController.delegate = self;

    if(self.searchIsActive){
        [[[self searchDisplayController] searchBar] setText:[self.searchDisplayController.searchBar text]];
        //NSLog(@"@@@@@@  searchIsActive YES / searchbar:%@ ",[self.searchDisplayController.searchBar text]);
        
    }
    else{
        
        [NSFetchedResultsController deleteCacheWithName:@"Master"];        
        
        NSError *error = nil;
        if (![[self fetchedResultsController] performFetch:&error]) {
            
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
        } 
        

    }
#if 0
    if (isReadOnly == NO) {
        if ([gAppEngine isStarted]==NO) {
            [aiv startAnimating];
        }
        [gAppEngine addRegistrationDelegate:self];
    }
#endif
    self.editing = NO;  
    //indexArray = nil;
    [self.tableView reloadData];
  
    
}

- (void)viewWillDisappear:(BOOL)animated 
{
    [super viewWillDisappear:animated];
    
    NSLog(@"########## viewWillDisappear"); 
    //if (isReadOnly == NO) {
    //    [gAppEngine removeRegistrationDelegate:self];
    //}
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
   //NSLog(@"############# count = %@",[[self.fetchedResultsController sections] count]);
    // Return the number of sections.
    //return [[fetchedResultsController sections] count];
    
	if (self.searchIsActive) {
        return 1;//[self.filteredListContent count];
        //NSLog(@"############# count = %d",[self.filteredListContent count]);
    }
    
    return [[fetchedResultsController sections] count];
    //return [[self->sections allKeys] count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 50;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
   // NSLog(@"############# tableView numberOfRowsInSection");
    //NSLog(@"############# count = %@",[[self.fetchedResultsController sections] count]);    
	if (self.searchIsActive) {
        return [self.filteredListContent count];
        //NSLog(@"############# count = %d",[self.filteredListContent count]);
    }
    
    NSInteger numberOfRows = 0;

    if ([[self.fetchedResultsController sections] count] > 0) {
        id <NSFetchedResultsSectionInfo> sectionInfo = [[self.fetchedResultsController sections] objectAtIndex:section];
        numberOfRows = [sectionInfo numberOfObjects];
    }
    //NSLog(@"############# count = %d",numberOfRows);
    return numberOfRows;
    
}

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    //NSLog(@"############ sectionIndexTitlesForTableView");
    /*
    if(self.searchIsActive){
        return nil;
    }
    
    
    NSMutableArray *indices = [NSMutableArray array];
    
    id <NSFetchedResultsSectionInfo> sectionInfo;
    
    for( sectionInfo in [fetchedResultsController sections] )
    {
        [indices addObject:[sectionInfo name]];
    }
    return indices; 
     */
    return  nil;
    /*
    
    NSLog(@">>>>>>>> sectionIndexTitlesForTableView");
    NSMutableArray* indexTitles = [NSMutableArray arrayWithObject:UITableViewIndexSearch];  // add magnifying glass
    [indexTitles addObjectsFromArray:[self.fetchedResultsController sectionIndexTitles]];
    return indexTitles;
*/
    //return [[self.indexArray allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];

   // return indexArray;
}


/*
//設定分類開頭標題
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section{
    NSLog(@"############ titleForHeaderInSection ");
    return [[[self->sections allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)] objectAtIndex:section];
}
*/
#if 0
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath 
{ 
    //NSLog(@"############ tableView cellForRowAtIndexPath");

    static NSString *CellIdentifier = @"CellContact";
    
    CellContactUITableView *cell1 = (CellContactUITableView *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];

    
    if (cell1!=nil) {
        // yes it had a cell we could reuse
        NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);
    } else {
       // NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);        
        // no cell to reuse, we have to create a new instance by loading it from the IB file
        [cellOwnerLoadContact loadMyNibFile:@"CellContactUITableView"];
        cell1 = (CellContactUITableView *)cellOwnerLoadContact.cell;
        
        NSLog(@"Loading cell from nib %@", CellIdentifier);
    }
    
    cell1.showsReorderControl=YES; 
    cell1.accessoryType=UITableViewCellAccessoryDisclosureIndicator;

    Contacts *contact = nil;
	if (self.searchIsActive) {
        //return [self.filteredListContent count];
        contact = [[self filteredListContent] objectAtIndex:[indexPath row]];
    }
    else{
        contact = [fetchedResultsController objectAtIndexPath:indexPath];
    }
    
    cell1.lastname.text= contact.lastname;// [[fetchedResultsController objectAtIndex:indexPath.row] objectForKey:@"lastname"];//[book objectForKey:@"lastname"];
    cell1.firstname.text = contact.firstname;// [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"firstname"];//[book objectForKey:@"firstname"];
    //cell1.actionButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self];
    
    //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
    return cell1;

}



- (void)configureCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
    //NSManagedObject *managedObject = [self.fetchedResultsController objectAtIndexPath:indexPath];
    //cell.textLabel.text = [NSString stringWithFormat:@"%@ %@" ,[[managedObject valueForKey:@"firstname"] description], [managedObject valueForKey:@"lastname"]];
    
    
	Contacts *contact = [fetchedResultsController objectAtIndexPath:indexPath];
	cell.textLabel.text = [NSString stringWithFormat:@"%@ %@" ,contact.firstname,contact.lastname];
    
    
}
#endif
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    // Dequeue or if necessary create a RecipeTableViewCell, then set its recipe to the recipe for the current row.
    static NSString *CellIdentifier = @"CellContact";
    
    ContactCellTableView *Cell = (ContactCellTableView *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (Cell == nil) {
        Cell = [[[ContactCellTableView alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
		Cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    
	[self configureCell:Cell atIndexPath:indexPath];
    
    return Cell;
}


- (void)configureCell:(ContactCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath {
    // Configure the cell
	//Recipe *recipe = (Recipe *)[fetchedResultsController objectAtIndexPath:indexPath];
    //cell.recipe = recipe;
    
    Contacts *contact = nil;
	if (self.searchIsActive) {
        //return [self.filteredListContent count];
        contact = [[self filteredListContent] objectAtIndex:[indexPath row]];
    }
    else{
        contact = [fetchedResultsController objectAtIndexPath:indexPath];
    }
    
	//Contacts *contact = [fetchedResultsController objectAtIndexPath:indexPath];
    cell.contact = contact;
	//cell.textLabel.text = [NSString stringWithFormat:@"%@ %@" ,contact.firstname,contact.lastname];    
}
/*

// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@"############# tableView canEditRowAtIndexPath");
    // Return NO if you do not want the specified item to be editable.
    return YES;

}
*/
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
   // NSLog(@"############# tableView canMoveRowAtIndexPath");
    return NO;
}


- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
   // NSLog(@"############# tableView moveRowAtIndexPath");
    //handle movement of UITableViewCells here
    //UITableView cells don't just swap places; one moves directly to an index, others shift by 1 position. 
}
/*
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return UITableViewCellEditingStyleDelete | UITableViewCellEditingStyleInsert; // return 3;
}

- (NSArray *)selectedIndexPaths {
    NSArray *retVal = nil;
    //object_getInstanceVariable(self, "_selectedIndexPaths", (void **)&retVal);
    return retVal;
}
*/

 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
 {
    // NSLog(@"############# tableView commitEditingStyle");
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
    
    if(self.editing==YES) return;
   // NSLog(@"############# tableView didSelectRowAtIndexPath");
/*
    if(self.searchIsActive){
        self.fetchedResultsController.delegate = nil;
    }
*/ 
    
       
    //aFetchedResultsController.delegate = nil;
	ContactDetailViewController *selectViewController = [[ContactDetailViewController alloc] initWithNibName:@"ContactDetailViewController" bundle:nil];
    
    Contacts *selectedObject = nil;

    fetchedResultsController.delegate = nil;

	if (self.searchIsActive) {
        //return [self.filteredListContent count];
        selectedObject = [[self filteredListContent] objectAtIndex:[indexPath row]];
    }
    else{
        selectedObject = [fetchedResultsController objectAtIndexPath:indexPath];
    }

    [selectViewController setContact:selectedObject];   
    //[selectViewController setEditedObject:selectedObject];
    [selectViewController setIsReadyOnly:isReadyOnly];    
    [selectViewController setHidesBottomBarWhenPushed:YES];    
	[self.navigationController pushViewController:selectViewController animated:YES];
	[selectViewController release];
    
    

}


#import "addContactViewController.h"

//#import "newContactUIViewController.h"
-(void)addNewContact
{
    //handle adding new contact here
   // NSLog(@"###### Enter addNewContact...");


  
    
   //self.fetchedResultsController.delegate = nil;
    
    addContactViewController *addViewController = [[addContactViewController alloc]  initWithNibName:@"addContactViewController" bundle:nil];	
    
    //addViewController.title = @"";
    /*
    addViewController.delegate = self;
	
	// Create a new managed object context for the new book -- set its persistent store coordinator to the same as that from the fetched results controller's context.
	NSManagedObjectContext *addingContext = [[NSManagedObjectContext alloc] init];
	self.addingManagedObjectContext = addingContext;
	[addingContext release];
	
	[addingManagedObjectContext setPersistentStoreCoordinator:[[fetchedResultsController managedObjectContext] persistentStoreCoordinator]];
    
	addViewController.contact = (Contacts *)[NSEntityDescription insertNewObjectForEntityForName:@"Contacts" inManagedObjectContext:addingContext];
    

        //[contactUISearchBar resignFirstResponder];
    //[self.view endEditing:YES];
     */
    [addViewController setHidesBottomBarWhenPushed:YES];
    [self.navigationController pushViewController:addViewController animated:YES];
        //[detailView release];
    [addViewController autorelease];
  
  
    
}



#pragma mark - Fetched results controller

- (NSFetchedResultsController *)fetchedResultsController
{
   // NSLog(@"####### fetchedResultsController");
    if (fetchedResultsController != nil) {
        return fetchedResultsController;
    }
    
    // Set up the fetched results controller.
    // Create the fetch request for the entity.
    NSFetchRequest *fetchRequest = [[[NSFetchRequest alloc] init] autorelease];
    // Edit the entity name as appropriate.
    NSEntityDescription *entity = [NSEntityDescription entityForName:@"Contacts" inManagedObjectContext:self.managedObjectContext];
    [fetchRequest setEntity:entity];
    
    
    // Set the batch size to a suitable number.
    [fetchRequest setFetchBatchSize:20];
    
    /*
     NSSortDescriptor *authorDescriptor = [[NSSortDescriptor alloc] initWithKey:@"firstname" ascending:YES];
     NSSortDescriptor *titleDescriptor = [[NSSortDescriptor alloc] initWithKey:@"lastname" ascending:YES];
     NSArray *sortDescriptors = [[NSArray alloc] initWithObjects:authorDescriptor, titleDescriptor, nil];
     */
    
    // Edit the sort key as appropriate.
    NSSortDescriptor *sortDescriptor = [[[NSSortDescriptor alloc] initWithKey:@"section_key" ascending:YES] autorelease];
    //NSArray *sortDescriptors = [NSArray arrayWithObjects:sortDescriptor, nil];
    NSArray *sortDescriptors = [[[NSArray alloc] initWithObjects:sortDescriptor,nil] autorelease];
    
       
    
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
	    /*
	     Replace this implementation with code to handle the error appropriately.
         
	     abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. 
	     */
	    NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
	    abort();
	}
    
    //[sectionKey release];
    
	// Memory management.
	//[aFetchedResultsController release];
	//[fetchRequest release];
	//[sortDescriptors release];
    
    
    return fetchedResultsController;


}    

- (void)controllerWillChangeContent:(NSFetchedResultsController *)controller
{
    //NSLog(@"############ controllerWillChangeContent");
    [self.tableView beginUpdates];
}

- (void)controller:(NSFetchedResultsController *)controller didChangeSection:(id <NSFetchedResultsSectionInfo>)sectionInfo
           atIndex:(NSUInteger)sectionIndex forChangeType:(NSFetchedResultsChangeType)type
{
   // NSLog(@"######## controller didChangeSection");    
    /*
     switch(type) {
     case NSFetchedResultsChangeInsert:
     [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
     break;
     
     case NSFetchedResultsChangeDelete:
     [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
     break;
     }
     */
    
    switch(type) {
        case NSFetchedResultsChangeInsert:
            [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex]
                          withRowAnimation:UITableViewRowAnimationFade];
            break;
        case NSFetchedResultsChangeDelete:
            [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
            
            break;
        case NSFetchedResultsChangeMove:
            break;
        case NSFetchedResultsChangeUpdate: 
            break;
        default:
            break;
    }   
    /*
    switch(type) {
            
        case NSFetchedResultsChangeInsert:
           // NSLog(@"######  didChangeSection NSFetchedResultsChangeInsert");
            if (!((sectionIndex == 0) && ([self.tableView numberOfSections] == 1)))
                [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
            break;
        case NSFetchedResultsChangeDelete:
            //NSLog(@"######  didChangeSection NSFetchedResultsChangeDelete");
            if (!((sectionIndex == 0) && ([self.tableView numberOfSections] == 1) ))
                [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationFade];
            break;
        case NSFetchedResultsChangeMove:
            break;
        case NSFetchedResultsChangeUpdate: 
            break;
        default:
            break;
    }    
*/ 
    
}



- (void)controller:(NSFetchedResultsController *)controller didChangeObject:(id)anObject
       atIndexPath:(NSIndexPath *)indexPath forChangeType:(NSFetchedResultsChangeType)type
      newIndexPath:(NSIndexPath *)newIndexPath
{
    //NSLog(@"######## controller didChangeObject");
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
            [self.tableView insertRowsAtIndexPaths:[NSArray arrayWithObject:newIndexPath] withRowAnimation:UITableViewRowAnimationFade];
            break;
        case NSFetchedResultsChangeDelete:
            [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
            break;
        case NSFetchedResultsChangeUpdate: {
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
        }
        case NSFetchedResultsChangeMove:
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
}

/*
 // Implementing the above methods to update the table view in response to individual changes may have performance implications if a large number of changes are made simultaneously. If this proves to be an issue, you can instead just implement controllerDidChangeContent: which notifies the delegate that all section and object changes have been processed. 
 
 - (void)controllerDidChangeContent:(NSFetchedResultsController *)controller
 {
 // In the simplest, most efficient, case, reload the table view.
 [self.tableView reloadData];
 }
 */





- (void)insertNewObject
{

    

        // Create a new instance of the entity managed by the fetched results controller.
        NSManagedObjectContext *context = [self.fetchedResultsController managedObjectContext];
        NSEntityDescription *entity = [[self.fetchedResultsController fetchRequest] entity];
        NSManagedObject *newManagedObject = [NSEntityDescription insertNewObjectForEntityForName:[entity name] inManagedObjectContext:context];
        
        // If appropriate, configure the new managed object.
        // Normally you should use accessor methods, but using KVC here avoids the need to add a custom class to the template.
    
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", @"aaaa"] forKey:@"firstname"];
    
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", @"bbbbb"] forKey:@"lastname"];
    [newManagedObject setValue:[NSString stringWithFormat:@"%@", @"123"] forKey:@"secureid"];
    
        
        // Save the context.
        NSError *error = nil;
        if (![context save:&error]) {
            /*
             Replace this implementation with code to handle the error appropriately.
             
             abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. 
             */
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
        }  
    
    
    
    
    
    
    
}

/*
- (IBAction)addContact {
	NSLog(@"addContact");
    

     newContactUIViewController *NewContactUIViewController = nil;
     
     NewContactUIViewController = [[newContactUIViewController alloc] initWithStyle:UITableViewStyleGrouped];
     NewContactUIViewController.delegate = self;
     
     // Create a new managed object context for the new book -- set its persistent store coordinator to the same as that from the fetched results controller's context.
     NSManagedObjectContext *addingContext = [[NSManagedObjectContext alloc] init];
     self.addingManagedObjectContext = addingContext;
     [addingContext release];
     
     [addingManagedObjectContext setPersistentStoreCoordinator:[[fetchedResultsController managedObjectContext] persistentStoreCoordinator]];
     
     //NewContactUIViewController.contact = (Contacts *)[NSEntityDescription insertNewObjectForEntityForName:@"Contacts" inManagedObjectContext:addingContext];
     
     UINavigationController *nexentModalViewController:navController animated:YES];
     
     [newContactUIViewController release];
     [navController release];
}
*/




#pragma mark UISearchBarDelegate

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
    //self.filteredListContent = nil;
   // NSLog(@"########## searchBarTextDidBeginEditing");  
    // only show the status bar's cancel button while in edit mode
    contactUISearchBar.showsCancelButton = YES;
    contactUISearchBar.autocorrectionType = UITextAutocorrectionTypeNo;
    // flush the previous search content
    
}

#pragma mark -
#pragma mark Content Filtering

- (void)filterContentForSearchText:(NSString*)searchText scope:(NSString*)scope
{
    //NSLog(@"########## filterContentForSearchText searchText"); 
    self.filteredListContent = nil;
    //    [filteredListContent removeAllObjects]; // First clear the filtered array.
    
    NSPredicate * predicate = [NSPredicate predicateWithFormat:@"firstname like[cd] %@ or lastname like[cd] %@", [NSString stringWithFormat:@"*%@*",searchText],[NSString stringWithFormat:@"*%@*",searchText]];
    self.filteredListContent = [[[self fetchedResultsController] fetchedObjects] filteredArrayUsingPredicate:predicate];

}

#pragma mark -
#pragma mark UISearchDisplayController Delegate Methods

- (BOOL)searchDisplayController:(UISearchDisplayController *)controller shouldReloadTableForSearchString:(NSString *)searchString
{
//    NSLog(@"########## searchDisplayController shouldReloadTableForSearchString");  
    [self filterContentForSearchText:searchString scope:
	 [[self.searchDisplayController.searchBar scopeButtonTitles] objectAtIndex:
	  [self.searchDisplayController.searchBar selectedScopeButtonIndex]]];
    
    return YES;
}

- (BOOL)searchDisplayController:(UISearchDisplayController *)controller shouldReloadTableForSearchScope:(NSInteger)searchOption
{
//    NSLog(@"########## searchDisplayController shouldReloadTableForSearchScope"); 
    [self filterContentForSearchText:[self.searchDisplayController.searchBar text] scope:
	 [[self.searchDisplayController.searchBar scopeButtonTitles] objectAtIndex:searchOption]];
    
    return YES;
}

- (void)searchDisplayControllerWillBeginSearch:(UISearchDisplayController *)controller {
    //NSLog(@"########## searchDisplayControllerWillBeginSearch"); 
	self.searchIsActive = YES;
    
	//[self performSelector:@selector(addWord) withObject:nil afterDelay:3.0];
}

- (void)searchDisplayControllerDidEndSearch:(UISearchDisplayController *)controller {
   // NSLog(@"########## searchDisplayControllerDidEndSearch"); 
    self.filteredListContent = nil;
	self.searchIsActive = NO;
    [self.tableView reloadData];    
//    [fetchedResultsController release];
    //[fetchedResultsController delegate];
  //  [self.tableView reloadData];
    
   // NSPredicate * predicate = [NSPredicate predicateWithFormat:@""];
   // self.filteredListContent = [[[self fetchedResultsController] fetchedObjects] filteredArrayUsingPredicate:predicate];
  //  [self.tableView reloadData];
    
    //[self.tableView beginUpdates];
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    //NSLog(@"############  titleForHeaderInSection");

    if(self.searchIsActive){
        return nil;
    }
    else{
        //NSLog(@"############  sectionname ->%@ ",[[[self.fetchedResultsController sections] objectAtIndex:section] name]);
        
        id <NSFetchedResultsSectionInfo> sectionInfo = 
        [[self.fetchedResultsController sections] objectAtIndex:section];
                //return [sectionInfo name];  
        return [sectionInfo name]; 
    
    
    }
        //return [[[fetchedResultsController sections] objectAtIndex:section] name];

    /*
    if(self.searchIsActive){
        return nil;
    }    
    
    if (!(section == 0 && [self.tableView numberOfSections] == 1)) {
        id <NSFetchedResultsSectionInfo> sectionInfo = 
        [[self.fetchedResultsController sections] objectAtIndex:section];
        //return [sectionInfo name];  
        return [[[self.fetchedResultsController sections] objectAtIndex:section] name];
    }
    return nil ;*/
}



/**
 Add controller's delegate method; informs the delegate that the add operation has completed, and indicates whether the user saved the new contact.
 */
- (void)addContactViewController:(addContactViewController *)controller didFinishWithSave:(BOOL)save {
    //NSLog(@"######didFinishWithSave");	
	if (save) {
        
		NSNotificationCenter *dnc = [NSNotificationCenter defaultCenter];
		[dnc addObserver:self selector:@selector(addControllerContextDidSave:) name:NSManagedObjectContextDidSaveNotification object:addingManagedObjectContext];
		
		NSError *error;
		if (![addingManagedObjectContext save:&error]) {
			// Update to handle the error appropriately.
			NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
			//exit(-1);  // Fail
		}
		[dnc removeObserver:self name:NSManagedObjectContextDidSaveNotification object:addingManagedObjectContext];
	}
	// Release the adding managed object context.
	self.addingManagedObjectContext = nil;
    
	// Dismiss the modal view to return to the main list
    [self dismissModalViewControllerAnimated:YES];
}


/**
 Notification from the add controller's context's save operation. This is used to update the fetched results controller's managed object context with the new book instead of performing a fetch (which would be a much more computationally expensive operation).
 */
- (void)addControllerContextDidSave:(NSNotification*)saveNotification {
	   // NSLog(@"##### addControllerContextDidSave");
	
    NSManagedObjectContext *context = [fetchedResultsController managedObjectContext];
	// Merging changes causes the fetched results controller to update its results
	[context mergeChangesFromContextDidSaveNotification:saveNotification];	
     
}


#pragma mark - SIP Registration Status

- (void)onRegistrationNew:(Registration*)registration
{
    NSLog(@">>>>> onRegistrationNew");
    
    if ([registration reason]==nil) {
		//label_sipstatus.text = @"Starting";
        [aiv startAnimating];
    }
	if ([registration code]>=200 && [registration code]<=299) {
		//label_sipstatus.text = @"Registered on server";
        [aiv stopAnimating];
        //[noTouchView setHidden:YES];
	} else if ([registration code]==0) {
		//label_sipstatus.text = [NSString stringWithFormat: @"--- %@", [registration reason]];
        [aiv startAnimating];
	} else {
		//label_sipstatus.text = [NSString stringWithFormat: @"%i %@", [registration code], [registration reason]];
        [aiv startAnimating];
	}
    
}

- (void)onRegistrationRemove:(Registration*)registration
{
    NSLog(@">>>>> onRegistrationRemove");
    [aiv stopAnimating];
    //[noTouchView setHidden:YES];
#if 0
	label_sipstatus.text = @"Unregistered";
	[self configureSIPStatus:[registration code]];
#endif
}

- (void)onRegistrationUpdate:(Registration*)registration
{
    NSLog(@">>>>> onRegistrationUpdate");
    
	if ([registration reason]==nil) {
		//label_sipstatus.text = @"Starting";
        [aiv startAnimating];
    }
	if ([registration code]>=200 && [registration code]<=299) {
		//label_sipstatus.text = @"Registered on server";
        [aiv stopAnimating];
        //[noTouchView setHidden:YES];
	} else if ([registration code]==0) {
		//label_sipstatus.text = [NSString stringWithFormat: @"--- %@", [registration reason]];
        [aiv startAnimating];
	} else {
		//label_sipstatus.text = [NSString stringWithFormat: @"%i %@", [registration code], [registration reason]];
        [aiv startAnimating];
	}
	
	//[self configureSIPStatus:[registration code]];
}

@end
