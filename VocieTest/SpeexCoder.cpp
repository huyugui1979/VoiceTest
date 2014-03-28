//
//  SpeexCoder.cpp
//  AQS
//
//  Created by user on 14-1-7.
//  Copyright (c) 2014年 midfar.com. All rights reserved.
//

#include <stdio.h>
#include <limits.h>
#include "SpeexCoder.h"
#ifndef NULL
#define NULL 0
#endif
//#define ENABLE_AEC
//#ifdef ENABLE_AEC
//#define USE_AEC_CAPTURE          /* will print a lot of speex warning */
//#endif
#define NEED_DUMP
#include <stdio.h>
FILE *file = 0;
char msg[128];
void debugPrint(const char *msg)
{
#ifdef NEED_DUMP
    if (file == 0)
        file = fopen("dump.txt", "a");
    fprintf(file, msg);
#endif
}

void closeDebugPrint()
{
#ifdef NEED_DUMP
    if (file)
        fclose(file);
    file = 0;
#endif
}

#define ECHOTAILSIZE  25
short psSpeaker[160 * ECHOTAILSIZE * 10];
void speex_jitter_init(SpeexJitter *jitter, void *decoder, int sampling_rate)
{
    jitter->dec = decoder;
    speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &jitter->frame_size);
    
    jitter->packets = jitter_buffer_init(jitter->frame_size);
    
    speex_bits_init(&jitter->current_packet);
    jitter->valid_bits = 0;
}
void speex_jitter_reset(SpeexJitter *jitter)
{
    jitter_buffer_reset(jitter->packets);
}
void speex_jitter_destroy(SpeexJitter *jitter)
{
    jitter_buffer_destroy(jitter->packets);
    speex_bits_destroy(&jitter->current_packet);
}

void speex_jitter_put(SpeexJitter *jitter, char *packet, int len, int timestamp)
{
    JitterBufferPacket p;
    p.data = packet;
    p.len = len;
    p.timestamp = timestamp;
    p.span = jitter->frame_size;
    jitter_buffer_put(jitter->packets, &p);
    //sprintf(msg, "put %d\n", timestamp);
   
}

void speex_jitter_get(SpeexJitter *jitter, spx_int16_t *out, int *current_timestamp)
{
    int i;
    int ret;
    spx_int32_t activity;
    int bufferCount = 0;
    JitterBufferPacket packet;
    //char data[40960];
    //packet.data = data;
    //packet.len = 40960;
    packet.data = reinterpret_cast<char *>(psSpeaker);
    packet.len = 160 * ECHOTAILSIZE * 10;
    
    if (jitter->valid_bits)
    {
        /* Try decoding last received packet */
        ret = speex_decode_int(jitter->dec, &jitter->current_packet, out);
        if (ret == 0)
        {
            jitter_buffer_tick(jitter->packets);
            return;
        } else {
            jitter->valid_bits = 0;
        }
    }
    
    if (current_timestamp)
        ret = jitter_buffer_get(jitter->packets, &packet, jitter->frame_size, current_timestamp);
    else
        ret = jitter_buffer_get(jitter->packets, &packet, jitter->frame_size, NULL);
    
    if (ret == JITTER_BUFFER_MISSING)
    {
        /* No packet found */
        speex_decode_int(jitter->dec, NULL, out);
    }
    else {
        speex_bits_read_from(&jitter->current_packet, packet.data, packet.len);
        /* Decode packet */
        ret = speex_decode_int(jitter->dec, &jitter->current_packet, out);
        if (ret == 0)
        {
            jitter->valid_bits = 1;
            printf("decode correct\n");
        } else {
            /* Error while decoding */
            for (i=0;i<jitter->frame_size;i++)
                out[i]=0;
            printf("decode error\n");
        }
    }
//    speex_decoder_ctl(jitter->dec, SPEEX_GET_ACTIVITY, &activity);
//    if (activity < 30)
//    {
//        jitter_buffer_update_delay(jitter->packets, &packet, NULL);
//    }
    jitter_buffer_tick(jitter->packets);
    //ret = jitter_buffer_ctl(jitter->packets, JITTER_BUFFER_GET_AVALIABLE_COUNT, &bufferCount);
    //sprintf(msg, “   get %d bufferCount=%d\n”, speex_jitter_get_pointer_timestamp(jitter), bufferCount);
    //debugPrint(msg);
}

