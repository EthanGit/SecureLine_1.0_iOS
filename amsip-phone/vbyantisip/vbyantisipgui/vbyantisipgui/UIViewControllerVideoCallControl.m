#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerVideoCallControl.h"

#import "vbyantisipAppDelegate.h"

#include <amsip/am_options.h>


#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

//#define RECORDING_MODE

static UIDeviceOrientation mCurrentOrientation = UIDeviceOrientationPortrait;
static UIViewControllerVideoCallControl* mCurrentControlerVideoCallControl=nil;

struct bmp_file_header { 
    unsigned short   type; 
    unsigned long    size; 
    unsigned long    reserved; 
    unsigned long    bitmap_offset;
    unsigned long    header_size; 
    signed   long    width; 
    signed   long    height; 
    unsigned short   planes; 
    unsigned short   bits_per_pixel; 
    unsigned long    compression; 
    unsigned long    bitmap_size;
    signed   long    horizontal_resolution;
    signed   long    vertical_resolution;
    unsigned long    num_colors; 
    unsigned long    num_important_colors; 
}; 

static void fix_endian_func(void* data, int size) {
    int endian_test = 1;
    unsigned char* endian_test_bytes = (unsigned char *)&endian_test;
    if (endian_test_bytes[0] == '\0')
    {
        unsigned char* cdata = data;
        int i;
        for (i=0; i<size/2; i++) {
            unsigned char temp = cdata[i];
            cdata[i] = cdata[size-1 - i];
            cdata[size-1 - i] = temp;
        }
    }
}

#define fix_endian(x)  (fix_endian_func(&(x), sizeof(x)))

static unsigned char read_u8(FILE * pFile) {
    unsigned char result;
    if (fread(&result, 1, 1, pFile) < 1) {
        printf("Unexpected end of file");
        exit(-1);
    }
    return result;
}

static unsigned short read_u16(FILE * pFile) {
    unsigned short result = 0;
    if (fread(&result,  1, 2, pFile) < 2) {
        printf("Unexpected end of file");
        exit(-1);
    }
    fix_endian(result);
    return result;
}

static unsigned long read_u32(FILE * pFile) {
    unsigned long result = 0;
    if (fread(&result, 1, 4, pFile) < 4) {
        printf("Unexpected end of file");
        exit(-1);
    }
    fix_endian(result);
    return result;
}

static signed long read_s32(FILE * pFile) {
    unsigned long result = 0;
    if (fread(&result, 1, 4, pFile) < 4) {
        printf("Unexpected end of file");
        exit(-1);
    }
    fix_endian(result);
    if ((result >> 31) & 1) { /* If it's negative... */
        result |= ((unsigned long)(-1)) << 32;
    }
    return (long)result;
}

static void read_bitmap_header(FILE * pFile, struct bmp_file_header* header) {
    header->type                  = read_u16(pFile);
    header->size                  = read_u32(pFile);
    header->reserved              = read_u32(pFile);
    header->bitmap_offset         = read_u32(pFile);
    header->header_size           = read_u32(pFile);
    header->width                 = read_s32(pFile);
    header->height                = read_s32(pFile);
    header->planes                = read_u16(pFile);
    header->bits_per_pixel        = read_u16(pFile); 
    header->compression           = read_u32(pFile);
    header->bitmap_size           = read_u32(pFile);
    header->horizontal_resolution = read_s32(pFile);
    header->vertical_resolution   = read_u32(pFile);
    header->num_colors            = read_u32(pFile);
    header->num_important_colors  = read_u32(pFile);
}


void _on_video_module_new_image_cb(int pin, int width, int height, int format, int size, void *pixel);

void _on_video_module_new_image_cb(int pin, int width, int height, int format, int size, void *pixel)
{
	//NSLog(@"_on_video_module_new_image_cb: receiving image %ix%i fmt=%i", width, height, format);
    
    if (mCurrentControlerVideoCallControl!=nil)
    {
        [mCurrentControlerVideoCallControl setImage:pixel withWidth:width withHeight:height withFormat:format withSize:size];
				[mCurrentControlerVideoCallControl performSelectorOnMainThread : @ selector(drawRemoteImage) withObject:nil waitUntilDone:NO];
    }
}

@implementation UIViewControllerVideoCallControl

- (void)onCallUpdate:(Call *)call {
	NSLog(@"UIViewControllerCallControl: onCallUpdate");
}

