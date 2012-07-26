//
//  Vbyantisip_NSOpenGLView.m
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/2/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>

#import "Vbyantisip_NSOpenGLView.h"

@implementation Vbyantisip_NSOpenGLView

- (void) init_image
{
    //uint32_t yuv_color = 128 << 24 | 128 << 16 | 128 << 8 | 128;
    uint32_t yuv_color = 0 << 24 | 128 << 16 | 0 << 8 | 128;
    
    [mutex lock];
    
    [mNSOpenGLContext makeCurrentContext];
    
    /* Free previous texture if any */
    if (mTextureRef)
        glDeleteTextures (1, &mTextureRef);
    
    if (mOpenGLBuf)
    {
        mOpenGLBuf = realloc (mOpenGLBuf, sizeof (char) *
                                  mVideoWidth * mVideoHeight * 4);
    }
    else
    {
        mOpenGLBuf = malloc (sizeof (char) *
                                 mVideoWidth * mVideoHeight * 2);
        
        if (mFormat==5) {
            uint32_t *p, *q;
            p = (uint32_t *) mOpenGLBuf;
            q = (uint32_t *) (char *) (mOpenGLBuf + (sizeof(char) * mVideoWidth * mVideoHeight * 2));
            
            for (; p < q; p++) *p = yuv_color;
        } else {
            memset(mOpenGLBuf, 0, mVideoWidth * mVideoHeight * 4);
        }
        
    }
    
    /* Create textures */
    glGenTextures (1, &mTextureRef);
    
    glEnable (GL_TEXTURE_RECTANGLE_EXT);
    glEnable (GL_UNPACK_CLIENT_STORAGE_APPLE);
    
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, mVideoWidth);
    
    glBindTexture (GL_TEXTURE_RECTANGLE_EXT, mTextureRef);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    /* Use VRAM texturing */
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                     GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
    
    /* Tell the driver not to make a copy of the texture but to use
     our buffer */
    glPixelStorei (GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
    
    /* Linear interpolation */
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                     GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                     GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    /* I have no idea what this exactly does, but it seems to be
     necessary for scaling */
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                     GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT,
                     GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    if (mFormat==5)
    {
        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA,
                      mVideoWidth, mVideoHeight, 0,
                      GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                      mOpenGLBuf);
    } else {
        glTexImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA,
                      mVideoWidth, mVideoHeight, 0,
                      GL_BGRA, GL_UNSIGNED_INT_8_8_8_8,
                      mOpenGLBuf);
    }
    
    mOpenglReady = YES;
    [mutex unlock];
    
}

- (void) update_image
{
    if (mOpenglReady==NO)
    {
        return;
    }
    
    [mutex lock];
    
    [mNSOpenGLContext makeCurrentContext];
    
    glBindTexture (GL_TEXTURE_RECTANGLE_EXT, mTextureRef);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, mVideoWidth);
    
    if (mFormat==5) {
        glTexSubImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0,
                         mVideoWidth, mVideoHeight,
                         GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE,
                         mOpenGLBuf);
    } else {
        glTexSubImage2D (GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0,
                         mVideoWidth, mVideoHeight,
                         GL_BGRA, GL_UNSIGNED_INT_8_8_8_8,
                         mOpenGLBuf);
    }
    [mutex unlock];
}

- (void) initializeAll: (NSRect) frame {
    
    mNSOpenGLContext = [self openGLContext];
    [mNSOpenGLContext makeCurrentContext];
    [mutex lock];
    [mNSOpenGLContext update];
    [mutex unlock];
    
    mTextureRef = 0;
    mOpenglReady = NO;
    mVideoWidth = 10;
    mVideoHeight = 10;
    mFormat = 5;
    mOpenGLBuf = nil;
    mutex = [[NSLock alloc] init];
    
    [self init_image];
    
    glClearColor (0.0, 0.0, 0.0, 0.0);

    return;
}

- (id) initWithFrame: (NSRect) frame
{
    NSOpenGLPixelFormatAttribute attribs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAWindow,
        0
    };
    
    NSOpenGLPixelFormat * fmt = [[NSOpenGLPixelFormat alloc]
                                 initWithAttributes: attribs];
    
    if (!fmt)
    {
        NSLog (@"error: Cannot create NSOpenGLPixelFormat\n");
        return nil;
    }
    
    self = [super initWithFrame:frame pixelFormat:fmt];
    [self initializeAll:frame];
    return self;
}

- (void) awakeFromNib
{
    NSRect rect = [self frame];
    NSOpenGLPixelFormatAttribute attribs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAWindow,
        0
    };
    
    NSOpenGLPixelFormat * fmt = [[NSOpenGLPixelFormat alloc]
                                 initWithAttributes: attribs];
    
    if (!fmt)
    {
        NSLog (@"error: Cannot create NSOpenGLPixelFormat\n");
        return;
    }
    
    [self setPixelFormat:fmt];
    
    [self initializeAll:rect];
}

- (void) drawRect: (NSRect) rect
{
    [mNSOpenGLContext makeCurrentContext];
    
    if (mOpenglReady==NO)
        return;
    
    [mutex lock];
        
    GLint params[] = { 1 };
    CGLSetParameter (CGLGetCurrentContext(), kCGLCPSwapInterval, params);
    glBindTexture (GL_TEXTURE_RECTANGLE_EXT, mTextureRef);

    
    float X = 1.0, Y = 1.0;
    glBegin (GL_QUADS);
    glTexCoord2f (0.0, 0.0);
    glVertex2f (-X, Y);
    glTexCoord2f (0.0, (float) mVideoHeight);
    glVertex2f (-X, -Y);
    glTexCoord2f ((float) mVideoWidth, (float) mVideoHeight);
    glVertex2f (X, -Y);
    glTexCoord2f ((float) mVideoWidth, 0.0);
    glVertex2f (X, Y);
    glEnd();

    [mNSOpenGLContext flushBuffer];
    [mutex unlock];
}

-(void)setImage:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels
{
    if (mVideoWidth!=width && mVideoHeight!=height)
    {
        mVideoWidth = width;
        mVideoHeight = height;
        if (format==5) //MS_UYVY
            mFormat=5;
        else
            mFormat=format;
        [self init_image];
    }

    if (mFormat==5) //MS_UYVY
        memcpy(mOpenGLBuf, pixels, sizeof (char) * mVideoWidth * mVideoHeight * 2);
    else
        memcpy(mOpenGLBuf, pixels, sizeof (char) * mVideoWidth * mVideoHeight * 4);
}

- (void) reshape
{
    [mutex lock];
    
    if (mOpenglReady==NO)
    {
        [mutex unlock];
        return;
    }
    
    [mNSOpenGLContext makeCurrentContext];
    
    NSRect rect = [self bounds];
    glViewport (0, 0, rect.size.width, rect.size.height);
        
    [mutex unlock];
}


- (void) displayCurrentImage {
    if ([self lockFocusIfCanDraw])
    {
        [self drawRect: [self bounds]];
        [self update_image];
        [self unlockFocus];
    }
}


@end
