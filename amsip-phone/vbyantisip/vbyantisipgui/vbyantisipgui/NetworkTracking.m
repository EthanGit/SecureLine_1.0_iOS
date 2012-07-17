//
//  NetworkTracking.m
//  iamsip
//
//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <CoreFoundation/CoreFoundation.h>

#import "NetworkTracking.h"
#import "MultiCastDelegate.h"
#import "NetworkTrackingDelegate.h"

#define kShouldPrintReachabilityFlags 1

static void PrintReachabilityFlags(SCNetworkReachabilityFlags    flags, const char* comment)
{
#if kShouldPrintReachabilityFlags
	
    NSLog(@"NetworkTracking Flag Status: %c%c %c%c%c%c%c%c%c %s\n",
		  (flags & kSCNetworkReachabilityFlagsIsWWAN)				  ? 'W' : '-',
		  (flags & kSCNetworkReachabilityFlagsReachable)            ? 'R' : '-',
		  
		  (flags & kSCNetworkReachabilityFlagsTransientConnection)  ? 't' : '-',
		  (flags & kSCNetworkReachabilityFlagsConnectionRequired)   ? 'c' : '-',
		  (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)  ? 'C' : '-',
		  (flags & kSCNetworkReachabilityFlagsInterventionRequired) ? 'i' : '-',
		  (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)   ? 'D' : '-',
		  (flags & kSCNetworkReachabilityFlagsIsLocalAddress)       ? 'l' : '-',
		  (flags & kSCNetworkReachabilityFlagsIsDirect)             ? 'd' : '-',
		  comment
		  );
#endif
}

static NetworkTracking *gNetworkTracking=NULL;

@implementation NetworkTracking

@synthesize networkTrackingDelegates;

+ (NetworkTracking*) getInstance {
	if (gNetworkTracking)
		return gNetworkTracking;
	gNetworkTracking = [NetworkTracking reachabilityForInternetConnection];
	return gNetworkTracking;
}

- (id) init {
    self = [super init];
    if (self != nil) {
		started=NO;
		networkTrackingDelegates = [[MulticastDelegate alloc] init];
	}
    return self;
}

static void ReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info)
{
#pragma unused (target, flags)
	NSCAssert(info != NULL, @"info was NULL in ReachabilityCallback");
	NSCAssert([(NSObject*) info isKindOfClass: [NetworkTracking class]], @"info was wrong class in ReachabilityCallback");
	
	//We're on the main RunLoop, so an NSAutoreleasePool is not necessary, but is added defensively
	// in case someon uses the Reachablity object in a different thread.
	NSAutoreleasePool* myPool = [[NSAutoreleasePool alloc] init];
	
	NetworkTracking* noteObject = (NetworkTracking*) info;
	// Post a notification to notify the client that the network reachability changed.
	//OLD CALLBACK [[NSNotificationCenter defaultCenter] postNotificationName: kReachabilityChangedNotification object: noteObject];
	
	[[noteObject networkTrackingDelegates] onNetworkTrackingUpdate:noteObject];
	
	[myPool release];
}

- (BOOL) startNotifer
{
	if (started==YES)
		return YES;
	
	SCNetworkReachabilityContext context = {0, self, NULL, NULL, NULL};
	if(!SCNetworkReachabilitySetCallback(reachabilityRef, ReachabilityCallback, &context))
	{
		NSLog(@"NetworkTracking failed to initialize");
		return NO;
	}
	if(!SCNetworkReachabilityScheduleWithRunLoop(reachabilityRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode))
	{
		NSLog(@"NetworkTracking failed to schedule");
		return NO;
	}
	started=YES;
	return YES;
}

- (void) dealloc
{
	if(reachabilityRef!= NULL)
	{
		SCNetworkReachabilityUnscheduleFromRunLoop(reachabilityRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	}
	if(reachabilityRef!= NULL)
	{
		CFRelease(reachabilityRef);
	}
	[super dealloc];
}

+ (NetworkTracking*) reachabilityWithHostName: (NSString*) hostName;
{
	NetworkTracking* retVal = NULL;
	SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithName(NULL, [hostName UTF8String]);
	if(reachability!= NULL)
	{
		retVal= [[[self alloc] init] autorelease];
		if(retVal!= NULL)
		{
			retVal->reachabilityRef = reachability;
			retVal->localWiFiRef = NO;
		}
	}
	return retVal;
}

+ (NetworkTracking*) reachabilityWithAddress: (const struct sockaddr_in*) hostAddress;
{
	SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)hostAddress);
	NetworkTracking* retVal = NULL;
	if(reachability!= NULL)
	{
		retVal= [[[self alloc] init] autorelease];
		if(retVal!= NULL)
		{
			retVal->reachabilityRef = reachability;
			retVal->localWiFiRef = NO;
		}
	}
	return retVal;
}

