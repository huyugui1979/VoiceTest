//
//  VoiceSession.cpp
//  VocieTest
//
//  Created by user on 14-1-14.
//  Copyright (c) 2014年 user. All rights reserved.
//
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "VoiceSession.h"
#include "ServerError.h"
#include <string>
#include <signal.h>
#define FRAME_SIZE 160
//
void Mix(char* sourseFile,int number,char *objectFile)
{
    //归一化混音
    int const MAX=32767;
    int const MIN=-32768;
    
    double f=1;
    int output;
    int i = 0,j = 0;
    
    for (i=0;i<FRAME_SIZE;i++)
    {
        int temp=0;
        for (j=0;j<number;j++)
        {
            temp+=*(short*)(sourseFile+j*FRAME_SIZE*2+i*2);
        }
        output=(int)(temp*f);
        if (output>MAX)
        {
            f=(double)MAX/(double)(output);
            output=MAX;
        }
        if (output<MIN)
        {
            f=(double)MIN/(double)(output);
            output=MIN;
        }
        if (f<1)
        {
            f+=((double)1-f)/(double)32;
        }
        *(short*)(objectFile+i*2)=(short)output;
    }
}

int VoiceSession::GetData(char* data)
{
    //
    pthread_mutex_lock(&_decorder_mutex);
    int k = _clients.size();
    char*  da=new char[k*320];
    int index=0;
    //      std::for_each(this->_decCorders.begin(), _decCorders.end(), [&](const std::pair<int,SanSpeexDec*>& p){
    //        p.second->get((short*)(da+index*320));
    //          index++;
    //    });
    for(auto it=this->_clients.begin();it!=_clients.end();it++)
    {
        it->second->dec->get((short*)(da+index*320));
        index++;
    }
    //
    Mix(da,_clients.size(),data);
    delete da;
    //
    pthread_mutex_unlock(&_decorder_mutex);
    return 0;
    //
}

int VoiceSession::SendData(char* data,int len)
{
    //
    char outBuffer[1024];
    bzero(outBuffer,1024);
    
    //int outSize =_coder.Encoder((short*)data, outBuffer, FRAME_SIZE);
    int outSize = _enCoder.encode((const short*)data, outBuffer);
    
    if(outSize == 0)
    {
        printf("encode size is %d\n",outSize);
        return 0;
    }
    // printf("encode size is %d\n",outSize);
    char* temp = new char[outSize];
    memcpy(temp, outBuffer, outSize);
    //pthread_mutex_lock(&_s_mutex);
    this->_sendQueue.push(pair<int,char*>(outSize,temp));
    
    //pthread_cond_signal(&_s_cond);
    if(this->_sendQueue.size()==5)
    {
        unsigned char buffer[1024];
        bzero(buffer,1024);
        buffer[0]=1;
        buffer[1]=17;
        
        bcopy(&_playerId,buffer+2,4);
        bcopy(&_seq, buffer+6, 4);
        _seq++;
        char* count=(char*)(buffer+10);
        int totalSize =11;
        
        for(int i=0;i<5;i++)  {
            char* size = (char*)(buffer+totalSize);
            //
            pair<int,char*> data = _sendQueue.front();
            
            *size = data.first;
            totalSize++;
            memcpy(buffer+totalSize, data.second, data.first);
            totalSize +=data.first;
            delete data.second;
            _sendQueue.pop();
            *count = *count+1;
        }
        //crc check
        
        unsigned short Crc=0;
        for(int i=0; i<totalSize; i++){
            
            Crc = (Crc >> 8) ^ _crc16Table[(Crc & 0xFF) ^ buffer[i]];
        }
        memcpy((void*)(buffer+totalSize),&Crc,2);
        printf("send seq is %d,crc is %d,size is %d\n",_seq-1,Crc,totalSize+2);
        //
        totalSize+=2;
        //
         ushort s = totalSize;
         pthread_mutex_lock(&_socket_mutex);
        if(_clientStatus==2)
        {
               int  res = send(_sockfd,&s,2,0);
                if(res<0)
                {
                    Reset();
                     _serverEvent(26,NULL);
                    
                    goto end;
                }
               send(_sockfd,buffer,totalSize,0);
            if(res<0)
            {
                Reset();
                 _serverEvent(26,NULL);
                goto end;            }
        }
    end:pthread_mutex_unlock(&_socket_mutex);
    }
    //pthread_mutex_unlock(&_s_mutex);
    return 0;
    //
}

