//
//  vbyantisipAppDelegate.h
//  AppEngine
//
//  Created by Aymeric MOIZARD on 16/10/09.
//  Copyright antisip 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

//#import "UIViewControllerCallControl.h"
//#import "UIViewControllerStatus.h"
//#import "UIViewControllerDialpad.h"


//#import "AppEngine.h"
//#import "NetworkTrackingDelegate.h"

#import "gentriceGlobal.h"


@class Call;
@class Registration;

@protocol NetworkTrackingDelegate;
@protocol EngineDelegate;

@interface UITabBarController (Autorotate)
@end

@implementation UITabBarController (Autorotate)

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    UIViewController *controller = self.selectedViewController;
    if ([controller isKindOfClass:[UINavigationController class]])
        controller = [(UINavigationController *)controller visibleViewController];
    return [controller shouldAutorotateToInterfaceOrientation:interfaceOrientation];
}

@end

@interface vbyantisipAppDelegate : NSObject <UIApplicationDelegate, UITabBarControllerDelegate, NetworkTrackingDelegate, EngineDelegate> {
	IBOutlet UIWindow *window;
	IBOutlet UITabBarController *tabBarController;
@public
	IBOutlet UIViewController *viewControllerDialpad;
	//IBOutlet UIViewControllerCallControl *viewControllerCallControl;
	//IBOutlet UIViewControllerStatus *viewControllerStatus;
	
	
	Registration *registration;

    NSString *tmp_data;
    BOOL connect_finished;
    
@private
	bool isInBackground;
	NSString *proxy;
	NSString *login;
	NSString *password;
	NSString *identity;
	NSString *transport;
	NSString *outboundproxy;
	NSString *stun;
	int ptime;
	BOOL speex16k;
	BOOL g729;
	BOOL g722;
	BOOL iLBC;
	BOOL speex8k;
	BOOL gsm8k;
	BOOL pcmu;
	BOOL pcma;
	BOOL aec;
	BOOL elimiter;
	BOOL srtp;
	BOOL naptr;
	int reginterval;
    
	BOOL vp8;
	BOOL h264;
	BOOL mp4v;
	BOOL h2631998;
	int uploadbandwidth;
	int downloadbandwidth;
    

    
#ifdef GENTRICE
    UIAlertView *currentAlert;
    BOOL doingPasscodeLogin;
#endif
}

+(void)_initialize;

-(void)dokeepAliveHandler;
-(void)stopBackgroundTask;

-(void)showCallControlView;

-(void)restartAll;
-(void)updatetokeninfo;
-(void)onCallNew:(Call *)call;
-(void)onCallRemove:(Call *)call;

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;
//@property (nonatomic, retain) IBOutlet UIViewControllerCallControl *viewControllerCallControl;
//@property (nonatomic, retain) IBOutlet UIViewControllerStatus *viewControllerStatus;

#ifdef GENTRICE
@property (nonatomic, retain, readonly) NSManagedObjectContext *managedObjectContext;
@property (nonatomic, retain, readonly) NSManagedObjectModel *managedObjectModel;
@property (nonatomic, retain, readonly) NSPersistentStoreCoordinator *persistentStoreCoordinator;
@property UIAlertView *currentAlert;
@property BOOL doingPasscodeLogin;
@property (retain, nonatomic) NSString *tmp_data;
@property BOOL connect_finished;

- (void)saveContext;
- (NSURL *)applicationDocumentsDirectory;
- (NSString*) stripSecureIDfromURL:(NSString*) targetString ;
- (void) checkPasscodeLoginStatus:(Call*) call;
- (void)createPersistenStoreCoordinator;
- (NSString*) lookupDisplayName:(NSString*) caller;

- (void)initSystemAlert;
- (void)dismissSystemAlert;

#endif //GENTRICE

@end