- (void)updateLabels
{
	if (shouldShowStats) {
#if !TARGET_IPHONE_SIMULATOR
		NSString *frameRateString = [NSString stringWithFormat:@"%.2f FPS ", [videoProcessor videoFrameRate]];
 		frameRateLabel.text = frameRateString;
 		[frameRateLabel setBackgroundColor:[UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.25]];
 		
 		NSString *dimensionsString = [NSString stringWithFormat:@"%d x %d ", [videoProcessor videoDimensions].width, [videoProcessor videoDimensions].height];
 		dimensionsLabel.text = dimensionsString;
 		[dimensionsLabel setBackgroundColor:[UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.25]];
 		
 		CMVideoCodecType type = [videoProcessor videoType];
 		type = OSSwapHostToBigInt32( type );
 		NSString *typeString = [NSString stringWithFormat:@"%.4s ", (char*)&type];
 		typeLabel.text = typeString;
 		[typeLabel setBackgroundColor:[UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.25]];
#endif
 	}
 	else {
 		frameRateLabel.text = @"";
 		[frameRateLabel setBackgroundColor:[UIColor clearColor]];
 		
 		dimensionsLabel.text = @"";
 		[dimensionsLabel setBackgroundColor:[UIColor clearColor]];
 		
 		typeLabel.text = @"";
 		[typeLabel setBackgroundColor:[UIColor clearColor]];
 	}
}

- (UIImageView *)createImageView {
    CGRect imageFrame = CGRectMake(0, 0, entireView.bounds.size.width, entireView.bounds.size.height);
    imageview_remoteView = [[UIImageView alloc] initWithFrame:imageFrame];
    return [imageview_remoteView autorelease];
}

- (UIView *)createSelfView {
    CGRect imageFrame = CGRectMake(entireView.bounds.size.width-72-10, entireView.bounds.size.height-96-10, 72, 96);
    selfView = [[UIView alloc] initWithFrame:imageFrame];
    return [selfView autorelease];
}

- (UILabel *)labelWithText:(NSString *)text yPosition:(CGFloat)yPosition
{
	CGFloat labelWidth = 100.0;
	CGFloat labelHeight = 20.0;
	CGFloat xPosition = imageview_remoteView.bounds.size.width - labelWidth - 10;
	CGRect labelFrame = CGRectMake(xPosition, yPosition, labelWidth, labelHeight);
	UILabel *label = [[UILabel alloc] initWithFrame:labelFrame];
	[label setFont:[UIFont systemFontOfSize:18]];
	[label setLineBreakMode:UILineBreakModeWordWrap];
	[label setTextAlignment:UITextAlignmentRight];
	[label setTextColor:[UIColor whiteColor]];
	[label setBackgroundColor:[UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.25]];
	[[label layer] setCornerRadius: 4];
	[label setText:text];
	
	return [label autorelease];
}

#pragma mark RosyWriterVideoProcessorDelegate

- (void)recordingWillStart
{
	dispatch_async(dispatch_get_main_queue(), ^{
		//[[self recordButton] setEnabled:NO];	
		//[[self recordButton] setTitle:@"Stop"];
        
		// Disable the idle timer while we are recording
		[UIApplication sharedApplication].idleTimerDisabled = YES;
        
		// Make sure we have time to finish saving the movie if the app is backgrounded during recording
		if ([[UIDevice currentDevice] isMultitaskingSupported])
			backgroundRecordingID = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{}];
	});
}

- (void)recordingDidStart
{
	dispatch_async(dispatch_get_main_queue(), ^{
		//[[self recordButton] setEnabled:YES];
	});
}

- (void)recordingWillStop
{
	dispatch_async(dispatch_get_main_queue(), ^{
		// Disable until saving to the camera roll is complete
		//[[self recordButton] setTitle:@"Record"];
		//[[self recordButton] setEnabled:NO];
		
		// Pause the capture session so that saving will be as fast as possible.
		// We resume the sesssion in recordingDidStop:
		//[videoProcessor pauseCaptureSession];
	});
}

- (void)recordingDidStop
{
	dispatch_async(dispatch_get_main_queue(), ^{
		//[[self recordButton] setEnabled:YES];
		
		[UIApplication sharedApplication].idleTimerDisabled = NO;
        
		//[videoProcessor resumeCaptureSession];
        
		if ([[UIDevice currentDevice] isMultitaskingSupported]) {
			[[UIApplication sharedApplication] endBackgroundTask:backgroundRecordingID];
			backgroundRecordingID = UIBackgroundTaskInvalid;
		}
	});
}

int iphone_add_selfview_image(int width, int height, int format, mblk_t *data);

