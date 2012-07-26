//
//  Vbyantisip_NSOpenGLView.h
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/2/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <AppKit/AppKit.h>

@interface Vbyantisip_NSOpenGLView : NSOpenGLView{
    BOOL mOpenglReady;
    int mVideoWidth;
    int mVideoHeight;
    int mFormat;

    char *mOpenGLBuf;
    GLuint mTextureRef;
    NSOpenGLContext *mNSOpenGLContext;
    NSLock *mutex;
}

- (void)displayCurrentImage;
- (void)setImage:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels;

@end
