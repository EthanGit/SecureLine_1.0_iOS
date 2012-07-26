#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

@interface UIViewControllerDialpad : UIViewController <UITextFieldDelegate>   {
    IBOutlet UITextField *target_field;

	NSTimer *timerPlus;
}

+(void)_keepAtLinkTime;

- (IBAction)actionButton0:(id)sender;
- (IBAction)actionButton1:(id)sender;
- (IBAction)actionButton2:(id)sender;
- (IBAction)actionButton3:(id)sender;
- (IBAction)actionButton4:(id)sender;
- (IBAction)actionButton5:(id)sender;
- (IBAction)actionButton6:(id)sender;
- (IBAction)actionButton7:(id)sender;
- (IBAction)actionButton8:(id)sender;
- (IBAction)actionButton9:(id)sender;
- (IBAction)actionButtondelete:(id)sender;
- (IBAction)actionButtonhang:(id)sender;
- (IBAction)actionButtonpound:(id)sender;
- (IBAction)actionButtonstar:(id)sender;
- (IBAction)actionButtonstart:(id)sender;
- (IBAction)actionButton0up:(id)sender;

-(void)actionbuttonplus;

-(void)pushCallControlList;

@end
