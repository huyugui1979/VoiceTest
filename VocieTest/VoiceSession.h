//
//  VoiceSession.h
//  VocieTest
//
//  Created by user on 14-1-14.
//  Copyright (c) 2014年 user. All rights reserved.
//

#ifndef __VocieTest__VoiceSession__
#define __VocieTest__VoiceSession__
#include "SpeexCoder.h"
#include "voiceClient.h"
#include <map>
#include <vector>
#include <queue>
using namespace std;
class VoiceSession
{
  //
    typedef void (*OnServerEvent)(int type,void* param);
public:
    VoiceSession();
    ~VoiceSession();
    int Init(const char* addess,short port,int timeout);
    void Reset();
    int LoginServer(int playerId);//m=1s=3
    int LoginOutServer();//m=1,s=7
    int CreateRoom(int roomId);//m=1,s=9
    int DestroyRoom(int roomId);//m=1,s=25
    int EnterRoom(int roomId);//m=1,s=11
    int LeaveRoom();
    int GetRoomList(vector<int>* vs);//m=1,s=13
    int SendData(char* data,int len);//m=1,s=17
    int SetRecvData(int playerId,bool recv);//m=1,s=27;
    int BeginTalk();//m=1,s=29;
    int StopTalk();//m=1,s=31;
    int GetData(char* data);
   
    int GetRoomMember(int roomId,vector<int>* vs);//m=1,s=15
    int SetServerEvent(OnServerEvent event)
    {
        _serverEvent=event;
    }
private:
   
    void Signal();
    void initCrc();
    int  Wait(int seconds);
    int CheckServer();
    int InitSockStruct(const char* address,short port,int timeout);
   
    static void * RecvServerProc(void *p);
    static void * ServerTimerProc(void *p);
    int SendDataToServer(char* buffer, int len);
private:
    OnServerEvent                 _serverEvent;
    queue<pair<int,char*>>        _sendQueue;
    map<int,voiceClient*>         _clients;
  
    int _sockfd;
    int _playerId;
    int _audioBufferCount;
    int _roomId;
    int _seq;
    pthread_cond_t   _cond;
    
    pthread_mutex_t  _mutex;
    pthread_mutex_t  _decorder_mutex;
    pthread_mutex_t  _socket_mutex;
  
    unsigned short   _crc16Table[256];
    SanSpeexEnc                 _enCoder;
   
   
    int                         _clientStatus;
    char                        _serverError;
    short                       _udpPort;
    int                         _udpfd;
    int                         _voiceQuality;
    void*                       _serverResult;
    
   
  //
};
#endif /* defined(__VocieTest__VoiceSession__) */
