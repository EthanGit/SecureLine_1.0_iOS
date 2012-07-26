//
//  contactUIViewController.m
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//
#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

//#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"

#import "ContactEntry.h"
//#import "detailHistoryViewController.h"

#import "contactUIViewController.h"
//#import "UITableViewCellFavorites.h"
#import "CellContactUITableView.h"
#import "detailContactViewController.h"
#include <amsip/am_options.h>

@implementation contactUIViewController

@synthesize contactUISearchBar;
@synthesize contactUITableView;
@synthesize isReadyOnly;
//@synthesize sections;
//@synthesize dataSource;



- (void)viewWillAppear:(BOOL)animated 
{
    
    [super viewWillAppear:animated];

    NSLog(@"########## viewWillAppear");   
#if 1
    [myFavoriteList removeAllObjects];
    //[myContactDb load_contacts:myFavoriteList];
/*    NSMutableArray *array = [myContactDb load_data];
    for ( NSMutableDictionary* dict in array) {
        [dataSource addObject:dict];
    }
    [myFavoriteList addObjectsFromArray:dataSource];
*/
    NSMutableArray *array = [myContactDb load_data];
    for ( NSMutableDictionary* dict in array) {
        [myFavoriteList addObject:dict];
    }
    
    
    //[myFavoriteList addObjectsFromArray:dataSource];//on launch it should display all the records     
    [contactUITableView reloadData];
#endif
      
    NSLog(@"######## viewWillAppear end, %d", [myFavoriteList count]);    

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
    NSLog(@"########## viewDidLoad");
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    contactUITableView.backgroundView = tableBgImage;
    [tableBgImage release];   

    
    //UIColor *background = [[UIColor alloc] initWithCGColor:[[UIColor lightGrayColor] CGColor]];
       // UIImage *background = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"search-bar.png"]];
    //UIColor *backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"search-bar.png"]];
    UIColor *background = [UIColor lightGrayColor]; //[[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"search-bar.png"]];
    
     contactUISearchBar.backgroundColor = background;
    [background release];
    
    
    // contactUITableView.delegate = self;
    // contactUITableView.dataSource = self;
    dataSource = [[NSMutableArray alloc]init]; 
    
    /*
    for(char c = 'A';c<='Z';c++)
        [dataSource addObject:[NSString stringWithFormat:@"%cName",c]];
    
    
    myFavoriteList = [[NSMutableArray arrayWithObjects:nil, nil] retain];
    */
    //searchedData = [[NSMutableArray alloc]init];
    //tableData = [[NSMutableArray alloc]init];


    // Do any additional setup after loading the view from its nib.
    //dataSource = [[NSMutableArray arrayWithObjects:nil] retain];
    dataSource = [[NSMutableArray arrayWithObjects:nil, nil] retain];
    myFavoriteList = [[NSMutableArray arrayWithObjects:nil, nil] retain];
    
    myContactDb = [SqliteContactHelper alloc];
    [myContactDb open_database];
    /*[myContactDb load_contacts:dataSource];    
     */

    NSMutableArray *array = [myContactDb load_data];
 
    for ( NSMutableDictionary* dict in array) {
        [dataSource addObject:dict];
    }
    
    /*
    NSMutableDictionary *sections = [[NSMutableDictionary alloc] init];
    BOOL found;
    
    // Loop through the books and create our keys
    for (NSDictionary *dict in array)
    {
        NSString *c = [[dict objectForKey:@"firstname"] substringToIndex:1];
        
        found = NO;
        
        for (NSString *str in [self->sections allKeys])
        {
            if ([str isEqualToString:c])
            {
                found = YES;
            }
        }
        
        if (!found)
        {
            [self->sections setValue:[[NSMutableArray alloc] init] forKey:c];
        }
    }    

    // Sort each section array
    for (NSString *key in [self->sections allKeys])
    {
        [[self->sections objectForKey:key] sortUsingDescriptors:[NSArray arrayWithObject:[NSSortDescriptor sortDescriptorWithKey:@"firstname" ascending:YES]]];
    }
    */
    [myFavoriteList addObjectsFromArray:dataSource];//on launch it should display all the records    

    if (isReadyOnly == NO) {
        UIBarButtonItem *addButton = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(addNewContact)] autorelease];
        self.navigationItem.rightBarButtonItem = addButton;
    }
    self.navigationItem.title = @"Contacts";

    //UIBarButtonItem *editButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemEdit target:self action:@selector(editContactList)];
    //self.navigationItem.leftBarButtonItem = self.editButtonItem;;
    
    //[addButton release];
    //[editButton release];
    //[dataSource release];
    //[searchedData release];

    
}

