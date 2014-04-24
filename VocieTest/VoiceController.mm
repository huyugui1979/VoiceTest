//
//  VoiceController.m
//  VocieTest
//
//  Created by user on 14-1-9.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import "VoiceController.h"
#include <string>
@implementation VoiceController
static void AQInputCallback (void                   * inUserData,
                             AudioQueueRef          inAudioQueue,
                             AudioQueueBufferRef    inBuffer,
                             const AudioTimeStamp   * inStartTime,
                             unsigned long          inNumPackets,
                             const AudioStreamPacketDescription * inPacketDesc)
{
    VoiceController * THIS = (__bridge  VoiceController *) inUserData;
    if (inNumPackets > 0)
    {
        //
        THIS->_voiceSession->SendData((char*)inBuffer->mAudioData, 320);
    }
    
    AudioQueueEnqueueBuffer(inAudioQueue, inBuffer, 0, NULL);
    
}

static VoiceController *sharedInstance= nil;
+(VoiceController *) sharedInstance

{
    if(!sharedInstance)
        
    {
        sharedInstance = [[self alloc] init];
    }
    return sharedInstance;
}

static void OnServerEvent(int type,void* param)
{
    //
    @autoreleasepool{
            NSLog(@"server event type is %d ",type);
        if(type ==26)//server disconnect
        {
            //
            [[VoiceController sharedInstance] stopRecord];
            [[VoiceController sharedInstance] stopPlay];
            
            [[NSNotificationCenter defaultCenter] postNotificationName:@"ServerDisconnect" object:nil userInfo:nil];
            //
            
        }
        if(type == 28)//server error
        {
            
            
        }
        if(type == 22)//member join
        {
            voiceClient* t=(voiceClient*)param;
           
            NSString * p = [NSString stringWithFormat:@"%d",t->player_id];
            NSString* r = [NSString stringWithFormat:@"%d",t->room_id];
            NSDictionary* dic= [NSDictionary dictionaryWithObjectsAndKeys:p,@"playerId",r,@"roomId",nil];
            
            NSLog(@"member join\n");
            [[NSNotificationCenter defaultCenter] postNotificationName:@"EnterRoom" object:nil userInfo:dic];
        }
        if(type ==24)//member left
        {
           //
            voiceClient* t=(voiceClient*)param;
            
            NSString * p = [NSString stringWithFormat:@"%d",t->player_id];
            NSString* r = [NSString stringWithFormat:@"%d",t->room_id];
            NSDictionary* dic= [NSDictionary dictionaryWithObjectsAndKeys:p,@"playerId",r,@"roomId",nil];
            [[NSNotificationCenter defaultCenter] postNotificationName:@"LeaveRoom" object:nil userInfo:dic];
            //
        }
        if(type == 30)
        {
            voiceClient* t=(voiceClient*)param;
            
            NSString * p = [NSString stringWithFormat:@"%d",t->player_id];
            NSDictionary* dic= [NSDictionary dictionaryWithObjectsAndKeys:p,@"playerId",nil];
            [[NSNotificationCenter defaultCenter] postNotificationName:@"BeginTalk" object:nil userInfo:dic];
        }
        if(type == 32)
        {
            voiceClient* t=(voiceClient*)param;
        
            NSString * p = [NSString stringWithFormat:@"%d",t->player_id];
            NSDictionary* dic= [NSDictionary dictionaryWithObjectsAndKeys:p,@"playerId",nil];
            [[NSNotificationCenter defaultCenter] postNotificationName:@"StopTalk" object:nil userInfo:dic];
        }
        
    }
    
    //
}
static void BufferCallback(void *inUserData,AudioQueueRef inAQ,
                           
                           AudioQueueBufferRef buffer)
{
    //
    VoiceController * THIS = (__bridge  VoiceController *) inUserData;
    
    
    char temp[320];
    
    THIS->_voiceSession->GetData(temp);
    //THIS->_playStream->PopupStream((char*)buffer->mAudioData, 1600);
    memcpy(buffer->mAudioData, temp, 320);
    buffer->mAudioDataByteSize=320;
    AudioQueueEnqueueBuffer(inAQ, buffer, 0, NULL);
    //
}
-(int)stopPlay
{
    //
    
    AudioQueueStop(_playQueue,false);
    for(int i=0;i<2;i++)
    {
        AudioQueueFreeBuffer(_playQueue, _playBuffers[i]);
    }
    AudioQueueDispose(_playQueue, false);
    return 0;
    //
}
-(int)setAudioRecv:(int)playerId redv:(bool)recv
{
    int res = _voiceSession->SetRecvData(playerId,recv);
    if(res == 0)
    {
        
    }
    return res;
}
-(int)stopRecord
{
    //
    AudioQueueStop(_recordQueue,false);
    for(int i=0;i<2;i++){
        AudioQueueFreeBuffer(_recordQueue, _recordBuffers[i]);
    }
    AudioQueueDispose(_recordQueue, false);
    return 0;
    //
}
-(int)creaetRoom:(int)roomId
{
    //
    int res = _voiceSession->CreateRoom(roomId);
    if(res == 0)
    {
        //
        NSLog(@"create room succeed");
        //
    }
    return res;
    //
}
-(int)getMemberList:(int)roomId array:(NSMutableArray*)array
{
    vector<int> vs ;
    int res = _voiceSession->GetRoomMember(roomId,&vs);
    if(res == 0)
        for(int i=0;i<vs.size();i++)
        {
            NSString* ss = [NSString stringWithFormat:@"%d",vs.at(i)];
            [array addObject:ss];
        }
    return res;
}
-(int)getRoomList:(NSMutableArray*)array
{
    //
    vector<int> vs ;
    int res = _voiceSession->GetRoomList(&vs);
    if(res == 0)
        for(int i=0;i<vs.size();i++)
        {
            NSString* ss = [NSString stringWithFormat:@"%d",vs.at(i)];
            [array addObject:ss];
        }
    return res;
    //
}
-(int)startPlay
{
    
    //
    _play=1;
    
    AudioQueueNewOutput(&_recordFormat,
                        BufferCallback,
                        (__bridge void*)self /* userData */,
                       CFRunLoopGetCurrent(), kCFRunLoopCommonModes,
                        0,
                        &_playQueue);
    
    // put samples in the buffer
    
    for (int i=0;i<2;i++)
    {
        AudioQueueAllocateBuffer(_playQueue, 320, &_playBuffers[i]);
        AudioQueueEnqueueBuffer(_playQueue, _playBuffers[i], 0, NULL);
        BufferCallback((__bridge void*)self, _playQueue, _playBuffers[i]);
        
    }
    //
    AudioQueueStart(_playQueue, NULL);
    
    //
    return 0;
    //
    
    //
}
-(int)resetServer
{
    //
    
    _voiceSession->Reset();
    return 0;
    //
}
-(int)connectServer:(NSString*)serverAdress port:(short)port
{
    int res=0;
    //if((res = _voiceSession->Init("42.121.19.238", 9009))!=0)
        //            if((res = _voiceSession->Init("192.168.128.130", 9009))!=0)
    if((res = _voiceSession->Init([serverAdress cStringUsingEncoding:NSUTF8StringEncoding], port))!=0)
        
    {
        NSLog(@"init voice session failed,error code is %d",res);
        return -1;
    }
    return res;
}
-(id)init;
{
    //
    self = [super init];
    _play = 0;
    if(self != nil )
    {
        int res =0;
        _voiceSession = new VoiceSession();
        
        _voiceSession->SetServerEvent(OnServerEvent);
        [self SetupAudioFormat];
        //
        AudioSessionInitialize(NULL, NULL, NULL, NULL);
        UInt32 category = kAudioSessionCategory_PlayAndRecord;
        res=0;
        if((res =AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category)) !=kAudioSessionNoError ){
            NSLog(@"AudioSessionSetProperty set error ,code is %d",res);
            return nil;
        }
        //
        UInt32 defaultToSpeaker = 1;
        if((res= AudioSessionSetProperty (kAudioSessionProperty_OverrideCategoryDefaultToSpeaker, sizeof (defaultToSpeaker), &defaultToSpeaker )) != kAudioSessionNoError)
        {
            return nil;
        }
    }
    return self;
}
-(int)beginTalk
{
    int res = _voiceSession->BeginTalk() ;
    return  res;
}
-(int)stopTalk
{
    int res = _voiceSession->StopTalk() ;
    return  res;
}
-(int )loginOut
{
   int res = _voiceSession->LoginOutServer() ;
    return  res;
}
-(int)loginServer:(int)playerId
{
    
    int res = _voiceSession->LoginServer(playerId);
    return res;
}
-(int)leaveRoom
{
    //
    int res = _voiceSession->LeaveRoom();
    return res;
    //
}
-(int)enterRoom:(int)roomId
{
    int res = _voiceSession->EnterRoom(roomId);
    return res;
}
-(int)getRoomMember:(NSMutableArray*)array
{
    //
    //
}
-(int)startRecord
{
    
    AudioQueueNewInput(
                       &_recordFormat,
                       AQInputCallback,
                       (__bridge void*) self /* userData */,
                         CFRunLoopGetCurrent(), kCFRunLoopCommonModes,
                        0 /* flags */, &_recordQueue);
    
    for (int i=0;i<2;i++)
    {
        AudioQueueAllocateBuffer(_recordQueue, 320, &_recordBuffers[i]);
        AudioQueueEnqueueBuffer(_recordQueue, _recordBuffers[i], 0, NULL);
    }
    //
    int status = AudioQueueStart(_recordQueue, NULL);
    return status;
}

-(void)SetupAudioFormat
{
	memset(&_recordFormat, 0, sizeof(_recordFormat));
    
    _recordFormat.mFormatID = kAudioFormatLinearPCM;
    _recordFormat.mSampleRate = 8000.0;
    _recordFormat.mBitsPerChannel = 16;
    _recordFormat.mChannelsPerFrame = 1;
    _recordFormat.mFramesPerPacket = 1;
    _recordFormat.mBytesPerFrame = (_recordFormat.mBitsPerChannel / 8) * _recordFormat.mChannelsPerFrame;
    _recordFormat.mBytesPerPacket = _recordFormat.mBytesPerFrame * _recordFormat.mFramesPerPacket;
    _recordFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	
}
@end