void * VoiceSession::ServerTimerProc(void *p)
{
    VoiceSession *v = (VoiceSession*)p;
    while(v->_clientStatus!=-1)
    {
        sleep(3);
        //
        char udpBuffer[1024];
        bzero(udpBuffer,1024);
        
        udpBuffer[0]=1;
        udpBuffer[1]=99;
        ushort s = 2;
        pthread_mutex_lock(&v->_socket_mutex);
        if(v->_clientStatus !=-1)
        {
        int res = send(v->_sockfd,&s,2,0);
            if(res<0)
            {
                v->Reset();
                v->_serverEvent(26,NULL);
                     
           }

        res =send(v->_sockfd,udpBuffer,2,0);
            if(res<0)
            {
                v->Reset();
                v->_serverEvent(26,NULL);
                
            }
        }
        pthread_mutex_unlock(&v->_socket_mutex);
        
        printf("send a timer\n");
    }
    return 0;
}

void VoiceSession::Reset(){
    //
    if(_clientStatus != -1)
    {
        close(_sockfd);
    }
    //
    
    _seq=0;
    _sockfd=0;
    _clientStatus=-1;
    auto it=_clients.begin();
    for(;it !=_clients.end();it++)
    {
        delete it->second;
    }
    _clients.clear();
   
    //
}
void * VoiceSession::RecvServerProc(void *p)
{
    //
    VoiceSession *v = (VoiceSession*)p;
    unsigned char udpBuffer[1024];
    while(v->_clientStatus != -1)
    {
        //
        bzero(udpBuffer, 1024);
        ushort ll = 0;
        int len = recv(v->_sockfd,&ll,2,MSG_WAITALL);
        if(len<=0)
        {
            v->Reset();
               
             v->_serverEvent(26,NULL);
    
            break;
        }
        int recvPos=0;
        if(len ==2)
        {
            
            len = recv(v->_sockfd, udpBuffer, ll, MSG_WAITALL);
            if(len<=0)
            {
                //
                v->Reset();

                v->_serverEvent(26,NULL);
                
                break;
            }
            if(len>0)
            {
                //
                char m=udpBuffer[0],s=udpBuffer[1];
                if(m ==1 && s == 100)
                {
                    //
                    v->_serverError = udpBuffer[2];
                    printf("recvive a server error,error is %d",v->_serverError);
                    v->Signal();
                    //
                }
                if(m == 1 && s==2) //check server
                {
                    
                    v->Signal();
                }
                if(m == 1 && s==4) //login server
                {
                    v->Signal();
                }
                if(m== 1 && s ==6)//login out
                {
                    v->Signal();
                }
                if(m==1 && s == 8)//create room
                {
                  
                    v->Signal() ;
                }
                if(m==1 && s == 26)//destroy room
                {
                    v->Signal() ;
                }
                if(m ==1 && s==28)
                {
                    v->Signal();
                }
                if(m==1 && s == 10)//enter room
                {
                    v->Signal() ;
                }
                if(m==1 && s==12)//left room
                {
                    v->Signal() ;
                }
                if(m == 1 && s ==17)
                {
                    unsigned short Crc=0;
                    for(int i=0; i<len-2; i++){
                        Crc = (Crc >> 8) ^ v->_crc16Table[(Crc & 0xFF) ^ udpBuffer[i]];
                        
                    }
                    //
                   int* playerId = (int*)(udpBuffer+2);
                   if( v->_clients.count(*playerId)==0)//
                       continue;
                    SanSpeexDec* dec = v->_clients.at(*playerId)->dec;
                    char* count = (char*)(udpBuffer+10);
                    int total=11;
                    int* seq = (int*)(udpBuffer+6);
                    unsigned short temp =*(unsigned short*)(udpBuffer+len-2);
                    if(Crc != temp)
                    {
                        printf("crc error,recv crc is %d,seq is %d, len is %d,ll is %d\n",temp,*seq,len,ll);
                        
                        continue;
                    }
                    
                    printf("recv seq is %d,crc is%d,size is %d\n",*seq,Crc,len);
                    for(int i=0;i<*count;i++)
                    {
                        char* len =(char*)(udpBuffer+total);
                        total++;
                        pthread_mutex_lock(&v->_decorder_mutex);
                        
                        dec->put((const char*)udpBuffer+total, *len);
                        pthread_mutex_unlock(&v->_decorder_mutex);
                        total=total+*len;
                        
                    }
                              
                    //
                }
                if(m==1 && s ==14) //get room member
                {
                    char* t = (char*)(udpBuffer+2);
                   
                    int len = *(ushort*)t;
                    printf("get %d player\n",len);
                    t=t+2;
                    for(int i=0;i<len;i++)
                    {
                        voiceClient* c=new voiceClient();;
                        c->room_id = v->_roomId;
                        c->player_id =*(int*)t;
                        t=t+4;
                        c->client_ip = *(int*)t;
                        t=t+4;
                        c->udp_port = *(short*)t;
                        t=t+2;
                        SanSpeexDec* dec = new SanSpeexDec();
                        c->dec=dec;
                        v->_clients[c->player_id]=c;
                        
                    }
                  
                    v->Signal() ;
                    
                }
                if(m==1 && s ==16)//room list
                {
                    vector<int>* room_list = new vector<int>();
                    char* t = (char*)(udpBuffer+2);
                    ushort count = *(ushort*)t;
                    t=t+2;
                    for(int i =0;i<count;i++)
                    {
                        room_list->push_back(*(int*)(t));
                        t=t+4;
                    }
                    v->_serverResult=(void*)room_list;
                    v->Signal() ;
                    
                }
                if(m==1 && s==22) //member join echo 
                {
                    //
                    
                    
                     char* t = (char*)(udpBuffer+2);
                    voiceClient* c=new voiceClient();;
                    c->player_id =*(uint*)t;
                    t=t+4;
                
                    c->dec=new SanSpeexDec();
                    c->room_id=v->_roomId;
                    v->_clients[c->player_id]=c;

                    v->_serverEvent(22,c);
                    printf("playerId %d get in",c->player_id);
                    //
  
                }
                if(m==1 && s==24) //member left echo
                {
                    
                    int* playerId = (int*)(udpBuffer+2);
                    voiceClient* c = v->_clients.at(*playerId);
                    v->_clients.erase(*playerId);
                  
                    v->_serverEvent(24,c );
                    delete c;
                    //
    
                }
                if(m==1 && s==30)
                {
                    v->Signal();
                }
                if(m==1 && s==32)
                {
                    v->Signal();
                }
                if(m==2 && s == 30)//begin talk echo
                {
                    //
                    int* playerId = (int*)(udpBuffer+2);
                    voiceClient* c = v->_clients.at(*playerId);
                    v->_serverEvent(30,c );
                    //
                }
                if(m==2 && s==32)//stop talk echo
                {
                    //
                    int* playerId = (int*)(udpBuffer+2);
                    voiceClient* c = v->_clients.at(*playerId);
                    v->_serverEvent(32,c );
                    //
                }
                               //
            }
        }
        //
    }
}
void VoiceSession::Signal()
{
    //pthread_mutex_lock(&_mutex);
    pthread_cond_signal(&_cond);
   // pthread_mutex_unlock(&_mutex);
    
}
int VoiceSession::Wait(int seconds)
{
    //
    pthread_mutex_lock(&_mutex);
    struct timespec tv;
    tv.tv_sec = time(0) + seconds;
    tv.tv_nsec = 0;
    
    int ret = pthread_cond_timedwait( &_cond , &_mutex ,&tv );
    
    pthread_mutex_unlock(&_mutex);
    if(ret ==ETIMEDOUT)
    {
        printf("wait time out\n");
        return -1;
    }
    else
        return 0;
    //
}
int VoiceSession::SendDataToServer(char* buffer, int len)
{
    int res=-1;
    
    ushort s = len;
    pthread_mutex_lock(&_socket_mutex);
    if(_clientStatus !=-1)
    {
        int res = send(_sockfd,&s,2,0);
        if(res<0)
        {
            Reset();
            _serverEvent(26,NULL);
            
        }
        res = send(_sockfd,buffer,len,0);
        if(res<0)
        {
            Reset();
            _serverEvent(26,NULL);
            
        }
    }
    pthread_mutex_unlock(&_socket_mutex);
    
    if(this->Wait(100)!= -1)
    {
        if(this->_serverError ==0)
            res =0;
        else
        {
            printf("recve server error,error is %d",this->_serverError);
            res =this->_serverError;
            this->_serverError =0;
        }
        
    }else{
        res =TIME_OUT;
    }
    
    return res;
}

