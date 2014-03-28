//
//  SpeexCoder.h
//  AQS
//
//  Created by user on 14-1-7.
//  Copyright (c) 2014年 midfar.com. All rights reserved.
//

#ifndef __AQS__SpeexCoder__
#define __AQS__SpeexCoder__

//copy from speex_jitter_buffer.h(.c)
#include <speex/speex_jitter.h>
#include <speex/speex.h>
#include <speex/speex.h>
#include <speex/speex_jitter.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>


/** @defgroup SpeexJitter SpeexJitter: Adaptive jitter buffer specifically for Speex
 *  This is the jitter buffer that reorders UDP/RTP packets and adjusts the buffer size
 * to maintain good quality and low latency. This is a simplified version that works only
 * with Speex, but is much easier to use.
 *  @{
 */

/** Speex jitter-buffer state. Never use it directly! */
typedef struct SpeexJitter {
    SpeexBits current_packet;         /**< Current Speex packet */
    int valid_bits;                   /**< True if Speex bits are valid */
    JitterBuffer *packets;            /**< Generic jitter buffer state */
    void *dec;                        /**< Pointer to Speex decoder */
    spx_int32_t frame_size;           /**< Frame size of Speex decoder */
} SpeexJitter;

/** Initialise jitter buffer
 *
 * @param jitter State of the Speex jitter buffer
 * @param decoder Speex decoder to call
 * @param sampling_rate Sampling rate used by the decoder
 */
void speex_jitter_init(SpeexJitter *jitter, void *decoder, int sampling_rate);

/** Destroy jitter buffer */
void speex_jitter_destroy(SpeexJitter *jitter);

/** Put one packet into the jitter buffer */
void speex_jitter_put(SpeexJitter *jitter, char *packet, int len, int timestamp);

/** Get one packet from the jitter buffer */
void speex_jitter_get(SpeexJitter *jitter, spx_int16_t *out, int *start_offset);

/** Get pointer timestamp of jitter buffer */
int speex_jitter_get_pointer_timestamp(SpeexJitter *jitter);

#define SAMPLING_RATE 8000
#define FRAME_SIZE 160
////////////////////////////////////////////////////////////////////

class SanSpeexEnc
{
public:
    SanSpeexEnc();
    ~SanSpeexEnc();
    
    /**
     * @return 4 + 4 + encodedLen
     * @rawin  must be 320 length
     *  4+encodedLen |   timestamp  | encodedBuf
     * ————–+————–+————–
     *  4            |   4          | encodedLen
     */
    int encode(const short *rawin, char *encodedBufOutWithTimestampAndLen);
    
    int playbackEcho(const short *rawin, bool ok); //TODO: remember to sync
    
    void captureEcho(short *rawin);
    void captureEcho(short *rawin, short *playback);
    
private:
    SpeexPreprocessState *preprocess;
    void                 *enc_state;
    SpeexBits             enc_bits;
    int                   send_timestamp;
    short                 pcm[FRAME_SIZE], pcm2[FRAME_SIZE];
    SpeexEchoState       *echo_state;
    char                  encodedBuf[FRAME_SIZE*2];
    
};
class SanSpeexDec
{
public:
    SanSpeexDec();
    ~SanSpeexDec();
    
    void setSanSpeexEnc(SanSpeexEnc *ptr);
    void resetJitterBuffer();
    int put(const char *encodedBufWithTimestamp, int encodedLenWithTimestamp);
    int get(short *rawout);
    int afterPlayback(short *rawin, bool ok);
    
private:
    void               *dec_state;
    SpeexBits           dec_bits;
    short               pcm[FRAME_SIZE];
    SanSpeexEnc        *speexenc;
    SpeexJitter         jitter;
    int                 mostUpdatedTSatPut;
    bool                firsttimecalling_get;
    
};
#endif /* defined(__AQS__SpeexCoder__) */