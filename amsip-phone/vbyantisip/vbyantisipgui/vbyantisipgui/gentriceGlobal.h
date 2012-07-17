//
//  gentriceGlobal.h
//  vbyantisipgui
//
//  Created by arthur on 2012/5/25.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import <Foundation/Foundation.h>


#define GENTRICE_CODE           1

#if GENTRICE_CODE > 0
//===========================================================
#define GENTRICE
#define GENTRICE_DEBUG          1


#define APP_VERSION             @"1.0a-125"
#define APP_SDK_VERSION         @"4.7.0-79"
#define APP_IOS_VERSION         @"4.7.0-26"

//===========================================================

#else 

#undef  GENTRICE
#undef  GENTRICE_DEBUG

#endif //GENTRICE_CODE > 0



@interface gentriceGlobal : NSObject

@end