int VoiceSession::SetRecvData(int playerId,bool recv)
{
    if(_clientStatus != 2)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==1)
            return HAVE_LOGIN;
    }
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=27;
    
    bcopy(&playerId,sendBuffer+2,4);
    bcopy(&recv,sendBuffer+6,1);
    int res = SendDataToServer(sendBuffer,7);
    if(res == 0)
    {
       
        
    }
    return res;

}
int VoiceSession::BeginTalk()
{
    //
    if(_clientStatus != 2)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==1)
            return HAVE_LOGIN;
    }
    //
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=29;
    
   
    int res = SendDataToServer(sendBuffer,2);
    if(res == 0)
    {
       
    }
    return res;

    //
}
int VoiceSession::StopTalk()
{
    //
    if(_clientStatus != 2)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==1)
            return HAVE_LOGIN;
    }
    //
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=31;
    
    int res = SendDataToServer(sendBuffer,2);
    if(res == 0)
    {
        
    }
    return res;
    //
}
int VoiceSession::LeaveRoom()
{
    if(_clientStatus != 2)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==1)
            return HAVE_LOGIN;
    }
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=11;
    bcopy(&_roomId, sendBuffer+2,4);
    int res = SendDataToServer(sendBuffer,6
                               );
    if(res == 0)
    {
        _roomId=0;
        _clientStatus=1;
        //
        auto it=_clients.begin();
        for(;it !=_clients.end();it++)
        {
            delete it->second;
        }
        _clients.clear();
    }
    return res;
    
}
int VoiceSession::EnterRoom(int roomId)
{
    if(_clientStatus != 1)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=9;
    
    bcopy(&roomId,sendBuffer+2,4);
    int res = SendDataToServer(sendBuffer,6);
    if(res == 0)
    {
        _roomId= roomId;
        _clientStatus=2;
    }
    return res;
}
int VoiceSession::GetRoomMember(int roomId,vector<int>* s)
{
    //
    if(_clientStatus != 2)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==1)
            return NO_JOIN_ROOM;
    }
    int len=6;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=13;
    bcopy(&roomId,sendBuffer+2,4);
    int res = SendDataToServer(sendBuffer,len);
    if(res==0)
    {
        auto it = _clients.begin();
        for(;it !=_clients.end();it++)
        {
            s->push_back(it->first);
            //向客户打洞
//            struct sockaddr_in udp_sock;
//            int addrlen=sizeof(struct sockaddr_in);
//            bzero(&udp_sock,addrlen);
//            udp_sock.sin_family=AF_INET;
//            //_serverStruct.sin_addr.s_addr=inet_addr());
//            udp_sock.sin_addr.s_addr=htonl(it->second->client_ip);
//          
//            udp_sock.sin_port=htons(it->second->udp_port);
//            char s =1;
//            sendto(_udpfd, &s, 1, 0, (const struct sockaddr*)&udp_sock, addrlen);
//            //
//            char buffer[1024];
//            memset(buffer,0,1024);
//            *(char*)(buffer)=1;
//            *(char*)(buffer+1)=31;
//            *(int*)(buffer+2)=it->second->player_id;
//            short ss =6;
//            send(_sockfd,&ss,2,0);
//            send(_sockfd,buffer,ss,0);
            //
        }
    }
    return res;
    
    //
}
int VoiceSession::GetRoomList(vector<int>* s)
{
    //
    if(_clientStatus != 1)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    int len=2;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=15;
    
    int res = SendDataToServer(sendBuffer,len);
    if(res ==0)
    {
        vector<int>* roomList = (vector<int>*)_serverResult;
        for(int i=0;i<roomList->size();i++)
        {
            s->push_back(roomList->at(i));
        }
        delete roomList;
    }
    return res;
    //
}
int VoiceSession::DestroyRoom(int roomId)
{
    //
    if(_clientStatus != 1)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    int len=6;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=25;
    *(int*)(sendBuffer+2)=roomId;
    int res = SendDataToServer(sendBuffer,len);
    if(res ==0)
    {
        //
        
        //
    }
    return res;

    //
}

