//
//  voiceConfig.m
//  VocieTest
//
//  Created by mac on 14-4-9.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import "voiceConfig.h"

@implementation voiceConfig
@synthesize address,port;
+(voiceConfig*)sharedInstance
{
    static dispatch_once_t pred = 0;
    __strong static voiceConfig* _sharedObject = nil;
    dispatch_once(&pred, ^{
        _sharedObject = [[self alloc] init]; // or some other init method
        _sharedObject.address = @"192.168.1.100";//@"183.57.16.34";
        _sharedObject.port=9009;
    });
    return _sharedObject;
}
@end
