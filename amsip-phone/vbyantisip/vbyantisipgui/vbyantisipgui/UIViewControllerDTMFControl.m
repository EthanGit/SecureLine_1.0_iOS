//
//  UIViewControllerDTMFControl.m
//  vbyantisipgui
//
//  Created by  on 2012/3/26.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "UIViewControllerDTMFControl.h"

#import "AppEngine.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerCallControl.h"
#include <amsip/am_options.h>


@implementation UIViewControllerDTMFControl

@synthesize current_call_dtmf;


- (IBAction)actionDTMF1:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF1");
    //if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'1'];
    
}

- (IBAction)actionDTMF2:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF2");
    //if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'2'];
    
}

- (IBAction)actionDTMF3:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF3");
   // if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'3'];
    
}

- (IBAction)actionDTMF4:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF4");
    //if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'4'];
    
}

- (IBAction)actionDTMF5:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF5");    
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'5'];
    
}

- (IBAction)actionDTMF6:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF6");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'6'];
    
}

- (IBAction)actionDTMF7:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF7");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'7'];
    
}

- (IBAction)actionDTMF8:(id)sender {
    
    NSLog(@"UIViewControllerDTMFControl actionDTMF8");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'8'];
    
}

- (IBAction)actionDTMF9:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF9");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:9];
    
}

- (IBAction)actionDTMF0:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMF0");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'0'];
    
}

- (IBAction)actionDTMFpound:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMFpound");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'#'];
    
}

- (IBAction)actionDTMFstar:(id)sender {
    NSLog(@"UIViewControllerDTMFControl actionDTMFstar");     
    if (current_call_dtmf!=nil)
        [current_call_dtmf sendDtmf:'*'];
    
}

- (void)dealloc {
    [super dealloc];

}
/*

- (void)setCurrentCall:(Call*)call
{
    NSLog(@"UIViewControllerDTMFControl setCurrentCall");
	if (current_call_dtmf==call)
		return;
	

	
	//reset
	if (current_call_dtmf!=nil)
	{
		[current_call_dtmf removeCallStateChangeListener:self];
		current_call_dtmf=nil;
	}

	current_call_dtmf=call;
	if (current_call_dtmf!=nil)
	{
		[self onCallUpdate:call];
		[current_call_dtmf addCallStateChangeListener:self];
	}
}
*/
@end