- (void)viewDidUnload
{
    NSLog(@"####### viewDidUnload");
    [contactUISearchBar resignFirstResponder];
    [self setContactUISearchBar:nil];
    [self setContactUITableView:nil];
    [dataSource release];
    [self setIsReadyOnly:NO];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
/*
-(void) setEditing:(BOOL)editing animated:(BOOL)animated  
{  
    NSLog(@"########### setEditing editing animated");
    if(editing){ NSLog(@"####### editing");}
    else{ NSLog(@"######### done");}
    [super setEditing:editing animated:animated];   
    //self.navigationItem.leftBarButtonItem.enabled = !editing;     
    //self.navigationItem.leftBarButtonItem = ;
    [self.navigationItem.rightBarButtonItem setTitle:@"Done"];
}*/

-(void) setEditing:(BOOL)editing animated:(BOOL)animated  
{  
    [super setEditing:editing animated:animated];  
    self.navigationItem.leftBarButtonItem.enabled = !editing;  
    //[contactUITableView beginUpdates];  
    NSUInteger count = [myFavoriteList count];  
    NSArray *groupInsertIndexPath = [NSArray arrayWithObject:[NSIndexPath indexPathForRow:count inSection:0]];  
    
    // Add or remove the Add row as appropriate.  
    UITableViewRowAnimation animationStyle = UITableViewRowAnimationNone;  
    if (editing) {  
        self.navigationItem.rightBarButtonItem.title = @"Done";  
        if (animated) {  
            animationStyle = UITableViewRowAnimationFade;  
        }  
        [contactUITableView insertRowsAtIndexPaths:groupInsertIndexPath withRowAnimation:animationStyle];  
    }  
    else {        
        
       //        [modifyGroupIDArray removeAllObjects];  
        self.navigationItem.rightBarButtonItem.title = @"";  
        [contactUITableView deleteRowsAtIndexPaths:groupInsertIndexPath withRowAnimation:UITableViewRowAnimationFade];  
    }  
    //[contactUITableView endUpdates];      
}  


#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
NSLog(@"############ numberOfSectionsInTableView");
    // Return the number of sections.
    //return [[fetchedResultsController sections] count];
    return 1;
    //return [[self->sections allKeys] count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    NSLog(@"############# tableView numberOfRowsInSection");
    NSLog(@"###### count = %i",[myFavoriteList count]);
    //return [[self->sections valueForKey:[[[self->sections allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)] objectAtIndex:section]] count];
    // Return the number of rows in the section.
    return [myFavoriteList count];
}
/*
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    NSLog(@"############ sectionIndexTitlesForTableView");
    return [[self->sections allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
}
 */
/*

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath 
{  
   // NSLog(@"############# tableView cellForRowAtIndexPath indexPath:%i",indexPath.row);
    static NSString *CellIdentifier = @"CellContactUITableView";
 
    
        CellContactUITableView *cell1 = (CellContactUITableView *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];


        if ( cell1 != nil ) {
            // yes it had a cell we could reuse
            NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);
        } else {
            // no cell to reuse, we have to create a new instance by loading it from the IB file
            NSString *nibName = CellIdentifier;
            [cellOwnerLoadContact loadMyNibFile:nibName];
            cell1 = (CellContactUITableView *)cellOwnerLoadContact.cell;
            NSLog(@"Loading cell from nib %@", nibName);
        }
        
        //ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
       // [cell1.lastname setText:contact.lastname];
        //[cell1.firstname setText:contact.firstname];
    NSLog(@"########## run end %@",cell1.firstname);
        /*
        if ([contact.phone_numbers count]>0) {
            SipNumber *sip_number = [contact.phone_numbers objectAtIndex:0];
            if (sip_number)
                [cell1.type setText:sip_number.phone_type];
        } else {
            NSLog(@"error: no numbers available?");
        }
         */
        //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
        
   //     return cell1;
    
//}

//設定分類開頭標題
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section{
    NSLog(@"############ titleForHeaderInSection ");
    return [[[self->sections allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)] objectAtIndex:section];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath 
{ 
    NSLog(@"############ tableView cellForRowAtIndexPath");
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
    /*
    if(!editing){
        cell1.accessoryType=UITableViewCellAccessoryDetailDisclosureButton; 
    }
    else{
        cell1.accessoryType = UITableViewCellAccessoryCheckmark;
    }*/

    //ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
    //[cell1.lastname setText:[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"lastname"];
    //[cell1.firstname setText:contact.firstname];
    //NSDictionary *book = [[self->sections valueForKey:[[[self->sections allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)] objectAtIndex:indexPath.section]] objectAtIndex:indexPath.row];
    
    //cell.textLabel.text = [book objectForKey:@"title"];
    //cell.detailTextLabel.text = [book objectForKey:@"description"];
    
    
    cell1.lastname.text= [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"lastname"];//[book objectForKey:@"lastname"];
    cell1.firstname.text = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"firstname"];//[book objectForKey:@"firstname"];
    //cell1.actionButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self];
    
    //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
    return cell1;
    
}