+ (NetworkTracking*) reachabilityForInternetConnection;
{
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;
	return [self reachabilityWithAddress: &zeroAddress];
}

+ (NetworkTracking*) reachabilityForLocalWiFi;
{
	[super init];
	struct sockaddr_in localWifiAddress;
	bzero(&localWifiAddress, sizeof(localWifiAddress));
	localWifiAddress.sin_len = sizeof(localWifiAddress);
	localWifiAddress.sin_family = AF_INET;
	// IN_LINKLOCALNETNUM is defined in <netinet/in.h> as 169.254.0.0
	localWifiAddress.sin_addr.s_addr = htonl(IN_LINKLOCALNETNUM);
	NetworkTracking* retVal = [self reachabilityWithAddress: &localWifiAddress];
	if(retVal!= NULL)
	{
		retVal->localWiFiRef = YES;
	}
	return retVal;
}

#pragma mark Network Flag Handling

- (NetworkStatus) localWiFiStatusForFlags: (SCNetworkReachabilityFlags) flags
{
	PrintReachabilityFlags(flags, "localWiFiStatusForFlags");
	
	BOOL retVal = NotReachable;
	if((flags & kSCNetworkReachabilityFlagsReachable) && (flags & kSCNetworkReachabilityFlagsIsDirect))
	{
		retVal = ReachableViaWiFi;	
	}
	return retVal;
}

- (NetworkStatus) networkStatusForFlags: (SCNetworkReachabilityFlags) flags
{
	PrintReachabilityFlags(flags, "networkStatusForFlags");
	if ((flags & kSCNetworkReachabilityFlagsReachable) == 0)
	{
		// if target host is not reachable
		return NotReachable;
	}
	
	BOOL retVal = NotReachable;
	
	if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0)
	{
		// if target host is reachable and no connection is required
		//  then we'll assume (for now) that your on Wi-Fi
		retVal = ReachableViaWiFi;
	}
	
	
	if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
		 (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0))
	{
		// ... and the connection is on-demand (or on-traffic) if the
		//     calling application is using the CFSocketStream or higher APIs
		
		if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0)
		{
			// ... and no [user] intervention is needed
			retVal = ReachableViaWiFi;
		}
	}
	
	if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN)
	{
		// ... but WWAN connections are OK if the calling application
		//     is using the CFNetwork (CFSocketStream?) APIs.
		retVal = ReachableViaWWAN;
	}
	return retVal;
}

- (BOOL) connectionRequired;
{
	NSAssert(reachabilityRef != NULL, @"connectionRequired called with NULL reachabilityRef");
	SCNetworkReachabilityFlags flags;
	if (SCNetworkReachabilityGetFlags(reachabilityRef, &flags))
	{
		return (flags & kSCNetworkReachabilityFlagsConnectionRequired);
	}
	return NO;
}

- (NetworkStatus) currentReachabilityStatus
{
	NSAssert(reachabilityRef != NULL, @"currentNetworkStatus called with NULL reachabilityRef");
	NetworkStatus retVal = NotReachable;
	SCNetworkReachabilityFlags flags;
	if (SCNetworkReachabilityGetFlags(reachabilityRef, &flags))
	{
		if(localWiFiRef)
		{
			retVal = [self localWiFiStatusForFlags: flags];
		}
		else
		{
			retVal = [self networkStatusForFlags: flags];
		}
	}
	return retVal;
}

- (void) addNetworkTrackingDelegate:(id)_adelegate
{
	[self startNotifer];
	[networkTrackingDelegates addDelegate:_adelegate];
}

- (void) removeNetworkTrackingDelegate:(id)_adelegate
{
    [networkTrackingDelegates removeDelegate:_adelegate];	
}

@end
