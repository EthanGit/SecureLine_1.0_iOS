/*
     File: RosyWriterVideoProcessor.m
 Abstract: The class that creates and manages the AV capture session and asset writer
  Version: 1.2
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2011 Apple Inc. All Rights Reserved.
 
 */

#import <MobileCoreServices/MobileCoreServices.h>
#import <AssetsLibrary/AssetsLibrary.h>
#import "RosyWriterVideoProcessor.h"

#define BYTES_PER_PIXEL 4

@interface RosyWriterVideoProcessor ()

// Redeclared as readwrite so that we can write to the property and still be atomic with external readers.
@property (readwrite) Float64 videoFrameRate;
@property (readwrite) CMVideoDimensions videoDimensions;
@property (readwrite) CMVideoCodecType videoType;

@property (readwrite, getter=isRecording) BOOL recording;

@property (readwrite) AVCaptureVideoOrientation videoOrientation;

@end

@implementation RosyWriterVideoProcessor

@synthesize delegate;
@synthesize videoFrameRate, videoDimensions, videoType;
@synthesize referenceOrientation;
@synthesize videoOrientation;
@synthesize recording;
@synthesize readyUrl;
@synthesize writingUrl;

- (id) init
{
    if (self = [super init]) {
        previousSecondTimestamps = [[NSMutableArray alloc] init];
        referenceOrientation = UIDeviceOrientationPortrait;
        devicePosition = AVCaptureDevicePositionFront;
        
        NSString *documentPath = NSTemporaryDirectory();
        NSString *oldPath = [documentPath stringByAppendingPathComponent: @"video-writing.3gp"];
        writingUrl = [NSURL fileURLWithPath:oldPath];
        [writingUrl retain];
        NSString *newPath = [documentPath stringByAppendingPathComponent: @"video-ready.3gp"];
        readyUrl = [NSURL fileURLWithPath:newPath];
        [readyUrl retain];
    }
    return self;
}

- (void)dealloc 
{
    [previousSecondTimestamps release];

    [writingUrl release];
    [readyUrl release];
	[super dealloc];
}

#pragma mark Utilities

- (void) calculateFramerateAtTimestamp:(CMTime) timestamp
{
	[previousSecondTimestamps addObject:[NSValue valueWithCMTime:timestamp]];
    
	CMTime oneSecond = CMTimeMake( 1, 1 );
	CMTime oneSecondAgo = CMTimeSubtract( timestamp, oneSecond );
    
	while( CMTIME_COMPARE_INLINE( [[previousSecondTimestamps objectAtIndex:0] CMTimeValue], <, oneSecondAgo ) )
		[previousSecondTimestamps removeObjectAtIndex:0];
    
	Float64 newRate = (Float64) [previousSecondTimestamps count];
	self.videoFrameRate = (self.videoFrameRate + newRate) / 2;
}

- (NSURL *)getNextFile
{
    int i=0;
    NSFileManager *fileManager = [NSFileManager defaultManager];

    for (i=1;i<26;i++)
    {
        NSString *documentPath = NSTemporaryDirectory();
        NSString *tmp = [NSString stringWithFormat:@"Movie%i.mov", i];
        NSString *moviePath = [documentPath stringByAppendingPathComponent: tmp];
        if ([fileManager fileExistsAtPath:moviePath]) {
            
            NSError *error;
            BOOL success = [fileManager removeItemAtPath:moviePath error:&error];
            if (!success)
                [self showError:error];
            NSLog(@"file removed in %@", moviePath);
        }
    }
    
    NSString *filePath = [writingUrl path];
    if ([fileManager fileExistsAtPath:filePath]) {
        NSError *error;
        BOOL success = [fileManager removeItemAtPath:filePath error:&error];
        if (!success) {
            NSLog(@"error: file not removed in %@", filePath);
            return nil;
        }
        NSLog(@"file removed in %@", filePath);
    }
    return writingUrl;
}