- (void)pixelBufferReadyForDisplay:(CVPixelBufferRef)pixelBuffer
{
	// Don't make OpenGLES calls while in the background.
	//if ( [UIApplication sharedApplication].applicationState != UIApplicationStateBackground )
	//	[oglView displayPixelBuffer:pixelBuffer];
#ifdef RECORDING_MODE
    static int counter=0;
    counter++;
    if (videoProcessor && counter==20) {
        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^(void) {
            
            [videoProcessor stopRecording];
            
        });
    }
    if (videoProcessor && counter>=20) {
        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^(void) {
    
            NSFileManager *fileManager = [NSFileManager defaultManager];
            NSString *filePath = [[videoProcessor writingUrl] path];
            if ([fileManager fileExistsAtPath:filePath]) {
                NSString *filePath2 = [[videoProcessor readyUrl] path];
                if ([fileManager fileExistsAtPath:filePath2]) {
                    return;
                }
            }
            [videoProcessor startRecording];
            NSLog(@"counter reached %i", counter);
            counter=0;
        });
    }
                              
    // create a very small black window:
    if (fake_buffer!=NULL)
        iphone_add_selfview_image(4, 3, MS_YUV420P, dupb(fake_buffer));
#else
	if(CVPixelBufferLockBaseAddress(pixelBuffer, 0) == kCVReturnSuccess){
        UInt8 *data = (UInt8 *)CVPixelBufferGetBaseAddress(pixelBuffer);
        size_t size = CVPixelBufferGetDataSize(pixelBuffer);
		
		int width = CVPixelBufferGetWidth(pixelBuffer);
		int height = CVPixelBufferGetHeight(pixelBuffer);
		
		int pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);
		switch (pixelFormat) {
			case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
				//format = tmedia_nv12; // iPhone 3GS or 4
				//NSLog(@"format=NV12 %ix%i", width, height);
				break;
			case kCVPixelFormatType_422YpCbCr8:
				//format = tmedia_uyvy422; // iPhone 3
				NSLog(@"format=UYUY422 %ix%i", width, height);
				return;
			default:
				//format = tmedia_rgb32;
				NSLog(@"format=RGB32 %ix%i", width, height);
				return;
		}
        
        /* enable this to have automatic switch to static image */
        static int counter=0;
        
        counter++;
#if 0
        if (counter==15*15)
        {
            counter=0;
            if (mPrivacy==FALSE)
                mPrivacy=TRUE;
            else
                mPrivacy=FALSE;
        }
#endif
        
        mblk_t *buf;
        if (mPrivacy==FALSE)
        {
            //fix size?
            size = width*height*3/2;
            buf=allocb(size,0);
            memcpy(buf->b_wptr, data+ (16 * sizeof(UInt8)), size);
            buf->b_wptr+=size;
            if (UIDeviceOrientationIsPortrait(mCurrentOrientation))
            {
                mblk_set_payload_type(buf, 90); //ask for rotation
            }
            iphone_add_selfview_image(width, height, MS_NV12, buf);
        } else if (mStaticPicture!=NULL) {
            mblk_t *buf = dupb(mStaticPicture);
            iphone_add_selfview_image(mStaticPicture_width, mStaticPicture_height, mStaticPicture_format, buf);
        }
        
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);         
    }
#endif
}

