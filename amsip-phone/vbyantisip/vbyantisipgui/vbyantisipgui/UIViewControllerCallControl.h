#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#import "AppEngine.h"



@interface UIViewControllerCallControl : UIViewController <UIActionSheetDelegate, EngineDelegate, CallDelegate> {
    IBOutlet UIButton *buttonother;
    IBOutlet UIButton *buttonhang;
    IBOutlet UIButton *buttonhold;
    IBOutlet UIButton *buttonmute;
    IBOutlet UIImageView *image_animation;
    IBOutlet UILabel *label_num;
    IBOutlet UILabel *label_status;
    IBOutlet UILabel *label_rate;
    IBOutlet UILabel *label_duration;
	
@private
  NSArray * imageArray;
  
	UIViewController *parent;
	Call *current_call;
	NSDate *startDate;
	NSDate *endDate;
	
	NSTimer *myTimer;
	
    int pageNumber;
	
}

@property (nonatomic, retain) UIViewController *parent;

- (id)initWithPageNumber:(int)page;
- (void)setCurrentCall:(Call*)call;

- (void)actionButtonspeaker;

- (IBAction)actionButtonhang:(id)sender;
- (IBAction)actionButtonhold:(id)sender;
- (IBAction)actionButtonmute:(id)sender;
- (IBAction)actionButtonother:(id)sender;
- (IBAction)actionButtonDTMF:(id)sender;


- (void)setRemoteIdentity:(NSString*)str;
- (void)setRate:(NSString*)rate;

@end
