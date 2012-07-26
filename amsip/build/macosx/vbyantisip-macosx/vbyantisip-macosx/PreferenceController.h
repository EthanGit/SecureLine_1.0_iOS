//
//  PreferenceController.h
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/1/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface PreferenceController : NSWindowController {
	
	IBOutlet NSTextField *proxy;
	IBOutlet NSTextField *username;
	IBOutlet NSTextField *password;
	IBOutlet NSTextField *identity;
	IBOutlet NSTextField *stun;
	IBOutlet NSTextField *outbound;
	IBOutlet NSComboBox *transport;
	IBOutlet NSComboBox *combobox_input_sndcard;
	IBOutlet NSComboBox *combobox_output_sndcard;
	IBOutlet NSComboBox *combobox_camera;
}

- (IBAction)actionButtonSave:(id)sender;
- (IBAction)actionButtonCancel:(id)sender;

@end
