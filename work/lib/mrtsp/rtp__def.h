/*!
\file       rtp__def.h
\brief      rtmp module

 ----history----
\author     chengzhiyong
\date       2007-03-16
\version    0.01
\desc       create

$Author: chengzhiyong $
$Id: http.h,v 1.14 2008-09-02 08:27:27 chengzhiyong Exp $
*/

#ifndef _RTP_DEF_H
#define _RTP_DEF_H
#pragma pack(1)

#define  PACKET_BUFFER_SIZE         1600
#define  RTP_HEAD_SIZE              12
#define  HPMP_HEAD_SIZE             6 
#define  RTP_REV_HEAD_SIZE          (RTP_HEAD_SIZE+RTP_OUTSTRM_REV_HEAD_LEN)

enum 
{
    HPMP_DATA_VIDEO     = 0,
    HPMP_DATA_AUDIO     = 1, 
}; 

typedef struct 
{
    LONG                m_lRedundancy; 
    UCHAR               m_aucFecBuffer1[RTP_REV_HEAD_SIZE+PACKET_BUFFER_SIZE]; 
    UCHAR               m_aucFecBuffer2[RTP_REV_HEAD_SIZE+PACKET_BUFFER_SIZE]; 
    LONG                m_lFecPacketOffset1; 
    LONG                m_lFecPacketOffset2; 
    LONG                m_lFecBuffer1Len; 
    LONG                m_lFecBuffer2Len; 
} ST_ENC_FEC; 

#define INIT_ENC_FEC(stFec) {   \
    (stFec).m_lFecPacketOffset1 = 0;    \
    (stFec).m_lFecPacketOffset2 = -1;   \
    }

typedef struct 
{
    LONG                m_lVideoPt; 
    LONG                m_lVideoRedPt; 
    LONG                m_lVideoSsrc; 

    LONG                m_lVideoSeq; 
    ULONG               m_ulVideoTs; 
    ULONG               m_ulVideoCurTicks; 

    ST_ENC_FEC          m_stVideoEncFec; 

    LONG                m_lAudioPt; 
    LONG                m_lAudioRedPt; 
    LONG                m_lAudioSsrc; 

    LONG                m_lAudioSeq; 
    ULONG               m_ulAudioTs; 
    ULONG               m_ulAudioCurTicks;  

    ST_ENC_FEC          m_stAudioEncFec; 
} ST_RTP_ENC_RTP; 

typedef struct 
{
    LONG                m_lMaxPktSize; 
    LONG                m_lMinPktSize; 

    ULONG               m_ulLastSendTicks; 
    LONG                m_lCurFirstOffset; 
    LONG                m_lCurDataLen; 
    CHAR                m_aucBuffer[RTP_REV_HEAD_SIZE+PACKET_BUFFER_SIZE]; 
    LONG                m_lCurPktSize; 
    ULONG               m_ulLastTotalTicks; 
    LONG                m_lTotalPktCnt; 

    LONG                m_lStreamSsrc; 
    LONG                m_lStreamSeq; 
    ULONG               m_ulStreamTs; 
    ULONG               m_ulStreamCurTicks; 

    ST_ENC_FEC          m_stStreamEncFec; 
} ST_RTP_ENC_HPMP; 

typedef struct _ST_RTP_ENC_CHL
{
    DEF_LINK_NODE(_ST_RTP_ENC_CHL, link0); 

    ULONG               m_ulState;
    LONG                m_lRtpMode;

    USHORT              m_usVideoSendSeqNo; 
    USHORT              m_usAudioSendSeqNo; 

    ON_RTP_ENCODE_OPUT  m_pfnRtpEncodeOput; 
    VOID *              m_pEncodeOputUserdata; 

    union 
    {
        ST_RTP_ENC_RTP  m_stRtp; 
        ST_RTP_ENC_HPMP m_stHpmp; 
    }; 
} ST_RTP_ENC_CHL; 

typedef struct _ST_RTP_PKT 
{
    DEF_LINK_NODE(_ST_RTP_PKT, link0); 

    ULONG               m_ulReceiveTicks; 

    LONG                m_lSeqNo; 
    LONG                m_lTimestamp; 
    LONG                m_lSsrc; 
    union 
    {
        LONG            m_lPt; 
        LONG            m_lRedPkt; 
    }; 

    LONG                m_lDataLen; 
    UCHAR               m_aucRevHead[16]; 
    UCHAR               m_aucBuffer[4]; 
} ST_RTP_PKT; 

