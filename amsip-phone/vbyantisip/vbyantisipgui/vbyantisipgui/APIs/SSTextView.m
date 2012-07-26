//
//  UIPlaceHolderTextView.m
//  vbyantisipgui
//
//  Created by sanji on 12/7/10.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "SSTextView.h"

@interface SSTextView ()
- (void)_initialize;
- (void)_updateShouldDrawPlaceholder;
- (void)_updatefirstShouldDrawPlaceholder;
- (void)_textChanged:(NSNotification *)notification;
@end


@implementation SSTextView {
    BOOL _shouldDrawPlaceholder;
    NSInteger _text_height;
}


#pragma mark - Accessors

@synthesize placeholder = _placeholder;
@synthesize placeholderTextColor = _placeholderTextColor;


- (void)setText:(NSString *)string {
    [super setText:string];
    
    [self _updateShouldDrawPlaceholder];
}

- (void)setFirstText:(NSString *)string {
    [super setText:string];
}


- (void)setPlaceholder:(NSString *)string setHeight:(NSInteger)height{
    if ([string isEqual:_placeholder]) {
        return;
    }
    
    _placeholder = string;
    _text_height = height;
    [self _updateShouldDrawPlaceholder];
}

#pragma mark - NSObject

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UITextViewTextDidChangeNotification object:self];
}


#pragma mark - UIView

- (id)initWithCoder:(NSCoder *)aDecoder {
    if ((self = [super initWithCoder:aDecoder])) {
        [self _initialize];
    }
    return self;
}


- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        [self _initialize];
    }
    return self;
}


- (void)drawRect:(CGRect)rect {
    [super drawRect:rect];

    if (_shouldDrawPlaceholder) {
        NSInteger y = 8.0f;
       // NSLog(@">>> text length :%i / _text_height:%i",self.text.length,_text_height);
        if(self.text.length>0 && _text_height>0){
            
           y = _text_height;
        }

        [_placeholderTextColor set];
        [_placeholder drawInRect:CGRectMake(8.0f, y, self.frame.size.width - 16.0f, self.frame.size.height - 16.0f) withFont:self.font];//50.0f
    }
   
}


#pragma mark - Private

- (void)_initialize {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_textChanged:) name:UITextViewTextDidChangeNotification object:self];
    
    self.placeholderTextColor = [UIColor colorWithWhite:0.702f alpha:1.0f];
    _shouldDrawPlaceholder = NO;
}


- (void)_updateShouldDrawPlaceholder {
    BOOL prev = _shouldDrawPlaceholder;
    _shouldDrawPlaceholder = self.placeholder && self.placeholderTextColor && self.text.length == 0;
    
    if (prev != _shouldDrawPlaceholder) {
        [self setNeedsDisplay];
    }
}


- (void)_textChanged:(NSNotification *)notificaiton {
    [self _updateShouldDrawPlaceholder];    
}

@end
