#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "AppEngine.h"

@interface passcodeUIViewController : UIViewController <UITextFieldDelegate, EngineDelegate>   {
    IBOutlet UITextField *target_field;

    IBOutlet UILabel *pass1_field;
    IBOutlet UILabel *pass2_field;
    IBOutlet UILabel *pass3_field;
    IBOutlet UILabel *pass4_field;
	//NSTimer *timerPlus;
    
    Call *current_call;
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

@property (retain, nonatomic) IBOutlet UILabel *title_message;
@property Call *current_call;

-(void)actionbuttonplus;
-(void)pushView;
-(void)checkPasscode;
-(void)pushCallControlList;
-(void)setShowfield:(NSString *)string;
@end
