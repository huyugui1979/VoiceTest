//
//  VoiceController.h
//  VocieTest
//
//  Created by user on 14-1-9.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include "VoiceSession.h"
@interface VoiceController : NSObject
{
    AudioStreamBasicDescription	_recordFormat;
    AudioQueueRef				_recordQueue;
    AudioQueueRef                _playQueue;
    AudioQueueBufferRef         _recordBuffers[2];
    AudioQueueBufferRef         _playBuffers[2];
    
    VoiceSession* _voiceSession;
    int  _play;

}
+(VoiceController*)sharedInstance;
-(id)init;
-(int)startRecord;//开始录音
-(int)startPlay;//开始放音
-(int)stopRecord;//
-(int)setAudioRecv:(int)playerId redv:(bool)recv;//设置接收录音玩家列表，playerid＝0针对所有玩家，
-(int)stopPlay;
-(int)loginServer:(int)playerId;
-(int)loginOut;
-(int)beginTalk;
-(int)stopTalk;
-(int)getRoomList:(NSMutableArray*)array;
-(int)creaetRoom:(int)roomId;
-(int)getMemberList:(int)RoomId array:(NSMutableArray*)array;
-(int)enterRoom:(int)roomId;
-(int)leaveRoom;
-(int)connectServer:(NSString*)serverAdress port:(short)port;
-(int)resetServer;
@end