int VoiceSession::CreateRoom(int roomId)
{
    if(_clientStatus != 1)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
           return HAVE_JOIN_ROOM;
    }
    
    int len=6;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=7;
    *(int*)(sendBuffer+2)=roomId;
    int res = SendDataToServer(sendBuffer,len);
    if(res ==0)
    {
       
    }
    return res;
}

int   VoiceSession::LoginOutServer()
{
    if(_clientStatus != 1)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    int len=2;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=9;
    
    int res = SendDataToServer(sendBuffer,len);
    _clientStatus=0;
    return res;
    
}
int VoiceSession::LoginServer(int playerId)
{
    //
    if(_clientStatus != 0)
    {
        if(_clientStatus ==-1)
            return NOT_CONNECT;
        if(_clientStatus ==0)
            return NOT_LOGIN;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    int len=8;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=3;
    bcopy(&playerId, sendBuffer+2, 4);
    _playerId=playerId;
    bcopy(&_udpPort,sendBuffer+6,2);
    
    int res = SendDataToServer(sendBuffer,len);
    pthread_t tid ;
    if(res == 0)
    {
        _clientStatus=1;
        pthread_create(&tid, NULL, ServerTimerProc, this);
    }
    return res;
}
int VoiceSession::InitSockStruct(const char* address,short port)
{
    if(_clientStatus != -1)
    {
        if(_clientStatus ==1)
            return HAVE_LOGIN;
        if(_clientStatus ==0)
            return HAVE_CONNECT;
        if (_clientStatus ==2)
            return HAVE_JOIN_ROOM;
    }
    socklen_t addrlen;
    setuid(getuid());
    addrlen=sizeof(struct sockaddr_in);
    struct sockaddr_in  _serverStruct;
    bzero(&_serverStruct,addrlen);
    _serverStruct.sin_family=AF_INET;
    _serverStruct.sin_addr.s_addr=inet_addr(address);
    //cli_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    _serverStruct.sin_port=htons(port);
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == _sockfd) {
        printf("Failed to create socket\n");
        return CONNECT_FAILED;
    }
    int  set = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_NOSIGPIPE, (void  *)&set, sizeof(int));
    int res = connect(_sockfd,(const struct sockaddr*)&_serverStruct,addrlen);
    if(res <0)
    {
        
        return CONNECT_FAILED;
    }
    _clientStatus=0;
    return 0;
}

