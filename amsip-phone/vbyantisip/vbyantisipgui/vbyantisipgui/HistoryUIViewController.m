#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "HistoryUIViewController.h"

#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"

#import "CellHistoryUITableView.h"
#import "HistoryEntry.h"
//#import "detailHistoryViewController.h"
#import "detailHistoryViewController.h"
#import "UIViewControllerStatus.h"
#include <amsip/am_options.h>

@implementation HistoryUIViewController


UIActivityIndicatorView *aiv;

- (void)viewWillAppear:(BOOL)animated 
{
    
    [super viewWillAppear:animated];
    
    [myHistoryList removeAllObjects];
    [myHistoryDb load_history:myHistoryList];
    [historyTable reloadData];
    //20120525 SANJI[
    NSLog(@"######## viewWillAppear start");
    //self.navigationController.navigationBar.tintColor = [UIColor blackColor];
    

    
    NSLog(@"######## viewWillAppear end");    
    //] SANJI
#if 0
    if ([gAppEngine isStarted]==NO) {
        [aiv startAnimating];
    }    
    [gAppEngine addRegistrationDelegate:self];
#endif

}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];

#if 0
    [gAppEngine removeRegistrationDelegate:self];
    //[aiv stopAnimating];
#endif
}


- (void)viewDidLoad {
    [super viewDidLoad];
    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    historyTable.backgroundView = tableBgImage;
    [tableBgImage release];   
    
    //self.title = @"Recent";
    self.navigationItem.title = NSLocalizedString(@"tabRecent",nil);
    
    UIBarButtonItem *rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemTrash target:self action:@selector(deleteAllItems)];
    
    self.navigationItem.rightBarButtonItem = rightBarButtonItem;
    
    [rightBarButtonItem release];
    
    
    // NSMutableArray *searchButtons = [[NSMutableArray alloc] initWithCapacity:2];  
  
     
    UISegmentedControl *segmentedControl = [ [UISegmentedControl alloc] initWithItems:[NSArray arrayWithObjects:NSLocalizedString(@"btnAllCall", nil), NSLocalizedString(@"btnLostCall", nil),nil]]; 
    //segmentedControl.segmentedControlStyle = 
    segmentedControl.segmentedControlStyle = UISegmentedControlStyleBar; 
    segmentedControl.selectedSegmentIndex = 0; 
    [segmentedControl addTarget:self action:@selector(segmentAction:) forControlEvents:UIControlEventValueChanged]; 
     
    //UIImageView *segBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"button-all-lost-phone.png"]];
    
    
    //NSArray *segmentControlTitles = [[NSArray arrayWithObjects:@"Books", @"PDFs", nil] retain];
    //UIImage* dividerImage = [UIImage imageNamed:@"view-control-divider.png"];
    /*
    [segmentedControl setDividerImage:[UIImage imageNamed:@"divider_selected.png"] forLeftSegmentState:UIControlStateSelected rightSegmentState:UIControlStateNormal barMetrics:UIBarMetricsDefault];
    [segmentedControl setDividerImage:[UIImage imageNamed:@"divider_normal.png"] forLeftSegmentState:UIControlStateNormal rightSegmentState:UIControlStateNormal barMetrics:UIBarMetricsDefault];
    */
    //[segmentedControl setBackgroundImage:[UIImage imageNamed:@"button-all-lost-phone.png"] forState:UIControlStateHighlighted barMetrics:UIBarMetricsDefault];
    
    //segmentedControl.backgroundView = segBgImage;
    //[segBgImage release];   
    
    UIBarButtonItem *seg = [[UIBarButtonItem alloc] initWithCustomView:segmentedControl];
    self.navigationItem.leftBarButtonItem = seg; 

    
    [segmentedControl release];
    
    myHistoryList = [[NSMutableArray arrayWithObjects:nil] retain];
    myHistoryDb = [SqliteHistoryHelper alloc];
    [myHistoryDb open_database];

#if 0
    //activity indicator
    aiv = [[[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge] autorelease];
    aiv.center = CGPointMake(160, 220);
    [self.view addSubview:aiv];
    [aiv setHidesWhenStopped:YES];
#endif
    
    return;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


+(void)_keepAtLinkTime
{
    return;
}


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
    [myHistoryList removeAllObjects];
    [myHistoryDb load_history:myHistoryList];
    [historyTable reloadData];
}

