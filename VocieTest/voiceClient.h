//
//  voiceClient.h
//  VocieTest
//
//  Created by hyg on 14-2-25.
//  Copyright (c) 2014年 user. All rights reserved.
//

#ifndef __VocieTest__voiceClient__
#define __VocieTest__voiceClient__

#include <iostream>
#include "SpeexCoder.h"
class voiceClient
{
    //
   
public:
    ~voiceClient()
    {
        delete dec;
    }
    uint player_id;
    int client_ip;
    short udp_port;
    int room_id;
    SanSpeexDec* dec;
    //
};
#endif /* defined(__VocieTest__voiceClient__) */
