/*!
\file       rtp__mod.h
\brief      rtmp module

 ----history----
\author     chengzhiyong
\date       2007-03-16
\version    0.01
\desc       create

$Author: chengzhiyong $
$Id: http.h,v 1.14 2008-09-02 08:27:27 chengzhiyong Exp $
*/

#ifndef _RTP_H
#define _RTP_H
#pragma pack(1)
#ifdef __cplusplus
extern "C" {
#endif 

#define RTP_OUTSTRM_REV_HEAD_LEN    16

#define RTP_TRUE    1 
#define RTP_FALSE   0 

#define RTP_FAILED(rResult)     (rResult != RTP_OK)
#define RTP_SUCCED(rResult)     (rResult == RTP_OK)
#define RTP_OK      0
#define RTP_FAIL    1

enum 
{
    RTP_STRM_HPMP, 
    RTP_STRM_RTP, 
}; 

typedef void (* ON_RTP_ENCODE_OPUT)(void * puserdata, unsigned char * data, long len); 

void *  RTP_CreateEncodeChl(int strmmode); 
void    RTP_DeleteEncodeChl(void * rtp); 
void    RTP_SetEncodeOputHook(void * rtp, ON_RTP_ENCODE_OPUT pfnRtpEncodeOput, void * puserdata); 
void    RTP_EncodeInputVideo(void * rtp, unsigned char * data, long len, long eos); 
void    RTP_EncodeInputAudio(void * rtp, unsigned char * data, long len); 

/* only for RTP_STRM_RTP */
void    RTP_SetEncodeVideoParam(void * rtp, long pt, long redpt, long initsq, unsigned long initts, long ssrc, long redundancy); 
void    RTP_SetEncodeAudioParam(void * rtp, long pt, long redpt, long initsq, unsigned long initts, long ssrc, long redundancy); 

/* only for RTP_STRM_HPMP */
void    RTP_SetEncodeParam(void * rtp, long redundancy, long maxpktsize); 

typedef void (* ON_RTP_DECODE_VIDEO_OPUT)(void * puserdata, unsigned char * data, long len, long errind); 
typedef void (* ON_RTP_DECODE_AUDIO_OPUT)(void * puserdata, unsigned char * data, long len); 

void *  RTP_CreateDecodeChl(int strmmode, long maxjbuffms); 
void    RTP_DeleteDecodeChl(void * rtp); 
void    RTP_DecodeInput(void * rtp, unsigned char * data, long len); 

/* only for RTP_STRM_RTP */
void    RTP_SetDecodeVideoParam(void * rtp, long pt, long redpt, ON_RTP_DECODE_VIDEO_OPUT pfnRtpDecodeVideoOput, void * puserdata); 
void    RTP_SetDecodeAudioParam(void * rtp, long pt, long redpt, ON_RTP_DECODE_AUDIO_OPUT pfnRtpDecodeAudioOput, void * puserdata); 

/* only for RTP_STRM_HPMP */
void    RTP_SetDecodeParam(void * rtp, ON_RTP_DECODE_VIDEO_OPUT pfnRtpDecodeVideoOput, void * puserdatavideo, ON_RTP_DECODE_AUDIO_OPUT pfnRtpDecodeAudioOput, void * puserdataaudio); 

void    RTP_Start();
void    RTP_Stop(); 
void    RTP_Schd(); 

#ifdef __cplusplus
}
#endif 
#pragma pack()
#endif 

