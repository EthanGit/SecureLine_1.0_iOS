//
//  NetworkTracking.h
//  iamsip
//
//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <sys/socket.h>
#import <netinet/in.h>
#import <netinet6/in6.h>
#import <arpa/inet.h>
#import <ifaddrs.h>
#import <netdb.h>

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
	
	typedef enum {
		NotReachable = 0,
		ReachableViaWiFi,
		ReachableViaWWAN
	} NetworkStatus;
#define kReachabilityChangedNotification @"kNetworkReachabilityChangedNotification"
	

	@interface NetworkTracking : NSObject {
		BOOL started;
		BOOL localWiFiRef;
		SCNetworkReachabilityRef reachabilityRef;
		
		id networkTrackingDelegates;
	}

	+ (NetworkTracking*) getInstance;
	- (void) addNetworkTrackingDelegate:(id)_adelegate;
	- (void) removeNetworkTrackingDelegate:(id)_adelegate;

	//private members
	@property (nonatomic, assign) id networkTrackingDelegates;

	//reachabilityWithHostName- Use to check the reachability of a particular host name. 
	+ (NetworkTracking*) reachabilityWithHostName: (NSString*) hostName;
	
	//reachabilityWithAddress- Use to check the reachability of a particular IP address. 
	+ (NetworkTracking*) reachabilityWithAddress: (const struct sockaddr_in*) hostAddress;
	
	//reachabilityForInternetConnection- checks whether the default route is available.  
	//  Should be used by applications that do not connect to a particular host
	+ (NetworkTracking*) reachabilityForInternetConnection;
	
	//reachabilityForLocalWiFi- checks whether a local wifi connection is available.
	+ (NetworkTracking*) reachabilityForLocalWiFi;
	
	//Start listening for reachability notifications on the current run loop
	- (BOOL) startNotifer;
	
	- (NetworkStatus) currentReachabilityStatus;
	//WWAN may be available, but not active until a connection has been established.
	//WiFi may require a connection for VPN on Demand.
	- (BOOL) connectionRequired;

	@end
		
