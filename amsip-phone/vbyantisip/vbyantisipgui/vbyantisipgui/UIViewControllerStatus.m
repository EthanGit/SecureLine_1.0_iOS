#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerStatus.h"

#import "vbyantisipAppDelegate.h"
#import "NetworkTracking.h"

@implementation UIViewControllerStatus

+(void)_keepAtLinkTime
{
    return;
}

- (void) configureSIPStatus:(int) statusCode
{
	label_identity.text = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	if (statusCode>=200 && statusCode<=299)
		remoteSIPIcon.image = [UIImage imageNamed: @"WWAN5.png"];
	else
		remoteSIPIcon.image = [UIImage imageNamed: @"stop-32.png"];
}

- (void)onNetworkTrackingUpdate:(NetworkTracking *)networkTracking
{
	NetworkStatus netStatus = [networkTracking currentReachabilityStatus];
	BOOL connectionRequired= [networkTracking connectionRequired];
	NSString* statusString= @"";
	switch (netStatus)
	{
		case NotReachable:
		{
			statusString = @"No network found";
			internetConnectionIcon.image = [UIImage imageNamed: @"Airport-red.png"] ;
			//Minor interface detail- connectionRequired may return yes, even when the host is unreachable.  We cover that up here...
			connectionRequired= NO;
			label_summary.text= @"no internet access.";			
			break;
		}
			
		case ReachableViaWWAN:
		{
			statusString = @"Reachable Using 3G";
			internetConnectionIcon.image = [UIImage imageNamed: @"Airport3G.png"];
			
			label_summary.text= @"3G network is available.";
			if (connectionRequired==NO)
			{
			}
			break;
		}
		case ReachableViaWiFi:
		{
			statusString= @"Reachable Using WiFi";
			internetConnectionIcon.image = [UIImage imageNamed: @"Airport.png"];
			label_summary.text= @"Wifi network is available.";
			if (connectionRequired==NO)
			{
			}
			break;
		}
	}
	if(connectionRequired)
	{
		statusString= [NSString stringWithFormat: @"%@, Connection Required", statusString];
	}
	label_networkstatus.text= statusString;
}

- (void)onRegistrationNew:(Registration*)registration
{
	if ([registration reason]==nil)
		label_sipstatus.text = @"Starting";
	else if ([registration code]>=200 && [registration code]<=299)
		label_sipstatus.text = @"Registered on server";
	else if ([registration code]==0)
		label_sipstatus.text = [NSString stringWithFormat: @"--- %@", [registration reason]];
	else
		label_sipstatus.text = [NSString stringWithFormat: @"%i %@", [registration code], [registration reason]];
	
	[self configureSIPStatus:[registration code]];
}

- (void)onRegistrationRemove:(Registration*)registration
{
	label_sipstatus.text = @"Unregistered";
	[self configureSIPStatus:[registration code]];
}

- (void)onRegistrationUpdate:(Registration*)registration
{
	if ([registration reason]==nil)
		label_sipstatus.text = @"Starting";
	if ([registration code]>=200 && [registration code]<=299)
		label_sipstatus.text = @"Registered on server";
	else if ([registration code]==0)
		label_sipstatus.text = [NSString stringWithFormat: @"--- %@", [registration reason]];
	else
		label_sipstatus.text = [NSString stringWithFormat: @"%i %@", [registration code], [registration reason]];
	
	
	[self configureSIPStatus:[registration code]];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	
	NetworkTracking *gNetworkTracking = [NetworkTracking getInstance];
	[gNetworkTracking removeNetworkTrackingDelegate:self];
	[gAppEngine removeRegistrationDelegate:self];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	label_identity.text = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	
	NetworkTracking *gNetworkTracking = [NetworkTracking getInstance];
	[gNetworkTracking addNetworkTrackingDelegate:self];
	[self onNetworkTrackingUpdate:gNetworkTracking];
	
	[gAppEngine addRegistrationDelegate:self];
}



- (void)viewDidLoad {
	[super viewDidLoad];
}


@end