-(void) setImage:(void*)pixel withWidth:(int)width withHeight:(int)height withFormat:(int)format withSize:(int)size {
    
		ms_mutex_lock(&bitmap_lock);
    CGColorSpaceRef colorSpaceInfo = CGColorSpaceCreateDeviceRGB();
    bitmap = CGBitmapContextCreate(pixel, width, height, 8, width*4, colorSpaceInfo, kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRelease(colorSpaceInfo);
		ms_mutex_unlock(&bitmap_lock);
}

-(void) drawRemoteImage
{
	ms_mutex_lock(&bitmap_lock);
    CGImageRef imageRef = CGBitmapContextCreateImage(bitmap);
    UIImage *image = [UIImage imageWithCGImage:imageRef];
    imageview_remoteView.image = image;
    CGImageRelease(imageRef);
	ms_mutex_unlock(&bitmap_lock);
}

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

static mblk_t *ms_load_generate_yuv(MSVideoSize *reqsize)
{
	MSPicture buf;
	mblk_t *m=NULL;
	int ysize;
	
	m = yuv_buf_alloc(&buf, reqsize->width, reqsize->height);
	ysize=buf.strides[0]*buf.h;
	memset(buf.planes[0],16,ysize);
	memset(buf.planes[1],128,ysize/4);
	memset(buf.planes[2],128,ysize/4);
	buf.planes[3]=NULL;
	return m;
}


-(void) getStaticPicture
{
	MSVideoSize reqsize;
	struct stat statbuf;
	uint8_t *jpgbuf;
	int err;
	int size;
	NSString *filePath = [[NSBundle mainBundle] pathForResource:@"no_webcam" ofType:@"jpg"];  
	int fd=open([filePath cStringUsingEncoding:NSUTF8StringEncoding],O_RDONLY);

	reqsize.width = MS_VIDEO_SIZE_CIF_W;
	reqsize.height = MS_VIDEO_SIZE_CIF_H;
	
	if (fd!=-1){
		fstat(fd,&statbuf);
		if (statbuf.st_size<=0)
		{
			close(fd);
			mStaticPicture = ms_load_generate_yuv(&reqsize);
			mStaticPicture_width = reqsize.width;
			mStaticPicture_height = reqsize.height;
			return;
		}
		jpgbuf=(uint8_t*)ms_malloc0(statbuf.st_size);
		if (jpgbuf==NULL)
		{
			close(fd);
			mStaticPicture = ms_load_generate_yuv(&reqsize);
			mStaticPicture_width = reqsize.width;
			mStaticPicture_height = reqsize.height;
			return;
		}
		err=read(fd,jpgbuf,statbuf.st_size);
		if (err!=statbuf.st_size){
			ms_error("Could not read as much as wanted: %i<>%i !",err,statbuf.st_size);
		}
		mStaticPicture=ms_yuv_load_mjpeg(jpgbuf,statbuf.st_size,&reqsize);
		ms_free(jpgbuf);
		if (mStaticPicture==NULL)
		{
			close(fd);
			mStaticPicture = ms_load_generate_yuv(&reqsize);
			mStaticPicture_width = reqsize.width;
			mStaticPicture_height = reqsize.height;
			return;
		}
	}else{
		mStaticPicture = ms_load_generate_yuv(&reqsize);
		mStaticPicture_width = reqsize.width;
		mStaticPicture_height = reqsize.height;
		return;
	}
	close(fd);

	mStaticPicture_width = reqsize.width;
	mStaticPicture_height = reqsize.height;
	mStaticPicture_format=MS_YUV420P;
	size = mStaticPicture_width*mStaticPicture_height*3/2;
	
	NSLog(@"Static image loaded: %ix%i, %i, %i", mStaticPicture_width, mStaticPicture_height, mStaticPicture_format, size);
}

#if !TARGET_IPHONE_SIMULATOR

#endif

- (void) sendStaticImage {
    if (mStaticPicture!=NULL) {
        mblk_t *buf = dupb(mStaticPicture);
        iphone_add_selfview_image(mStaticPicture_width, mStaticPicture_height, mStaticPicture_format, buf);
    }
}

- (void) start {
#if !TARGET_IPHONE_SIMULATOR
	// Keep track of changes to the device orientation so we can update the video processor
    
	NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
	[notificationCenter addObserver:self selector:@selector(deviceOrientationDidChange) name:UIDeviceOrientationDidChangeNotification object:nil];
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    
    AVCaptureDevice *lAVCaptureDevice=nil;
    NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (AVCaptureDevice *tmp in cameras){
		
		if([tmp position]==AVCaptureDevicePositionFront)
		{
			lAVCaptureDevice = tmp;
			break;
		}
	}
	
	if(lAVCaptureDevice==nil)
	{
		NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
		for (AVCaptureDevice *tmp in cameras){
			lAVCaptureDevice = tmp;
			break;
		}
	}
	
	if(lAVCaptureDevice==nil)
    {
        if (myTimer==nil)
        {
            myTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(sendStaticImage) userInfo:nil repeats:YES];
        }
        return;
    }
    // Initialize the class responsible for managing AV capture session and asset writer
    videoProcessor = [[RosyWriterVideoProcessor alloc] init];
	videoProcessor.delegate = self;
    [videoProcessor setReferenceOrientation:mCurrentOrientation];
        
    // Setup and start the capture session
    [videoProcessor setupAndStartCaptureSession:selfView];

#ifdef RECORDING_MODE
    [videoProcessor startRecording];
#endif
    
#else
    if (myTimer==nil)
    {
        myTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(sendStaticImage) userInfo:nil repeats:YES];
    }
#endif
}