typedef struct 
{
    LINK_ENTITY(_ST_RTP_PKT) m_lnkJbuffPkt; 
    LINK_ENTITY(_ST_RTP_PKT) m_lnkFecPkt1; 
    LINK_ENTITY(_ST_RTP_PKT) m_lnkFecPkt2; 
    ST_RTP_PKT *             m_pstLossTmpPkt;

    LONG                m_lPt; 
    LONG                m_lSsrc; 
    LONG                m_lCurTimestamp; 
    
    LONG                m_lPktJbuffSize; 
    LONG                m_lFecJbuffSize;  

    LONG                m_lPktCurPtr; 
    LONG                m_alPktJbuffSize[8]; 
    ULONG               m_ulLastPktTicks; 
    
    LONG                m_lFecCurPtr; 
    LONG                m_alFecJbuffSize[8]; 
    ULONG               m_ulLastFecTicks; 
    
    LONG                m_lLastSeqNo;
    LONG                m_lLastTimestamp;  

    ULONG               m_ulLastRecvTicks;
} ST_DEC_JBUFF; 

#define INIT_DEC_JBUFF(stDecJbuff, jbuffms) {   \
    (stDecJbuff).m_lPktJbuffSize = jbuffms;     \
    (stDecJbuff).m_lFecJbuffSize = 0;   \
    (stDecJbuff).m_lLastSeqNo = MAX_LONG;   }

typedef struct 
{
    LONG                m_lVideoPt; 
    LONG                m_lVideoRedPt; 
    LONG                m_lAudioPt; 
    LONG                m_lAudioRedPt; 

    ST_DEC_JBUFF        m_stVideoJbuffer; 
    ST_DEC_JBUFF        m_stAudioJbuffer; 
} ST_RTP_DEC_RTP; 

typedef struct 
{
    ST_DEC_JBUFF        m_stStreamJbuffer; 

    LONG                m_lLastSeqNo;
    LONG                m_lDataTimestamp; 
    LONG                m_lDataType; 
    LONG                m_lDataTotalLen; 
    LONG                m_lDataLen; 
    CHAR *              m_pucBuffer;         
} ST_RTP_DEC_HPMP; 

typedef struct _ST_RTP_DEC_CHL
{
    DEF_LINK_NODE(_ST_RTP_DEC_CHL, link0); 

    ULONG               m_ulState; 
    LONG                m_lRtpMode;    
    LONG                m_lMaxJBufferMs; 
    LONG                m_lJbuffSize;  

    USHORT              m_usVideoRecvSeqNo; 
    USHORT              m_usAudioRecvSeqNo; 

    ON_RTP_DECODE_VIDEO_OPUT    m_pfnRtpDecodeVideoOput; 
    VOID *                      m_pVideoUserData; 
    ON_RTP_DECODE_AUDIO_OPUT    m_pfnRtpDecodeAudioOput; 
    VOID *                      m_pAudioUserData; 

    ULONG               m_ulCurTicks; 
    ULONG               m_ulLastDecJbuffTicks; 
    union 
    {
        ST_RTP_DEC_RTP  m_stRtp; 
        ST_RTP_DEC_HPMP m_stHpmp; 
    }; 
} ST_RTP_DEC_CHL; 

typedef struct 
{
    ULONG               m_ulState; 
    ULONG               m_ref_counts;
#if 0
    LOCK_ENTITY         m_lockEnc;
    LOCK_ENTITY         m_lockDec;
#endif 

    LINK_ENTITY(_ST_RTP_ENC_CHL)    m_lnkRtpEncChl; 
    LINK_ENTITY(_ST_RTP_DEC_CHL)    m_lnkRtpDecChl;

#if 0
#if defined(WIN32) || defined(WINCE)
    HANDLE              m_hThread; 
#endif 
#endif 
} ST_RTP_CB; 

#define RTP_STATE_INUSE     0x34892943

#define DLTA_VAL(dlta)      ((LONG)(SHORT)(dlta))

#pragma pack()
#endif 

