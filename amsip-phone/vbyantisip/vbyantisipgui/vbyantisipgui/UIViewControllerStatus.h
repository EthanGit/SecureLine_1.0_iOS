#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

@protocol NetworkTrackingDelegate;

@interface UIViewControllerStatus : UIViewController <NetworkTrackingDelegate>{

	IBOutlet UILabel* label_summary;
	IBOutlet UILabel* label_identity;
	
	IBOutlet UIImageView* remoteSIPIcon;
	IBOutlet UILabel* label_sipstatus;
	
	IBOutlet UIImageView* internetConnectionIcon;
	IBOutlet UILabel* label_networkstatus;
}

+(void)_keepAtLinkTime;

- (void) configureSIPStatus: (int) statusCode;

@end
