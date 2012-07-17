//
//  apiGlobal.h
//  vbyantisipgui
//
//  Created by sanji on 12/7/4.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

#define KeystoneAuth           1

//#define keystoneGetAPIURL   @"http://zoo.snatalk.com/~KeyStone/SecureLine_Conn_Test_11.php"
#define keystoneGetAPIURL   @"https://www.securekingdom.com/KeyStone/SecureLine_Conn_Test_11.php"
#define keystoneLoginVar    @"?uid=%@&token=%@&devicetoken=%@"   
//#define keystoneGetAPIURL   @"http://apps.glob-sq.com:60080/api/"
//#define keystoneLoginVar    @"?API=11&R=T" 

#define proxyRegisterAPIURL   @"https://sip.securekingdom.com/register/"
#define proxyRegisterVar    @"?username=%@"  


#define tokenRegisterAPIURL @"https://sip.securekingdom.com//RAPI/"
#define tokenRegisterVar   @"mapping.php?Action=setPhoneMapping&UDID=%@&exten=%@" 


#define SIPProxyServer @"sip.securekingdom.com"
#define SIPProxyServerRegex @"sip\\.securekingdom\\.com"
#define SIPProxyPort   @":5161"

@interface apiGlobal : NSObject

@end