- (CGFloat)angleOffsetFromPortraitOrientationToOrientation:(AVCaptureVideoOrientation)orientation
{
	CGFloat angle = 0.0;
	
	switch (orientation) {
		case AVCaptureVideoOrientationPortrait:
			angle = 0.0;
			break;
		case AVCaptureVideoOrientationPortraitUpsideDown:
			angle = M_PI;
			break;
		case AVCaptureVideoOrientationLandscapeRight:
			angle = -M_PI_2;
			break;
		case AVCaptureVideoOrientationLandscapeLeft:
			angle = M_PI_2;
			break;
		default:
			break;
	}

	return angle;
}

- (CGAffineTransform)transformFromCurrentVideoOrientationToOrientation:(AVCaptureVideoOrientation)orientation
{
	CGAffineTransform transform = CGAffineTransformIdentity;

	// Calculate offsets from an arbitrary reference orientation (portrait)
	CGFloat orientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:orientation];
	CGFloat videoOrientationAngleOffset = [self angleOffsetFromPortraitOrientationToOrientation:self.videoOrientation];
	
	// Find the difference in angle between the passed in orientation and the current video orientation
	CGFloat angleOffset = orientationAngleOffset - videoOrientationAngleOffset;
	transform = CGAffineTransformMakeRotation(angleOffset);
	
	return transform;
}

#pragma mark Recording

- (void) writeSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(NSString *)mediaType
{
	if ( assetWriter.status == AVAssetWriterStatusUnknown ) {
		
        if ([assetWriter startWriting]) {			
			[assetWriter startSessionAtSourceTime:CMSampleBufferGetPresentationTimeStamp(sampleBuffer)];
		}
		else {
			[self showError:[assetWriter error]];
		}
	}
	
	if ( assetWriter.status == AVAssetWriterStatusWriting ) {
		
		if (mediaType == AVMediaTypeVideo) {
			if (assetWriterVideoIn.readyForMoreMediaData) {
				if (![assetWriterVideoIn appendSampleBuffer:sampleBuffer]) {
					[self showError:[assetWriter error]];
				}
			}
		}
	}
}

- (BOOL) setupAssetWriterVideoInput:(CMFormatDescriptionRef)currentFormatDescription 
{
    float bitsPerPixel;
    CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(currentFormatDescription);
    int numPixels = dimensions.width * dimensions.height;
    int bitsPerSecond;
    
    // Assume that lower-than-SD resolutions are intended for streaming, and use a lower bitrate
    if ( numPixels < (640 * 480) )
        bitsPerPixel = 4.05; // This bitrate matches the quality produced by AVCaptureSessionPresetMedium or Low.
    else
        bitsPerPixel = 11.4; // This bitrate matches the quality produced by AVCaptureSessionPresetHigh.
    
    bitsPerSecond = numPixels * bitsPerPixel;
    
    NSDictionary *videoCompressionSettings = [NSDictionary dictionaryWithObjectsAndKeys:
                                              AVVideoCodecH264, AVVideoCodecKey,
                                              [NSNumber numberWithInteger:dimensions.width], AVVideoWidthKey,
                                              [NSNumber numberWithInteger:dimensions.height], AVVideoHeightKey,
                                              [NSDictionary dictionaryWithObjectsAndKeys:
                                               [NSNumber numberWithInteger:bitsPerSecond], AVVideoAverageBitRateKey,
                                               [NSNumber numberWithInteger:20], AVVideoMaxKeyFrameIntervalKey,
                                               AVVideoProfileLevelH264Baseline30, AVVideoProfileLevelKey,
                                               nil], AVVideoCompressionPropertiesKey,
                                              nil];

    if ([assetWriter canApplyOutputSettings:videoCompressionSettings forMediaType:AVMediaTypeVideo]) {
        assetWriterVideoIn = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:videoCompressionSettings];
        assetWriterVideoIn.expectsMediaDataInRealTime = YES;
        assetWriterVideoIn.transform = [self transformFromCurrentVideoOrientationToOrientation:self.referenceOrientation];
        if ([assetWriter canAddInput:assetWriterVideoIn])
            [assetWriter addInput:assetWriterVideoIn];
        else {
            NSLog(@"Couldn't add asset writer video input.");
            return NO;
        }
    }
    else {
        NSLog(@"Couldn't apply video output settings.");
        return NO;
    }
        
    return YES;
}