-(void)searchLost{
    [myHistoryList removeAllObjects];
    [myHistoryDb load_lost_history:myHistoryList];
    [historyTable reloadData];
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
    NSLog(@"######### indexPath:%@",indexPath);
        
 
    UIViewController *nextViewController = nil;
    nextViewController = [[detailHistoryViewController alloc] initWithNibName:@"detailHistoryViewController" bundle:nil];
    
    //HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
    //[historyTable deselectRowAtIndexPath:indexPath animated:YES];
    //detailHistoryViewController *detailViewController = [[detailHistoryViewController alloc] initWithNibName:@"detailHistoryViewController" bundle:nil];
   // [nextViewController.setHistoryEntry:historyEntry];
    NSMutableString *phonem = [[NSMutableString alloc] init];
    
    HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
    [historyTable deselectRowAtIndexPath:indexPath animated:YES];
    NSLog(@"############# run deselectRowAtIndexPath");
    if ([historyEntry.remoteuri rangeOfString : @"sip:"].location == NSNotFound)
    {
        phonem = [[historyEntry.remoteuri mutableCopy] autorelease];
        [phonem replaceOccurrencesOfString:@"-"
                                withString:@""
                                   options:NSLiteralSearch 
                                 range:NSMakeRange(0, [phonem length])];
        //[phonem initWithFormat:@"%@",phonem];
    
    }
    else{
        phonem = [[historyEntry.remoteuri mutableCopy] autorelease];
    }
    NSLog(@"########### set var value");
   
    
    ((detailHistoryViewController *)nextViewController).fnstring = [self stripSecureIDfromURL:historyEntry.remoteuri];     
    ((detailHistoryViewController *)nextViewController).secureid = [self stripSecureIDfromURL:historyEntry.remoteuri];
    ((detailHistoryViewController *)nextViewController).phonenum = phonem;
   // ((detailHistoryViewController *)nextViewController).phonem = indexPath.row;
    
    //[phonem release];
    
    //[phonem1 release];
    if (nextViewController) {
        [nextViewController setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:nextViewController animated:YES];  
  
        [nextViewController release];
    }
    


 /*   
//	HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
  
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
  */
   // return;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	return [myHistoryList count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
  CellHistoryUITableView *cell1 = (CellHistoryUITableView *)[tableView dequeueReusableCellWithIdentifier:@"historyCellViewController"];
  if ( cell1 != nil ) {
    // yes it had a cell we could reuse
    NSLog(@"reusing cell '%@' (%p) for row %d...", cell1.reuseIdentifier, cell1, indexPath.row);
  } else {
    // no cell to reuse, we have to create a new instance by loading it from the IB file
    NSString *nibName = @"historyCellViewController";
    [cellOwnerLoadHistory loadMyNibFile:nibName];
    cell1 = (CellHistoryUITableView *)cellOwnerLoadHistory.cell;
    NSLog(@"Loading cell from nib %@", nibName);
  }
  
  HistoryEntry *historyEntry = [myHistoryList objectAtIndex:indexPath.row];
    [cell1.remoteuri setText:[self stripSecureIDfromURL:historyEntry.remoteuri]];
  [cell1.duration setText:@"?"];
    
  
  int direction = [myHistoryDb findint_callinfo:historyEntry withKey:@"direction"];
  int sip_code = [myHistoryDb findint_callinfo:historyEntry withKey:@"sip_code"];
  NSString *end_date =  [[NSString alloc] initWithFormat:@"%@",[myHistoryDb findtext_callinfo:historyEntry withKey:@"end_date"]];
  NSLog(@"################   direction  = %i / sip_code = %i / end_date = %@",direction,sip_code, end_date);
  [cell1.reason setText:@"-"];
  if (direction==0 && sip_code==200) {
    [cell1.image setImage:[UIImage imageNamed:@"icon-call-out"]];//icon_arrow_right.png
  }
  else if (direction==1 && ([end_date isEqualToString:@"(null)"] || end_date==nil)){
      [cell1.image setImage:[UIImage imageNamed:@"icon-call-miss"]];//icon_arrow_left.png
  }
  else if (direction==1){
      [cell1.image setImage:[UIImage imageNamed:@"icon-call-in"]];//icon_arrow_left.png
  }
  else{
      [cell1.image setImage:[UIImage imageNamed:@"icon-call-miss"]];//icon_arrow_left.png
  }
    

    

//20120529 SANJI [ 
    NSDateFormatter *dateFormatter=[[NSDateFormatter alloc] init];
    
    [dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];  
    [dateFormatter setFormatterBehavior:NSDateFormatterBehavior10_4];
    NSString *start_date = [[NSString alloc] initWithFormat:@"%@",[myHistoryDb findtext_callinfo:historyEntry withKey:@"start_date"]];
    

    NSTimeInterval durtion = [[dateFormatter dateFromString:end_date] timeIntervalSinceDate:[dateFormatter dateFromString:start_date]];

   // NSString *date1 = @"2010-06-27 12:15:00";
   // NSString *date2 = @"2010-06-29 17:30:00";
	  
    NSLog(@"######## start_date:%@, end_date:%@,durtion=%f  ",start_date,end_date,durtion);
    
    //NSLog(@"######## date1:%@, date2:%@, durtion:%@",date1,date2,durtion);
    
    NSInteger hours=((int)durtion)/(60*60); 
    NSInteger minutes=(((int)durtion)-(hours*60*60))/60;
    NSInteger secs=((int)durtion)- (hours*60*60) - (minutes*60); 
    
    NSString *durtionString=[[NSString alloc] initWithFormat:@"%02d:%02d:%02d",hours,minutes,secs];
    
    [cell1.duration setText:durtionString];
    //[cell1.duration setText:durtionString];
    [dateFormatter release];    

    NSDateFormatter *dateFormatter2=[[NSDateFormatter alloc] init];               
    //dateWithTimeIntervalSinceReferenceDate
    [dateFormatter2 setDateFormat:@"yy-M-dd"];  
    [dateFormatter2 setFormatterBehavior:NSDateFormatterBehaviorDefault]; 
   
    //NSString *calldate = [[NSString alloc] initWithFormat:@"%@",[myHistoryDb findtext_callinfo:historyEntry withKey:@"start_date"]];
    NSDate *tmp_date = [[NSDate alloc] init];
    tmp_date = [dateFormatter2 dateFromString:start_date];
    
    //NSDate *calldate = [[NSDate alloc] initWithFormat:@"",[dateFormatter dateFromString:start_date]];
    //NSString *calldateString=[[NSString alloc] initWithFormat:@"%@",start_date];
    //NSDate *calldate_tmp = [dateFormatter2 dateFromString:start_date];//[[NSDate alloc] init];
    //NSString *calldate = [dateFormatter stringFromDate:calldate_tmp];
    
    NSLog(@"####### start_date=%@",start_date);
    
   // NSLog(@"####### calldate_tmp=%@",calldate_tmp);
     NSLog(@"####### calldate_tmp=%@",[dateFormatter2 stringFromDate:tmp_date]);
    //NSString *calldate1 = [dateFormatter dateFromString:calldate];
    [cell1.date setText:start_date];//[dateFormatter2 stringFromDate:tmp_date]
    //[calldateString release];
    //[calldate release];
    //[calldate release];
    [dateFormatter2 release];
    [durtionString release];
    [start_date release];
    [end_date release];


    
    
    
  if (sip_code>0) {
    NSString *reason = [myHistoryDb findtext_callinfo:historyEntry withKey:@"sip_reason"];
    [cell1.reason setText: [NSString stringWithFormat:@"%i %@", sip_code, reason]];
    [reason release];

      

      
  } else {    
    //NSString *start_date = [myHistoryDb findtext_callinfo:historyEntry withKey:@"start_date"];
    [cell1.duration setText:@"00:00:00"];
    //[start_date release];

  }
//] SANJI

  //cell1.name.textColor = [UIColor colorWithWhite:0.87 alpha:1.0];
  
  return cell1;
}




- (IBAction)deleteAllItems
{
  [myHistoryList removeAllObjects];
  [myHistoryDb remove_history];
  [historyTable reloadData];
}

-(IBAction)edithistorylist{}



- (void)dealloc {
    [super dealloc];
}



- (NSString*) stripSecureIDfromURL:(NSString*) targetString {
    NSString *result = [[[NSString alloc] initWithString:targetString] autorelease];
    
    NSRange strRange = [targetString rangeOfString:@"sip:"];
    //NSLog(@">>>>> targetString = %@", result);
    if (strRange.location != NSNotFound) {
        result = [result substringFromIndex:strRange.location+strRange.length];
        //NSLog(@">>>>> targetString1 = %@", result);
        strRange = [result rangeOfString:@"@"];
        if (strRange.location!=NSNotFound) {
            result = [result substringToIndex:strRange.location];
            //NSLog(@">>>>> targetString2 = %@", result);
        }
    }
    
    return result;
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
