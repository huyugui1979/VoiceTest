//
//  voiceConfig.h
//  VocieTest
//
//  Created by mac on 14-4-9.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface voiceConfig : NSObject
{
    
}
@property (retain,atomic) NSString* address;
@property (assign,atomic) short port;
@property (assign,atomic) int  timeout;
+(voiceConfig*)sharedInstance;
@end