- (void) startRecording
{
    if (captureSession==nil)
        return;
	dispatch_sync(movieWritingQueue, ^{
	
		if ( recordingWillBeStarted || self.recording )
			return;

		NSURL *movieURL = [self getNextFile];
		if (movieURL==nil)
		{
			NSLog(@"startRecording: retry later...");
			return;
		}
		NSLog(@"start recording in %@", movieURL);
    
		recordingWillBeStarted = YES;

		// recordingDidStart is called from captureOutput:didOutputSampleBuffer:fromConnection: once the asset writer is setup
		[self.delegate recordingWillStart];

		// Remove the file if one with the same name already exists

		// Create an asset writer
		NSError *error;
		//assetWriter = [[AVAssetWriter alloc] initWithURL:movieURL fileType:(NSString *)kUTTypeQuickTimeMovie error:&error];
		//assetWriter = [[AVAssetWriter alloc] initWithURL:movieURL fileType:(NSString *)AVFileTypeQuickTimeMovie error:&error];
        assetWriter = [[AVAssetWriter alloc] initWithURL:movieURL fileType:(NSString *)AVFileType3GPP error:&error];

        if (error)
        {
            NSLog(@"startRecording: AVAssetWriter initWithURL %@ error: %@", movieURL, error);
            
        } else
        {
            [assetWriter setShouldOptimizeForNetworkUse:YES];
        }    
	});	
}

- (void) stopRecording
{
    if (captureSession==nil)
        return;
	dispatch_sync(movieWritingQueue, ^{
		
		if ( recordingWillBeStopped || (self.recording == NO) )
			return;

        if ([assetWriter status]!=AVAssetWriterStatusWriting)
        {
            NSLog(@"stopRecording: AVAssetWriter status %i != AVAssetWriterStatusWriting", [assetWriter status]);
            return;
        }
		recordingWillBeStopped = YES;
		
		// recordingDidStop is called from saveMovieToCameraRoll
		[self.delegate recordingWillStop];

		if ([assetWriter finishWriting]) {
			[assetWriterVideoIn release];
			[assetWriter release];
			assetWriter = nil;
			
			readyToRecordVideo = NO;
			
            recordingWillBeStopped = NO;
            self.recording = NO;
            
            NSFileManager *fileManager = [NSFileManager defaultManager];
            NSString *filePath = [writingUrl path];
            if ([fileManager fileExistsAtPath:filePath]) {
                
                NSError *error;
                BOOL success = [fileManager moveItemAtURL:writingUrl toURL:readyUrl error:&error];
                if (!success)
                    NSLog(@"error: file renaming file to %@", readyUrl);
                else
                {
                    NSLog(@"file renamed to %@", readyUrl);
                }
            }            
            
            [self.delegate recordingDidStop];
        }
		else {
            recordingWillBeStopped = NO;
            NSLog(@"stopRecording: AVAssetWriter finishWriting %i error: %@", [assetWriter status], [assetWriter error]);

			//[self showError:[assetWriter error]];
		}
	});
}


