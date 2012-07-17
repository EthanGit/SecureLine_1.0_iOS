#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#if !TARGET_IPHONE_SIMULATOR
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureDevice.h>
#import "RosyWriterVideoProcessor.h"
#endif

#import "AppEngine.h"

#include <mediastreamer2/mscommon.h>

struct mblk_t;

#if !TARGET_IPHONE_SIMULATOR

@interface UIViewControllerVideoCallControl : UIViewController <RosyWriterVideoProcessorDelegate, EngineDelegate, CallDelegate> {
	
    BOOL mPrivacy;
    
	IBOutlet UIView *entireView;
    
@private
	UIView *selfView;
	UIImageView *imageview_remoteView;
    
	struct msgb *fake_buffer;
    struct msgb *mStaticPicture;
    int mStaticPicture_width;
    int mStaticPicture_height;
    int mStaticPicture_format;
    CGContextRef bitmap;
	ms_mutex_t bitmap_lock;

	NSTimer *myTimer;
    NSTimer *myFpsTimer;
    RosyWriterVideoProcessor *videoProcessor;
	UIBackgroundTaskIdentifier backgroundRecordingID;
	BOOL shouldShowStats;
    
	UILabel *frameRateLabel;
	UILabel *dimensionsLabel;
	UILabel *typeLabel;
    
}

#else

@interface UIViewControllerVideoCallControl : UIViewController <EngineDelegate, CallDelegate> {
	
    BOOL mPrivacy;
    
	IBOutlet UIView *entireView;
	
@private
	UIView *selfView;
	UIImageView *imageview_remoteView;
    
	struct msgb *fake_buffer;
    struct msgb *mStaticPicture;
    int mStaticPicture_width;
    int mStaticPicture_height;
    int mStaticPicture_format;
    CGContextRef bitmap;
	ms_mutex_t bitmap_lock;

	NSTimer *myTimer;
    NSTimer *myFpsTimer;
	UIBackgroundTaskIdentifier backgroundRecordingID;
	BOOL shouldShowStats;
    
	UILabel *frameRateLabel;
	UILabel *dimensionsLabel;
	UILabel *typeLabel;
    
}

#endif

-(void) setImage:(void*)pixel withWidth:(int)width withHeight:(int)height withFormat:(int)format withSize:(int)size;
-(void) drawRemoteImage;
-(void) getStaticPicture;

@end