/*
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"CellContact";
    
    CellContactUITableView *cell = (CellContactUITableView *)[tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if (cell == nil) {
        NSArray *topLevelObjects = [[NSBundle mainBundle] loadNibNamed:@"CellContactUITableView" owner:nil options:nil];
        cell = [topLevelObjects objectAtIndex:0];
    }
    
    //ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
    //[cell.lastname setText:contact.lastname];
    //[cell.firstname setText:contact.firstname];
    
    
    // Configure the cell...
    
    return cell;
}
 */



// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@"############# tableView canEditRowAtIndexPath");
    // Return NO if you do not want the specified item to be editable.
    return YES;
    /*
    if (indexPath.section == 0) { return YES;}
    else {  return NO; }    
     */
}

- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@"############# tableView canMoveRowAtIndexPath");
    return YES;
}


- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    NSLog(@"############# tableView moveRowAtIndexPath");
    //handle movement of UITableViewCells here
    //UITableView cells don't just swap places; one moves directly to an index, others shift by 1 position. 
}

 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
 {
     NSLog(@"############# tableView commitEditingStyle");
 //if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
     
     //[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
 //}   
     if (editingStyle == UITableViewCellEditingStyleDelete) { 
         [myFavoriteList removeObjectAtIndex:indexPath.row]; 
         // Delete the row from the data source. 
         [contactUITableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade]; 
         
     }    
     else if (editingStyle == UITableViewCellEditingStyleInsert) { 
         // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view. 
     }         
   
 }
 
- (UITableViewCellEditingStyle) tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
     NSLog(@"############# tableView editingStyleForRowAtIndexPath");
   // return UITableViewCellEditingStyleNone; 
     return UITableViewCellEditingStyleDelete | UITableViewCellEditingStyleInsert; 
    //return UITableViewCellEditingStyleDelete;
}