#pragma mark Capture

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection 
{	
	CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer);
    
	if ( connection == videoConnection ) {
		
		// Get framerate
		CMTime timestamp = CMSampleBufferGetPresentationTimeStamp( sampleBuffer );
		[self calculateFramerateAtTimestamp:timestamp];
        
		// Get frame dimensions (for onscreen display)
		if (self.videoDimensions.width == 0 && self.videoDimensions.height == 0)
			self.videoDimensions = CMVideoFormatDescriptionGetDimensions( formatDescription );
		
		// Get buffer type
		if ( self.videoType == 0 )
			self.videoType = CMFormatDescriptionGetMediaSubType( formatDescription );

		// Enqueue it for preview.  This is a shallow queue, so if image processing is taking too long,
		// we'll drop this frame for preview (this keeps preview latency low).
		OSStatus err = CMBufferQueueEnqueue(previewBufferQueue, sampleBuffer);
		if ( !err ) {
			dispatch_async(dispatch_get_main_queue(), ^{
				CMSampleBufferRef sbuf = (CMSampleBufferRef)CMBufferQueueDequeueAndRetain(previewBufferQueue);
				if (sbuf) {
					CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sbuf);
					[self.delegate pixelBufferReadyForDisplay:pixBuf];
					CFRelease(sbuf);
				}
			});
		}
	}
    
	CFRetain(sampleBuffer);
	CFRetain(formatDescription);
	dispatch_sync(movieWritingQueue, ^{

		if ( assetWriter ) {
		
			BOOL wasReadyToRecord = (readyToRecordVideo);
			
			if (connection == videoConnection) {
				
				// Initialize the video input if this is not done yet
				if (!readyToRecordVideo)
					readyToRecordVideo = [self setupAssetWriterVideoInput:formatDescription];
				
				// Write video data to file
				if (readyToRecordVideo)
					[self writeSampleBuffer:sampleBuffer ofType:AVMediaTypeVideo];
			}
			
			BOOL isReadyToRecord = (readyToRecordVideo);
			if ( !wasReadyToRecord && isReadyToRecord ) {
				recordingWillBeStarted = NO;
				self.recording = YES;
				[self.delegate recordingDidStart];
			}
		}
		CFRelease(sampleBuffer);
		CFRelease(formatDescription);
	});
}

-(void)selectDevicePosition:(AVCaptureDevicePosition)position 
{
    devicePosition = position;
}

- (AVCaptureDevice *)videoDeviceWithPosition:(AVCaptureDevicePosition)position 
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices)
        if ([device position] == position)
            return device;
    
    for (AVCaptureDevice *device in devices)
        return device;
    
    return nil;
}

- (BOOL) setupCaptureSession 
{
    AVCaptureDevice *captureDevice = [self videoDeviceWithPosition:devicePosition];        
    if (captureDevice==nil)
    {
         return NO;
    }
	/*
		Overview: RosyWriter uses separate GCD queues for audio and video capture.  If a single GCD queue
		is used to deliver both audio and video buffers, and our video processing consistently takes
		too long, the delivery queue can back up, resulting in audio being dropped.
		
		When recording, RosyWriter creates a third GCD queue for calls to AVAssetWriter.  This ensures
		that AVAssetWriter is not called to start or finish writing from multiple threads simultaneously.
		
		RosyWriter uses AVCaptureSession's default preset, AVCaptureSessionPresetHigh.
	 */
	 
    /*
	 * Create capture session
	 */
    captureSession = [[AVCaptureSession alloc] init];
        
    captureSession.sessionPreset = AVCaptureSessionPresetLow;
	/*
	 * Create video connection
	 */
    AVCaptureDeviceInput *videoIn = [[AVCaptureDeviceInput alloc] initWithDevice:captureDevice error:nil];
    if ([captureSession canAddInput:videoIn])
        [captureSession addInput:videoIn];
	[videoIn release];
    
	AVCaptureVideoDataOutput *videoOut = [[AVCaptureVideoDataOutput alloc] init];
	/*
		RosyWriter prefers to discard late video frames early in the capture pipeline, since its
		processing can take longer than real-time on some platforms (such as iPhone 3GS).
		Clients whose image processing is faster than real-time should consider setting AVCaptureVideoDataOutput's
		alwaysDiscardsLateVideoFrames property to NO. 
	 */
	//[videoOut setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    
    NSDictionary *dic = [[NSDictionary alloc] initWithObjectsAndKeys:
                         [NSNumber numberWithInt:176], (id)kCVPixelBufferWidthKey,
                         [NSNumber numberWithInt:144], (id)kCVPixelBufferHeightKey,
                         [NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange], (id)kCVPixelBufferPixelFormatTypeKey,
                         nil];
    
	[videoOut setVideoSettings:dic];
    [dic release];

    [videoOut setAlwaysDiscardsLateVideoFrames:YES];
    [videoOut setMinFrameDuration:CMTimeMake(1, 15)]; //deprecated...

	dispatch_queue_t videoCaptureQueue = dispatch_queue_create("Video Capture Queue", DISPATCH_QUEUE_SERIAL);
	[videoOut setSampleBufferDelegate:self queue:videoCaptureQueue];
	dispatch_release(videoCaptureQueue);
	if ([captureSession canAddOutput:videoOut])
		[captureSession addOutput:videoOut];
	videoConnection = [videoOut connectionWithMediaType:AVMediaTypeVideo];
	self.videoOrientation = [videoConnection videoOrientation];
    [videoConnection setVideoMinFrameDuration:CMTimeMake(1, 15)];
	[videoOut release];
    
	return YES;
}

