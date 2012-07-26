#import "UIViewControllerAbook.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerDialpad.h"

#import "AppEngine.h"
#import "UITableViewCellFavorites.h"

@implementation UIViewControllerAbook

@synthesize picker = _picker;

+(void)_keepAtLinkTime
{
    return;
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

- (IBAction)contactbuttonAction:(id)sender {
  if (self.picker==nil) {
    self.picker = [[ABPeoplePickerNavigationController alloc] init];
    [self.picker setDelegate:self];
    self.picker.peoplePickerDelegate= self;
  }
	[self presentModalViewController:self.picker	animated:YES];
}

- (NSInteger)tableView:(UITableView *)tableView
		 numberOfRowsInSection:(NSInteger)section
{
	return [myFavoriteList count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView
		 cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
  UITableViewCellFavorites *cell1 = (UITableViewCellFavorites *)[tableView dequeueReusableCellWithIdentifier:@"CellFavorites"];
  if ( cell1 != nil ) {
    // yes it had a cell we could reuse
    NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);
  } else {
    // no cell to reuse, we have to create a new instance by loading it from the IB file
    NSString *nibName = @"CellFavorites";
    [cellOwnerLoadFavorites loadMyNibFile:nibName];
    cell1 = (UITableViewCellFavorites *)cellOwnerLoadFavorites.cell;
    NSLog(@"Loading cell from nib %@", nibName);
  }
  
  ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
  [cell1.lastname setText:contact.lastname];
  [cell1.firstname setText:contact.firstname];
  
  if ([contact.phone_numbers count]>0) {
    SipNumber *sip_number = [contact.phone_numbers objectAtIndex:0];
    if (sip_number)
      [cell1.type setText:sip_number.phone_type];
  } else {
    NSLog(@"error: no numbers available?");
  }
  //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
  
  return cell1;
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
  
  if ([contact.phone_numbers count]<=0)
  {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    return;
  }
  
  if ([contact.phone_numbers count]==1)
  {
    SipNumber *sip_number = [contact.phone_numbers objectAtIndex:0];
    NSString *selected_phone = sip_number.phone_number;
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (selected_phone != nil && !([selected_phone isEqualToString:@"..."])){
      int res;
      if ([selected_phone rangeOfString : @"sip:"].location == NSNotFound)
      {
        NSMutableString *phonem = [[selected_phone mutableCopy] autorelease];
        [phonem replaceOccurrencesOfString:@"-"
                                withString:@""
                                   options:NSLiteralSearch 
                                     range:NSMakeRange(0, [phonem length])];
        
        res = [self dial:phonem];
        return;
      }
      res = [self dial:selected_phone];
      return;
    }
    return;
  }
  
  UIActionSheet *action_sheet = [[UIActionSheet alloc]initWithTitle:	NSLocalizedString(@"Select Number", @"Select Number")
                                                           delegate:	self
                                                  cancelButtonTitle:	NSLocalizedString(@"Cancel", @"Cancel")
                                             destructiveButtonTitle:	nil
                                                  otherButtonTitles: nil,
                                 nil];
  for( SipNumber *number in contact.phone_numbers)  
    [action_sheet addButtonWithTitle:number.phone_number]; 
  [action_sheet showFromTabBar:self.navigationController.tabBarController.tabBar];
  [action_sheet release];
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
  
  if (editingStyle == UITableViewCellEditingStyleDelete) {
    // Delete the row from the data source
    ContactEntry *contact = [myFavoriteList objectAtIndex:indexPath.row];
    [myFavoriteList removeObjectAtIndex:indexPath.row];
    [myFavoriteContactDb remove_contact:contact];
    [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
  }   
  else if (editingStyle == UITableViewCellEditingStyleInsert) {
    // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
  }   
}

/* Implement viewDidLoad if you need to do additional setup after loading the view. */
 - (void)viewDidLoad {
	 myFavoriteList = [[NSMutableArray arrayWithObjects:nil] retain];
   
   myFavoriteContactDb = [SqliteContactHelper alloc];
   [myFavoriteContactDb open_database];
   [myFavoriteContactDb load_contacts:myFavoriteList];
	 [super viewDidLoad];
}
 

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person{
	
  if (current_contact)
  {
    [current_contact release];
    current_contact=nil;
  }
	
  ABMultiValueRef phoneNumbers = ABRecordCopyValue(person, kABPersonPhoneProperty);
	if (ABMultiValueGetCount(phoneNumbers) <= 0) 
	{
    CFRelease(phoneNumbers); 
    return YES;
  }

  current_contact = [[ContactEntry alloc]init];
  ABMultiValueRef firstname = ABRecordCopyValue(person, kABPersonFirstNameProperty);
  ABMultiValueRef lastname = ABRecordCopyValue(person, kABPersonLastNameProperty);
  
  if (firstname)
    [current_contact setFirstname: [NSString stringWithFormat:@"%@", firstname]];
  if (lastname)
    [current_contact setLastname: [NSString stringWithFormat:@"%@", lastname]];
  if (firstname)
    CFRelease(firstname);
  if (lastname)
    CFRelease(lastname);
	
  CFIndex i; 
  for (i=0; i < ABMultiValueGetCount(phoneNumbers); i++) 
  { 
    CFStringRef label = ABMultiValueCopyLabelAtIndex(phoneNumbers, i); 
    NSString *localized_label = (NSString *)ABAddressBookCopyLocalizedLabel(label);
    NSString *number = (NSString *)ABMultiValueCopyValueAtIndex(phoneNumbers, i); 
    NSMutableString *phonem = [[number mutableCopy] autorelease];
    [phonem replaceOccurrencesOfString:@" "
                            withString:@""
                               options:NSLiteralSearch 
                                 range:NSMakeRange(0, [phonem length])];
    [phonem replaceOccurrencesOfString:@"("
                            withString:@"-"
                               options:NSLiteralSearch 
                                 range:NSMakeRange(0, [phonem length])];
    [phonem replaceOccurrencesOfString:@")"
                            withString:@"-"
                               options:NSLiteralSearch 
                                 range:NSMakeRange(0, [phonem length])];
    SipNumber *sip_number = [SipNumber alloc];
    [sip_number setPhone_type:localized_label];
    [sip_number setPhone_number:[phonem stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    [current_contact.phone_numbers insertObject:sip_number atIndex:0];

    [number release];
    CFRelease(label);
    [localized_label release];
  }
  
	CFRelease(phoneNumbers); 
  
	return	YES;
}


- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifier{
	
	[peoplePicker dismissModalViewControllerAnimated:YES];
	[peoplePicker release];
	self.picker = nil;
  
	NSString *contactName = [NSString stringWithFormat:@"%@ %@",  
								  ABRecordCopyValue(person, kABPersonFirstNameProperty),  
								  ABRecordCopyValue(person, kABPersonLastNameProperty) 
	];
	[contact_name setText:contactName];
  
	ABMultiValueRef phoneProperty = ABRecordCopyValue(person,property);
	NSString *number = (NSString *)ABMultiValueCopyValueAtIndex(phoneProperty,identifier);

  NSMutableString *phonem = [[number mutableCopy] autorelease];
  [phonem replaceOccurrencesOfString:@"("
                          withString:@""
                             options:NSLiteralSearch 
                               range:NSMakeRange(0, [phonem length])];
  [phonem replaceOccurrencesOfString:@")"
                          withString:@""
                             options:NSLiteralSearch 
                               range:NSMakeRange(0, [phonem length])];
  [phonem replaceOccurrencesOfString:@" "
                          withString:@""
                             options:NSLiteralSearch 
                               range:NSMakeRange(0, [phonem length])];
  [phonem replaceOccurrencesOfString:@"-"
                          withString:@""
                             options:NSLiteralSearch 
                               range:NSMakeRange(0, [phonem length])];

  
	if (phonem != nil){
    [self dial:phonem];
    return NO;
  }
   
  CFRelease(phoneProperty);
  return NO;
}


#pragma mark - UIActionSheetDelegate

/*
 a differer le message selon la categorie, si c'est un deal ou un bon plan(ticket pour un spectacle)
 */

- (void)actionSheet:(UIActionSheet *)modalView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  NSIndexPath *ipath = [phonelist indexPathForSelectedRow];
  if (ipath==nil)
    return;
  [phonelist deselectRowAtIndexPath:ipath animated:YES];
  int row = ipath.row;
  if (buttonIndex<1)
    return;

  ContactEntry *contact = [myFavoriteList objectAtIndex:row];
  SipNumber *sip_number = [contact.phone_numbers objectAtIndex:buttonIndex-1];
  NSString *selected_phone = sip_number.phone_number;
  
  if (selected_phone != nil && !([selected_phone isEqualToString:@"..."])){
    int res;
    if ([selected_phone rangeOfString : @"sip:"].location == NSNotFound)
    {
      NSMutableString *phonem = [[selected_phone mutableCopy] autorelease];
      [phonem replaceOccurrencesOfString:@"-"
                              withString:@""
                                 options:NSLiteralSearch 
                                   range:NSMakeRange(0, [phonem length])];
      
      res = [self dial:phonem];
      return;
    }
    res = [self dial:selected_phone];
    return;
  }
    
}

-(IBAction)addInContactList:(id)sender{

	[self.picker dismissModalViewControllerAnimated:YES];
	[self.picker release];
	self.picker = nil;
  
  if (current_contact==nil)
    return;

  if ([current_contact.phone_numbers count]<=0)
  {
    [current_contact release];
    current_contact = nil;
    return;
  }
  
  [myFavoriteList addObject:current_contact];
  [myFavoriteContactDb insert_contact:current_contact];
  [current_contact release];
  current_contact=nil;
  [phonelist reloadData];
}

-(IBAction)CancelPicker:(id)sender{
	[self.picker dismissModalViewControllerAnimated:YES];
	[self.picker release];
	self.picker = nil;
}

#pragma mark - UINavigationControllerDelegate

// Called when the navigation controller shows a new top view controller via a push, pop or setting of the view controller stack.
- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated{
  
  //set up the ABPeoplePicker controls here to get rid of he forced cacnel button on the right hand side but you also then have to 
  // the other views it pcuhes on to ensure they have to correct buttons shown at the correct time.
  
  if([navigationController isKindOfClass:[ABPeoplePickerNavigationController class]] 
     && [viewController isKindOfClass:[ABPersonViewController class]]){
    navigationController.topViewController.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(addInContactList:)];
    //navigationController.topViewController.navigationItem.leftBarButtonItem = nil;
  }
  else if([navigationController isKindOfClass:[ABPeoplePickerNavigationController class]]){
    navigationController.topViewController.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(CancelPicker:)];

  }
}


- (void)viewWillAppear:(BOOL)animated 
{
  [super viewWillAppear:animated];
	
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker{
	
	[peoplePicker dismissModalViewControllerAnimated:YES];
	[peoplePicker release];
	self.picker = nil;
  
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}

- (void)dealloc {
  [super dealloc];
  if (current_contact)
  {
    [current_contact release];
    current_contact=nil;
  }
}

@end