int speex_jitter_get_pointer_timestamp(SpeexJitter *jitter)
{
    return jitter_buffer_get_pointer_timestamp(jitter->packets);
}
//===========================================================
SanSpeexEnc::SanSpeexEnc()
:preprocess(0),
enc_state(0),
enc_bits(),
send_timestamp(0),
echo_state(0)
{
    for (int i=0; i<FRAME_SIZE; i++)
        pcm[i] = 0;
    
    int tmp;
    float tmpf;
    
    enc_state = speex_encoder_init(&speex_nb_mode);
    tmp = 3; //3,4,7,8,9,10 are not working
    speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &tmp);
    tmp = 0;
    speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &tmp);
    tmp = 1;
    speex_encoder_ctl(enc_state, SPEEX_SET_VBR_QUALITY, &tmp);
    
    tmp = 1;
    speex_encoder_ctl(enc_state, SPEEX_SET_VAD, &tmp);
    tmp = 1;
    speex_encoder_ctl(enc_state, SPEEX_SET_DTX, &tmp);
    tmp = 4;
    speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &tmp);
    speex_bits_init(&enc_bits);
    
//   echo_state = speex_echo_state_init(FRAME_SIZE, ECHOTAILSIZE*FRAME_SIZE);
//    tmp = SAMPLING_RATE;
//    speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);
//    
    preprocess = speex_preprocess_state_init(FRAME_SIZE, SAMPLING_RATE);
////    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
////
    int denoise = 1;
    int noiseSuppress = 5;
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //降噪
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪声的dB
    
    
    int agc = 1;
    int q=16000;
    //actually default is 8000(0,32768),here make it louder for voice is not loudy enough by default. 8000
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_AGC, &agc);//增益
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_AGC_LEVEL,&q);
    int vad = 1;
    int vadProbStart = 80;
    int vadProbContinue = 65;
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_VAD, &vad); //静音检测
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_PROB_START , &vadProbStart); //Set probability required for the VAD to go from silence to voice
    speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); //Set probability required for the VAD to stay in the voice state (integer percent)
}

SanSpeexEnc::~SanSpeexEnc()
{
    speex_encoder_destroy(enc_state); //–Destroy the encoder state
    speex_echo_state_destroy(echo_state);
    speex_preprocess_state_destroy(preprocess);
    speex_bits_destroy(&enc_bits); //–Destroy the bit-packing struct
    closeDebugPrint();
}

/**
 * @return 4 + 4 + encodedLen
 * @rawin  must be 320 length
 *  4+encodedLen |   timestamp  | encodedBuf
 * ————–+————–+————–
 *  4            |   4          | encodedLen
 */
int SanSpeexEnc::encode(const short *rawin, char *encodedBufOutWithTimestampAndLen)
{
#ifdef ENABLE_AEC
    /* Perform echo cancellation */
#ifdef USE_AEC_CAPTURE
    speex_echo_capture(echo_state, rawin, pcm2);
#else
    speex_echo_cancellation(echo_state, rawin, psSpeaker, pcm2);
#endif
    for (int i=0;i<FRAME_SIZE;i++)
        pcm[i] = pcm2[i];
#else
    for (int i=0;i<FRAME_SIZE;i++)
        pcm[i] = rawin[i];
#endif
    
    speex_bits_reset(&enc_bits);
    
    /* Apply noise/echo suppression */
    bool isSpeech = speex_preprocess_run(preprocess, pcm);
    /* Encode */
    int needTransmit = speex_encode_int(enc_state, pcm, &enc_bits);
    int packetSize = 0;
    if (/*isSpeech && */needTransmit)
    {
        packetSize = speex_bits_write(&enc_bits, encodedBufOutWithTimestampAndLen+4, FRAME_SIZE*2);
        
        ((int*)encodedBufOutWithTimestampAndLen)[0] = send_timestamp;
     //   printf("packetSize=%d, timestamp=%d", packetSize, send_timestamp);
    }
    
    send_timestamp += FRAME_SIZE;
    
    if (send_timestamp >= INT_MAX)
        send_timestamp = 0;
    
    if (packetSize > 0)
        return packetSize+4;
    else
        return 0;
}