- (void)stop {

    if (myTimer!=nil)
    {
        [myTimer invalidate];
        myTimer=nil;
    }
#if !TARGET_IPHONE_SIMULATOR
    if (videoProcessor)
        [videoProcessor pauseCaptureSession];
#ifdef RECORDING_MODE
    if (videoProcessor) {
        [videoProcessor stopRecording];

    }
#endif
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
	[notificationCenter removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    
    //wait for stopRecording to be done...
    if (videoProcessor) {
        // Stop and tear down the capture session
        [videoProcessor stopAndTearDownCaptureSession];
        videoProcessor.delegate = nil;
        [videoProcessor release];
        videoProcessor=nil;
    }
    
	for (UIView *view in selfView.subviews) {
		[view removeFromSuperview];
	}
#endif
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
	
	[myFpsTimer invalidate];
	myFpsTimer = nil;
    
    [self stop];
    am_option_set_image_callback(NULL);
    mCurrentControlerVideoCallControl=nil;
    if (mStaticPicture==NULL)
        freemsg(mStaticPicture);
    mStaticPicture=NULL;
    if (fake_buffer==NULL)
        freemsg(fake_buffer);
    fake_buffer=NULL;
		ms_mutex_destroy(&bitmap_lock);
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];	
	
		ms_mutex_init(&bitmap_lock,NULL);
    [self getStaticPicture];

    int width=4;
    int height=3;
	YuvBuf frame;
    fake_buffer = yuv_buf_alloc(&frame,width,height);
    int ysize=frame.strides[0]*frame.h;
    memset(frame.planes[0],16,ysize);
    memset(frame.planes[1],128,ysize/4);
    memset(frame.planes[2],128,ysize/4);

    mPrivacy=FALSE;
    [self start];
    
    am_option_set_image_callback(&_on_video_module_new_image_cb);
    mCurrentControlerVideoCallControl=self;
    
	myFpsTimer = [NSTimer scheduledTimerWithTimeInterval:0.25 target:self selector:@selector(updateLabels) userInfo:nil repeats:YES];
    
}

// UIDeviceOrientationDidChangeNotification selector
- (void)deviceOrientationDidChange
{
	UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation==mCurrentOrientation)
        return;
    
	// Don't update the reference orientation when the device orientation is face up/down or unknown.
	if ( UIDeviceOrientationIsPortrait(orientation) || UIDeviceOrientationIsLandscape(orientation) )
	{
        mCurrentOrientation = orientation;

        [self stop];
        for (UIView *view in entireView.subviews) {
            [view removeFromSuperview];
        }
        frameRateLabel = nil;
        dimensionsLabel = nil;
        typeLabel = nil;
        imageview_remoteView = nil;
        selfView = nil;
        
        imageview_remoteView = [self createImageView];
        [entireView addSubview:imageview_remoteView];
        selfView = [self createSelfView];
        [entireView addSubview:selfView];
        frameRateLabel = [self labelWithText:@"" yPosition: (CGFloat) 10.0];
        [imageview_remoteView addSubview:frameRateLabel];
        
        dimensionsLabel = [self labelWithText:@"" yPosition: (CGFloat) 34.0];
        [imageview_remoteView addSubview:dimensionsLabel];
        
        typeLabel = [self labelWithText:@"" yPosition: (CGFloat) 58.0];
        [imageview_remoteView addSubview:typeLabel];
        
        am_option_enable_preview(0);
        NSLog(@"viewWillAppear: window size %ix%i", (int)entireView.bounds.size.width/2, (int)entireView.bounds.size.height/2);
        if (UIDeviceOrientationIsPortrait(mCurrentOrientation))
            am_option_set_window_handle(0, entireView.bounds.size.width/2, entireView.bounds.size.height/2);
        else {
            am_option_set_window_handle(0, entireView.bounds.size.width/2, entireView.bounds.size.height/2);
        }
        am_option_enable_preview(1);
        
        [self start];
    }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {

    return YES;
}

- (void)viewDidUnload {
	[super viewDidUnload];
    [self stop];
    frameRateLabel = nil;
    dimensionsLabel = nil;
    typeLabel = nil;
    imageview_remoteView = nil;
    selfView = nil;
}

- (void)viewDidLoad {
	[super viewDidLoad];

 	// Set up labels
 	shouldShowStats = YES;
    imageview_remoteView = [self createImageView];
    [entireView addSubview:imageview_remoteView];
    selfView = [self createSelfView];
    [entireView addSubview:selfView];
	frameRateLabel = [self labelWithText:@"" yPosition: (CGFloat) 10.0];
	[imageview_remoteView addSubview:frameRateLabel];
	
	dimensionsLabel = [self labelWithText:@"" yPosition: (CGFloat) 34.0];
	[imageview_remoteView addSubview:dimensionsLabel];
	
	typeLabel = [self labelWithText:@"" yPosition: (CGFloat) 58.0];
	[imageview_remoteView addSubview:typeLabel];
}

- (void)dealloc {

    [self stop];
    frameRateLabel = nil;
    dimensionsLabel = nil;
    typeLabel = nil;
    imageview_remoteView = nil;
    selfView = nil;
    [super dealloc];
}


@end
