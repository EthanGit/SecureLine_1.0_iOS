#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerHistory.h"
#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"

#import "UITableViewCellHistory.h"
#import "HistoryEntry.h"

#include <amsip/am_options.h>

@implementation UIViewControllerHistory

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

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
  
  [historyTable deselectRowAtIndexPath:indexPath animated:YES];
  
  int res;
  if ([historyEntry.remoteuri rangeOfString : @"sip:"].location == NSNotFound)
  {
    NSMutableString *phonem = [[historyEntry.remoteuri mutableCopy] autorelease];
    [phonem replaceOccurrencesOfString:@"-"
                            withString:@""
                               options:NSLiteralSearch 
                                 range:NSMakeRange(0, [phonem length])];
    
    res = [self dial:phonem];
    return;
  }
  res = [self dial:historyEntry.remoteuri];
}

- (NSInteger)tableView:(UITableView *)tableView
 numberOfRowsInSection:(NSInteger)section
{
	return [myHistoryList count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
  UITableViewCellHistory *cell1 = (UITableViewCellHistory *)[tableView dequeueReusableCellWithIdentifier:@"CellHistory"];
  if ( cell1 != nil ) {
    // yes it had a cell we could reuse
    NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);
  } else {
    // no cell to reuse, we have to create a new instance by loading it from the IB file
    NSString *nibName = @"CellHistory";
    [cellOwnerLoadHistory loadMyNibFile:nibName];
    cell1 = (UITableViewCellHistory *)cellOwnerLoadHistory.cell;
    NSLog(@"Loading cell from nib %@", nibName);
  }
  
  HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
  [cell1.remoteuri setText:historyEntry.remoteuri];
  [cell1.duration setText:@"?"];
  
  int direction = [myHistoryDb findint_callinfo:historyEntry withKey:@"direction"];
  if (direction==0) {
    [cell1.image setImage:[UIImage imageNamed:@"icon_arrow_right.png"]];
  } else {
    [cell1.image setImage:[UIImage imageNamed:@"icon_arrow_left.png"]];
  }
  int sip_code = [myHistoryDb findint_callinfo:historyEntry withKey:@"sip_code"];
  
  [cell1.reason setText:@"-"];
  if (sip_code>0) {
    NSString *reason = [myHistoryDb findtext_callinfo:historyEntry withKey:@"sip_reason"];
    [cell1.reason setText: [NSString stringWithFormat:@"%i %@", sip_code, reason]];
    [reason release];
        
    NSString *start_date = [myHistoryDb findtext_callinfo:historyEntry withKey:@"start_date"];
    [cell1.duration setText:start_date];
    [start_date release];
  } else {    
    NSString *start_date = [myHistoryDb findtext_callinfo:historyEntry withKey:@"start_date"];
    [cell1.duration setText:start_date];
    [start_date release];
  }
  
  //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
  
  return cell1;
}

- (IBAction)deleteAllItems
{
  [myHistoryList removeAllObjects];
  [myHistoryDb remove_history];
  [historyTable reloadData];
}

- (void)viewWillAppear:(BOOL)animated 
{
  [super viewWillAppear:animated];

  [myHistoryList removeAllObjects];
  [myHistoryDb load_history:myHistoryList];
  [historyTable reloadData];
  
  UIBarButtonItem *rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemTrash target:self action:@selector(deleteAllItems)];
  self.navigationItem.rightBarButtonItem = rightBarButtonItem;
  [rightBarButtonItem release];
  
}

- (void)viewDidLoad {
  [super viewDidLoad];

  myHistoryList = [[NSMutableArray arrayWithObjects:nil] retain];
  myHistoryDb = [SqliteHistoryHelper alloc];
  [myHistoryDb open_database];
  return;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [super dealloc];
}

@end