int SanSpeexEnc::playbackEcho(const short *rawin, bool ok)
{
#ifdef ENABLE_AEC
    //TODO : sync
    /* Put frame into playback buffer */
#ifdef USE_AEC_CAPTURE
    if (!ok)
        speex_echo_state_reset(echo_state);
    speex_echo_playback(echo_state, rawin);
#endif
#endif
    return 1;
}

void SanSpeexEnc::captureEcho(short *rawin)
{
#ifdef ENABLE_AEC
#ifdef USE_AEC_CAPTURE
    speex_echo_capture(echo_state, rawin, pcm2);
    for (int i=0; i<FRAME_SIZE; i++)
        rawin[i] = pcm2[i];
    
    speex_bits_reset(&enc_bits);
    speex_preprocess_run(preprocess, rawin);
#endif
#endif
}

void SanSpeexEnc::captureEcho(short *rawin, short *playback)
{
#ifdef ENABLE_AEC
    speex_echo_cancellation(echo_state, rawin, playback, pcm2);
    speex_preprocess_run(preprocess, pcm2);
    for (int i=0; i<FRAME_SIZE; i++)
        rawin[i] = pcm2[i];
#endif
}

//==================================================
SanSpeexDec::SanSpeexDec()
:dec_state(0),
dec_bits(),
speexenc(0),
jitter(),
mostUpdatedTSatPut(0),
firsttimecalling_get(true)
{
    for (int i=0; i<FRAME_SIZE; i++)
        pcm[i] = 0;
    int tmp;
    
    dec_state = speex_decoder_init(&speex_nb_mode);
   //tmp = 1;
   // speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &tmp);
    speex_bits_init(&dec_bits);
    speex_jitter_init(&jitter, dec_state, SAMPLING_RATE);
}

SanSpeexDec::~SanSpeexDec()
{
    speex_jitter_destroy(&jitter);
    
    //–Destroy the decoder state
    speex_decoder_destroy(dec_state);
    
    //–Destroy the bit-stream truct
    speex_bits_destroy(&dec_bits);
}
void SanSpeexDec::resetJitterBuffer()
{
    //
    speex_jitter_reset(&jitter);
    //
}
void SanSpeexDec::setSanSpeexEnc(SanSpeexEnc *ptr)
{
    speexenc = ptr;
}

int SanSpeexDec::put(const char *encodedBufWithTimestamp, int encodedLenWithTimestamp)
{
    //buffer:
    //  timestamp | encodedBuf
    // ———–+——————————
    //   4        | encodedLenWithTimestamp – 4
    int success = 0;
    int recv_timestamp = ((int*)encodedBufWithTimestamp)[0];
    printf("recv_timestamp%d\n",recv_timestamp);
    mostUpdatedTSatPut = recv_timestamp;
    if (firsttimecalling_get)
        return 1;
    speex_jitter_put(&jitter, (char *)encodedBufWithTimestamp+4, encodedLenWithTimestamp-4, recv_timestamp);
    return 1;
}

int SanSpeexDec::get(short *rawout)
{
    if (firsttimecalling_get)
    {
        int ts = mostUpdatedTSatPut;
        firsttimecalling_get = false;
        speex_jitter_get(&jitter, rawout, &ts);
    }
    else
        speex_jitter_get(&jitter, rawout, 0);
    return 1;
}

int SanSpeexDec::afterPlayback(short *rawin, bool ok)
{
    if (speexenc)
        return speexenc->playbackEcho(rawin, ok);
    return 0;
}