void VoiceSession::initCrc()
{
    
    unsigned int i,j;
    unsigned short Crc;
    for (i = 0; i < 256; i++){
        Crc = i;
        for (j = 0; j < 8; j++){
            if(Crc & 0x1)
                Crc = (Crc >> 1) ^ 0xA001;
            else
                Crc >>= 1;
        }
        _crc16Table[i] = Crc;
    }
}

int VoiceSession::Init(const char* address,short port)
{
    //


  
    int res=0;
    //
    res = InitSockStruct(address,port);
    if(res==0)
    {
        pthread_t tid ;
        pthread_create(&tid, NULL, RecvServerProc, this);
    }
    return res;
    //
}

int VoiceSession::CheckServer()
{
    //
    int len=2;
    char sendBuffer[256];
    bzero(sendBuffer,sizeof(sendBuffer));
    sendBuffer[0]=1;
    sendBuffer[1]=1;
    int res = SendDataToServer(sendBuffer,len);
    return res;
    //
}
VoiceSession::VoiceSession()
{
    _clientStatus=-1;
   
  
    _voiceQuality=3;
    _sockfd = 0;
   
    _serverError=0;
    initCrc();
    pthread_cond_init( &_cond , NULL);
    pthread_mutex_init(&_mutex, NULL);
    pthread_mutex_init(&_decorder_mutex,NULL);
    pthread_mutex_init(&_socket_mutex,NULL
                       );
  
    //
    
}
VoiceSession::~VoiceSession()
{
    
    pthread_mutex_destroy(&_mutex);
    pthread_mutex_destroy(&_decorder_mutex);
    pthread_mutex_destroy(&_socket_mutex);
   
    pthread_cond_destroy(&_cond);
}