- (void) setupAndStartCaptureSession:(UIView *)selfView
{
	// Create a shallow queue for buffers going to the display for preview.
	OSStatus err = CMBufferQueueCreate(kCFAllocatorDefault, 1, CMBufferQueueGetCallbacksForUnsortedSampleBuffers(), &previewBufferQueue);
	if (err)
		[self showError:[NSError errorWithDomain:NSOSStatusErrorDomain code:err userInfo:nil]];
	
	// Create serial queue for movie writing
	movieWritingQueue = dispatch_queue_create("Movie Writing Queue", DISPATCH_QUEUE_SERIAL);
	
    if ( !captureSession )
	{
        [self setupCaptureSession];

        if (captureSession)
        {
            AVCaptureVideoPreviewLayer* previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
            previewLayer.frame = selfView.bounds;
            previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
            if ([previewLayer isOrientationSupported]==true)
                previewLayer.orientation = referenceOrientation;
            [selfView.layer addSublayer: previewLayer];
        }
    }
    
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(captureSessionStoppedRunningNotification:) name:AVCaptureSessionDidStopRunningNotification object:captureSession];
	
	if ( !captureSession.isRunning )
		[captureSession startRunning];
}

- (void) pauseCaptureSession
{
    if (captureSession==nil)
        return;
    
	if ( captureSession.isRunning )
		[captureSession stopRunning];
}

- (void) resumeCaptureSession
{
    if (captureSession==nil)
        return;
    
	if ( !captureSession.isRunning )
		[captureSession startRunning];
}

- (void)captureSessionStoppedRunningNotification:(NSNotification *)notification
{
	//dispatch_sync(movieWritingQueue, ^{
		if ( [self isRecording] ) {
			[self stopRecording];
		}
	//});
}

- (void) stopAndTearDownCaptureSession
{
    if (captureSession)
        [captureSession stopRunning];
	if (captureSession)
		[[NSNotificationCenter defaultCenter] removeObserver:self name:AVCaptureSessionDidStopRunningNotification object:captureSession];
    if (captureSession)
        [captureSession release];
	captureSession = nil;
	if (previewBufferQueue) {
		CFRelease(previewBufferQueue);
		previewBufferQueue = NULL;	
	}
	if (movieWritingQueue) {
		dispatch_release(movieWritingQueue);
		movieWritingQueue = NULL;
	}
}

#pragma mark Error Handling

- (void)showError:(NSError *)error
{
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^(void) {
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:[error localizedDescription]
                                                            message:[error localizedFailureReason]
                                                           delegate:nil
                                                  cancelButtonTitle:@"OK"
                                                  otherButtonTitles:nil];
        [alertView show];
        [alertView release];
    });
}

@end