#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSLog(@"############# tableView didSelectRowAtIndexPath");
    [contactUISearchBar resignFirstResponder];
    [contactUISearchBar endEditing:YES];
    [self.view endEditing:YES];
    /*
    if (self.navigationItem.rightBarButtonItem.title== @"Done") { 
        [deleteDic setObject:indexPath forKey:[contactUITableView objectAtIndex:indexPath.row]]; 
        
    } 
    else { 
        
    } */    

    
    UIViewController *nextViewController = nil;
    nextViewController = [[detailContactViewController alloc] initWithNibName:@"detailContactViewController" bundle:nil];
    
    //HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
    //[historyTable deselectRowAtIndexPath:indexPath animated:YES];
    //detailHistoryViewController *detailViewController = [[detailHistoryViewController alloc] initWithNibName:@"detailHistoryViewController" bundle:nil];
    // [nextViewController.setHistoryEntry:historyEntry];
    //NSMutableString *phonem = [[NSMutableString alloc] init];
    
    //ContactEntry *contactEntry = [myFavoriteList objectAtIndex:indexPath.row];
    [contactUITableView deselectRowAtIndexPath:indexPath animated:YES];
    NSLog(@"############# run deselectRowAtIndexPath");

    NSLog(@"########### set var value");
    
    ((detailContactViewController *)nextViewController).fnstring = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"firstname"];//contactEntry.firstname;     
    ((detailContactViewController *)nextViewController).lnstring = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"lastname"];;//contactEntry.lastname;
    ((detailContactViewController *)nextViewController).snumber = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"secureid"];//contactEntry.phone_string;
    ((detailContactViewController *)nextViewController).cstring = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"company"];//contactEntry.company; 
    ((detailContactViewController *)nextViewController).ostring = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"other"];//contactEntry.other;

    ((detailContactViewController *)nextViewController).cid = [[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"cid"];
    // ((detailHistoryViewController *)nextViewController).phonem = indexPath.row;
    NSLog(@"#############  end");
    
    NSLog(@"%@ - %@ - %@ - %@ - %@ ",[[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"firstname"],[[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"lastname"],[[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"secureid"],[[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"company"],[[myFavoriteList objectAtIndex:indexPath.row] objectForKey:@"other"]);
    //[phonem release];
    
    //[phonem1 release];
    if (nextViewController) {
        detailContactViewController *tmpVC = (detailContactViewController*) nextViewController;
        [tmpVC setIsReadyOnly:isReadyOnly];
        [nextViewController setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:nextViewController animated:YES];  
        
        [nextViewController release];
    }
    
}





#pragma mark UISearchBarDelegate

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
         NSLog(@"########## searchBarTextDidBeginEditing");  
    // only show the status bar's cancel button while in edit mode
    contactUISearchBar.showsCancelButton = YES;
    contactUISearchBar.autocorrectionType = UITextAutocorrectionTypeNo;
    // flush the previous search content
    //[myFavoriteList removeAllObjects];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
    contactUISearchBar.showsCancelButton = NO;
}

//In the first method, we are only changing the appearance of search bar when a user taps on search bar. Refer the first two images to find the difference. The main code will be written now:

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{

         NSLog(@"########## searchBar textDidChange");      
/*
    [myFavoriteList removeAllObjects];// remove all data that belongs to previous search
    if([searchText isEqualToString:@""]||searchText==nil){
        [contactUITableView reloadData];
        return;
    }
    NSInteger counter = 0;
    for(NSString *firstname in dataSource)
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
        NSRange r = [firstname rangeOfString:searchText];
        if(r.location != NSNotFound)
            [myFavoriteList addObject:firstname];
        counter++;
        [pool release];
    }
    [contactUITableView reloadData];
  */
    [myFavoriteList removeAllObjects];
    if([searchText isEqualToString:@""]||searchText==nil){
        [contactUITableView reloadData];
        return;
    }
    
    //NSString* keyword= searchBar.text;
    NSMutableArray *array = [myContactDb load_keyword:searchText];
    for ( NSMutableDictionary* dict in array) {
        [myFavoriteList addObject:dict];
    }
    
    [contactUITableView reloadData];    
    
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
    
    NSLog(@"######### searchBarCancelButtonClicked");
    /*
    [myFavoriteList removeAllObjects];    
    NSMutableArray *array = [myContactDb load_data];
    for ( NSMutableDictionary* dict in array) {
        [dataSource addObject:dict];
    }
    
    
    [myFavoriteList addObjectsFromArray:dataSource];//on launch it should display all the records    
    [contactUISearchBar resignFirstResponder];
    contactUISearchBar.text = @"";
    */
    /*
    [myFavoriteList removeAllObjects];
    NSMutableArray *array = [myContactDb load_data];
    for ( NSMutableDictionary* dict in array) {
        [myFavoriteList addObject:dict];
    }
    
    
    //[myFavoriteList addObjectsFromArray:dataSource];//on launch it should display all the records     
    [contactUITableView reloadData];  */  
    // if a valid search was entered but the user wanted to cancel, bring back the main list content

    [myFavoriteList removeAllObjects];
    [myFavoriteList addObjectsFromArray:dataSource];
    @try{
        [contactUITableView reloadData];
    }
    @catch(NSException *e){
        
    }
    [contactUISearchBar resignFirstResponder];
    contactUISearchBar.text = @"";
    
}

// called when Search (in our case "Done") button pressed
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
    NSLog(@"############# searchBarSearchButtonClicked");
    [myFavoriteList removeAllObjects];
    NSString* keyword= searchBar.text;
    NSMutableArray *array = [myContactDb load_keyword:keyword];
    for ( NSMutableDictionary* dict in array) {
        [myFavoriteList addObject:dict];
    }
    
    [contactUISearchBar resignFirstResponder];
    [contactUITableView reloadData];
}



#import "newContactUIViewController.h"

-(void) addNewContact
{
    //handle adding new contact here
    NSLog(@"###### Enter addNewContact...");
    
    UIViewController *newContactView=nil;
    
    newContactView = [[newContactUIViewController alloc] initWithNibName:@"newContactUIViewController" bundle:nil];
    newContactView.title = @"";
            
    if (newContactView!=nil) {
        //[contactUISearchBar resignFirstResponder];
        [self.view endEditing:YES];
        [newContactView setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:newContactView animated:YES];
        //[detailView release];
        [newContactView autorelease];
    }
    /*
    
    UIViewController *newContactView = [[newContactUIViewController alloc] initWithNibName:@"newContactUIViewController" bundle:nil];
    //UINavigationController *loginNavView = [[UINavigationController alloc] initWithRootViewController:newContactView];
    
    //[self.navigationBar setBarStyle:UIBarStyleBlack];
    [self.tabBarController presentModalViewController:newContactView animated:YES];
    [newContactView release];
    //[loginNavView release];    */
}

-(void)closesearchtext{
    NSLog(@"###### closesearchtext");
    [contactUISearchBar resignFirstResponder];
}
@end
