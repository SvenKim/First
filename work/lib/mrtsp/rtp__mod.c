/*!
\file       rtp__mod.c
\brief      rtmp module

 ----history----
\author     chengzhiyong
\date       2007-03-16
\version    0.01
\desc       create

$Author: chengzhiyong $
$Id: http.h,v 1.14 2008-09-02 08:27:27 chengzhiyong Exp $
*/
#include "mcore/mcore.h"
#include "rtp__mod.h"
#include "rtp__os.h"
#include "rtp__cfg.h"
#include "rtp__def.h"

ST_RTP_CB          m_stRtpCB; 

extern CONST UCHAR m_aucRtpRsMult[32][256]; 
extern CONST UCHAR m_aucRtpRsDiv[256][256]; 
extern CONST UCHAR m_aucRtpRsAdd[32][32]; 

STATIC VOID    RTP_EncodeRtpFec(ST_ENC_FEC * pstFec, UCHAR * pucData, LONG lLen)
{
    UCHAR ucLen1, ucLen2; 
    CONST UCHAR * pucRsMultTable; 
    LONG i; 

    if ( pstFec->m_lFecPacketOffset1 == (pstFec->m_lRedundancy>>1))
    {
        ASSERT(pstFec->m_lFecPacketOffset2 == 0 || pstFec->m_lFecPacketOffset2 == -1); 
        pstFec->m_lFecPacketOffset2 = 0;                 
    }

    if (lLen > PACKET_BUFFER_SIZE-4)
        lLen = PACKET_BUFFER_SIZE-4;        
    ucLen1 = (UCHAR)(lLen>>8); 
    ucLen2 = (UCHAR)(lLen&0xFF); 
    
    ASSERT(pstFec->m_lFecPacketOffset1 >= 0); 
    if ( pstFec->m_lFecPacketOffset1 == 0)
    {
        MEMSET(pstFec->m_aucFecBuffer1+RTP_REV_HEAD_SIZE, 0, PACKET_BUFFER_SIZE); 
        
        pstFec->m_aucFecBuffer1[2+RTP_REV_HEAD_SIZE] = ucLen1; 
        pstFec->m_aucFecBuffer1[3+RTP_REV_HEAD_SIZE] = ucLen2; 
        MEMCPY(pstFec->m_aucFecBuffer1+4+RTP_REV_HEAD_SIZE, pucData, lLen); 

        pstFec->m_lFecBuffer1Len = lLen+4; 
        pstFec->m_lFecPacketOffset1 ++; 
    }
    else 
    {
        pstFec->m_aucFecBuffer1[2+RTP_REV_HEAD_SIZE] ^= ucLen1; 
        pstFec->m_aucFecBuffer1[3+RTP_REV_HEAD_SIZE] ^= ucLen2; 

		for ( i=0; i< lLen; i++)
            pstFec->m_aucFecBuffer1[i+4+RTP_REV_HEAD_SIZE] ^= pucData[i]; 

        MAX(pstFec->m_lFecBuffer1Len, lLen+4, pstFec->m_lFecBuffer1Len); 
        pstFec->m_lFecPacketOffset1 ++; 
    }

    if ( pstFec->m_lFecPacketOffset2 >= 0)
    {
        pucRsMultTable = m_aucRtpRsMult[pstFec->m_lFecPacketOffset2]; 
        if ( pstFec->m_lFecPacketOffset2 == 0)
        {
            MEMSET(pstFec->m_aucFecBuffer2+RTP_REV_HEAD_SIZE, 0, PACKET_BUFFER_SIZE); 
            
            pstFec->m_aucFecBuffer2[2+RTP_REV_HEAD_SIZE] = pucRsMultTable[ucLen1]; 
            pstFec->m_aucFecBuffer2[3+RTP_REV_HEAD_SIZE] = pucRsMultTable[ucLen2]; 

            for ( i=0;  i< lLen; i++)
                pstFec->m_aucFecBuffer2[i+4+RTP_REV_HEAD_SIZE] = pucRsMultTable[pucData[i]]; 
            
            pstFec->m_lFecBuffer2Len = lLen+4; 
            pstFec->m_lFecPacketOffset2 ++; 
        }
        else 
        {
            pstFec->m_aucFecBuffer2[2+RTP_REV_HEAD_SIZE] ^= pucRsMultTable[ucLen1]; 
            pstFec->m_aucFecBuffer2[3+RTP_REV_HEAD_SIZE] ^= pucRsMultTable[ucLen2]; 

            for ( i=0;  i< lLen; i++)
                pstFec->m_aucFecBuffer2[i+4+RTP_REV_HEAD_SIZE] ^= pucRsMultTable[pucData[i]]; 
            
            MAX(pstFec->m_lFecBuffer2Len, lLen+4, pstFec->m_lFecBuffer2Len); 
            pstFec->m_lFecPacketOffset2 ++; 
        }
    }
}

STATIC VOID RTP_EncodeProcessHpmpPkt(ST_RTP_ENC_CHL * pstRtpChl)
{
    CHAR * pucData; 
    LONG lDataLen, i; 
    USHORT usVal; 
    ULONG ulCurTicks; 

    pucData = pstRtpChl->m_stHpmp.m_aucBuffer+RTP_REV_HEAD_SIZE; 
    lDataLen = pstRtpChl->m_stHpmp.m_lCurDataLen; 

    pucData -= 2; 
    lDataLen += 2; 

    usVal  = (USHORT)((pstRtpChl->m_stHpmp.m_lCurFirstOffset-1)<<4); 
    *(USHORT *)(pucData) = NTOA_W(usVal);

    if ( pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lRedundancy > 0)
        RTP_EncodeRtpFec(&pstRtpChl->m_stHpmp.m_stStreamEncFec, (unsigned char*)pucData, lDataLen); 

    ulCurTicks = CUR_TICKS(); 
    if ( pstRtpChl->m_stHpmp.m_lTotalPktCnt == 0)
        pstRtpChl->m_stHpmp.m_ulLastTotalTicks = ulCurTicks; 

    pucData -= HPMP_HEAD_SIZE; 
    lDataLen  += HPMP_HEAD_SIZE;
    *(USHORT *)(pucData+0) = NTOA_W(pstRtpChl->m_stHpmp.m_ulStreamTs);
    *(USHORT *)(pucData+2) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSeq);
    *(USHORT *)(pucData+4) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSsrc);

    pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, (unsigned char*)pucData, lDataLen);
    pstRtpChl->m_stHpmp.m_lTotalPktCnt ++; 
    if ( pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lRedundancy > 0)
    {
        if ( pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset1 >= pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lRedundancy )
        {
            pucData =(CHAR*) pstRtpChl->m_stHpmp.m_stStreamEncFec.m_aucFecBuffer1+RTP_REV_HEAD_SIZE; 
            lDataLen = pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecBuffer1Len; 
            pucData[0] = (UCHAR)0xFE; 
            pucData[1] = (UCHAR)pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset1; 
    
            pucData -= HPMP_HEAD_SIZE; 
            lDataLen  += HPMP_HEAD_SIZE;

            *(USHORT *)(pucData+0) = NTOA_W(pstRtpChl->m_stHpmp.m_ulStreamTs);
            *(USHORT *)(pucData+2) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSeq);
            *(USHORT *)(pucData+4) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSsrc);
            
            pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, (unsigned char*)pucData, lDataLen); 
            pstRtpChl->m_stHpmp.m_lTotalPktCnt ++; 
            pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset1 = 0; 
        }
    
        if ( pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset2 >= pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lRedundancy )
        {
            pucData = (CHAR*)pstRtpChl->m_stHpmp.m_stStreamEncFec.m_aucFecBuffer2+RTP_REV_HEAD_SIZE; 
            lDataLen = pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecBuffer2Len; 
            pucData[0] = (UCHAR)0xFE; 
            pucData[1] = (UCHAR)pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset2|0x80; 
    
            pucData -= HPMP_HEAD_SIZE; 
            lDataLen  += HPMP_HEAD_SIZE;
            
            *(USHORT *)(pucData+0) = NTOA_W(pstRtpChl->m_stHpmp.m_ulStreamTs);
            *(USHORT *)(pucData+2) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSeq);
            *(USHORT *)(pucData+4) = NTOA_W(pstRtpChl->m_stHpmp.m_lStreamSsrc);
            
            pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, (unsigned char*)pucData, lDataLen); 
            pstRtpChl->m_stHpmp.m_lTotalPktCnt ++; 
            pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lFecPacketOffset2 = 0; 
        }
    }
    
    pstRtpChl->m_usVideoSendSeqNo = 
    pstRtpChl->m_usAudioSendSeqNo = (USHORT)pstRtpChl->m_stHpmp.m_lStreamSeq; 
    pstRtpChl->m_stHpmp.m_lStreamSeq ++; 

    pstRtpChl->m_stHpmp.m_ulLastSendTicks = ulCurTicks; 
    pstRtpChl->m_stHpmp.m_lCurFirstOffset = 0; 
    pstRtpChl->m_stHpmp.m_lCurDataLen = 0; 

    if ( ulCurTicks - pstRtpChl->m_stHpmp.m_ulLastTotalTicks >= 2000)
    {
        pstRtpChl->m_stHpmp.m_ulLastTotalTicks = ulCurTicks; 
        if ( pstRtpChl->m_stHpmp.m_lTotalPktCnt > 10)
        {
            i = pstRtpChl->m_stHpmp.m_lCurPktSize*pstRtpChl->m_stHpmp.m_lTotalPktCnt/120; 
            i = (i+2)&(-3); 
            
            if ( i > pstRtpChl->m_stHpmp.m_lCurPktSize*12/10 
                || i< pstRtpChl->m_stHpmp.m_lCurPktSize*8/10)
            {
                if ( i > pstRtpChl->m_stHpmp.m_lMaxPktSize)
                    i = pstRtpChl->m_stHpmp.m_lMaxPktSize; 
                else if ( i< pstRtpChl->m_stHpmp.m_lMinPktSize)
                    i = pstRtpChl->m_stHpmp.m_lMinPktSize;                 
                pstRtpChl->m_stHpmp.m_lCurPktSize = i; 
            }
        }
        
        pstRtpChl->m_stHpmp.m_lTotalPktCnt = 0; 
    }
}

STATIC VOID RTP_EncodeProcessHpmpData(ST_RTP_ENC_CHL * pstRtpChl, LONG  lDataType, UCHAR * pucData, LONG lDataLen)
{
    LONG lTmp; 
   
    if ( pstRtpChl->m_stHpmp.m_lCurFirstOffset == 0)
        pstRtpChl->m_stHpmp.m_lCurFirstOffset = pstRtpChl->m_stHpmp.m_lCurDataLen+1; 
    lTmp = (lDataType<<24)+lDataLen; 
    *(ULONG *)(pstRtpChl->m_stHpmp.m_aucBuffer + RTP_REV_HEAD_SIZE + pstRtpChl->m_stHpmp.m_lCurDataLen) = NTOA_L(lTmp); 
    pstRtpChl->m_stHpmp.m_lCurDataLen += 4; 

    lTmp = pstRtpChl->m_stHpmp.m_lCurPktSize - pstRtpChl->m_stHpmp.m_lCurDataLen; 
    ASSERT(lTmp > 0); 
    if ( lTmp > lDataLen)
        lTmp = lDataLen;         
    MEMCPY(pstRtpChl->m_stHpmp.m_aucBuffer + RTP_REV_HEAD_SIZE + pstRtpChl->m_stHpmp.m_lCurDataLen, pucData, lTmp); 
    
    pstRtpChl->m_stHpmp.m_lCurDataLen += lTmp; 
    pucData += lTmp; 
    lDataLen -= lTmp; 

    if ( pstRtpChl->m_stHpmp.m_lCurDataLen < pstRtpChl->m_stHpmp.m_lCurPktSize - 16)
        return; 
    RTP_EncodeProcessHpmpPkt(pstRtpChl); 

    while ( lDataLen >= pstRtpChl->m_stHpmp.m_lCurPktSize - 16)
    {
        lTmp = pstRtpChl->m_stHpmp.m_lCurPktSize; 
        if ( lTmp > lDataLen)
            lTmp = lDataLen;
        
        MEMCPY(pstRtpChl->m_stHpmp.m_aucBuffer + RTP_REV_HEAD_SIZE, pucData, lTmp); 
        
        pstRtpChl->m_stHpmp.m_lCurDataLen = lTmp; 
        pucData += lTmp; 
        lDataLen -= lTmp; 
        
        RTP_EncodeProcessHpmpPkt(pstRtpChl); 
    }

    if ( lDataLen)
    {
        MEMCPY(pstRtpChl->m_stHpmp.m_aucBuffer + RTP_REV_HEAD_SIZE, pucData, lDataLen); 
        pstRtpChl->m_stHpmp.m_lCurDataLen = lDataLen; 
    }
}

void *  RTP_CreateEncodeChl(int strmmode)
{
    ST_RTP_ENC_CHL * pstRtpChl; 
    LOCK_ENC(); 

    if ( m_stRtpCB.m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_ENC(); 
        return 0; 
    }

    pstRtpChl = (ST_RTP_ENC_CHL *)MALLOC(sizeof(ST_RTP_ENC_CHL)); 
    if ( pstRtpChl == NULL)
    {
        UNLOCK_ENC(); 
        return 0; 
    }
    MEMSET(pstRtpChl, 0, sizeof(ST_RTP_ENC_CHL)); 

    pstRtpChl->m_lRtpMode = strmmode; 
    if (pstRtpChl->m_lRtpMode == RTP_STRM_RTP)
    {
        INIT_ENC_FEC(pstRtpChl->m_stRtp.m_stVideoEncFec); 
        INIT_ENC_FEC(pstRtpChl->m_stRtp.m_stAudioEncFec); 
    }
    else 
    {
        pstRtpChl->m_lRtpMode = RTP_STRM_HPMP; 
        INIT_ENC_FEC(pstRtpChl->m_stHpmp.m_stStreamEncFec); 
    }

    LINK_ADD_TAIL(m_stRtpCB.m_lnkRtpEncChl, pstRtpChl, link0); 
    pstRtpChl->m_ulState = RTP_STATE_INUSE; 

    UNLOCK_ENC(); 
    return pstRtpChl; 
}

void    RTP_DeleteEncodeChl(void * rtp)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_ENC(); 
        return; 
    }

    LINK_REMOVE_NODE(m_stRtpCB.m_lnkRtpEncChl, pstRtpChl, link0); 
    pstRtpChl->m_ulState = 0; 

    FREE(pstRtpChl); 
    UNLOCK_ENC(); 
}

void    RTP_SetEncodeOputHook(void * rtp, ON_RTP_ENCODE_OPUT pfnRtpEncodeOput, void * puserdata)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_ENC(); 
        return; 
    }

    pstRtpChl->m_pfnRtpEncodeOput = pfnRtpEncodeOput; 
    pstRtpChl->m_pEncodeOputUserdata = puserdata; 
    
    UNLOCK_ENC(); 
}

void    RTP_EncodeInputVideo(void * rtp, unsigned char * data, long len, long eos)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 
    ULONG ulCurTicks; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_ENC(); 
        return; 
    }

    ulCurTicks = CUR_TICKS(); 
    if (pstRtpChl->m_lRtpMode == RTP_STRM_HPMP)
    {
        pstRtpChl->m_stHpmp.m_ulStreamTs += ulCurTicks-pstRtpChl->m_stHpmp.m_ulStreamCurTicks; 
        pstRtpChl->m_stHpmp.m_ulStreamCurTicks = ulCurTicks;     

        while ( data[0] == 0)
        {
            data ++; 
            len --; 
        }
        if ( data[0] != 1)
            return; 
        data ++; 
        len --; 

        RTP_EncodeProcessHpmpData(pstRtpChl, HPMP_DATA_VIDEO, data, len);        
    }
    else 
    {
        if ( pstRtpChl->m_stRtp.m_stVideoEncFec.m_lRedundancy > 0)
               RTP_EncodeRtpFec(&pstRtpChl->m_stRtp.m_stVideoEncFec, data, len); 
             
        data -= RTP_HEAD_SIZE; 
        len  += RTP_HEAD_SIZE; 
     
        data[0] = 0x80; 
        data[1] = eos?(0x80|(UCHAR)pstRtpChl->m_stRtp.m_lVideoPt):(UCHAR)pstRtpChl->m_stRtp.m_lVideoPt; 
        *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lVideoSeq);
        pstRtpChl->m_stRtp.m_ulVideoTs += (ulCurTicks-pstRtpChl->m_stRtp.m_ulVideoCurTicks)*90; 
        pstRtpChl->m_stRtp.m_ulVideoCurTicks = ulCurTicks;     
        *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulVideoTs); 
        *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lVideoSsrc); 
     
        pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
        
        if ( pstRtpChl->m_stRtp.m_stVideoEncFec.m_lRedundancy > 0)
        {
            if ( pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset1 >= pstRtpChl->m_stRtp.m_stVideoEncFec.m_lRedundancy )
            {
                data = pstRtpChl->m_stRtp.m_stVideoEncFec.m_aucFecBuffer1+RTP_REV_HEAD_SIZE; 
                len = pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecBuffer1Len; 
                data[0] = 0x60; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset1; 
     
                data -= RTP_HEAD_SIZE; 
                len  += RTP_HEAD_SIZE;             
                data[0] = 0x80; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_lVideoRedPt; 
                *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lVideoSeq);
                *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulVideoTs); 
                *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lVideoSsrc); 
                
                pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
                pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset1 = 0; 
            }
     
            if ( pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset2 >= pstRtpChl->m_stRtp.m_stVideoEncFec.m_lRedundancy )
            {
                data = pstRtpChl->m_stRtp.m_stVideoEncFec.m_aucFecBuffer2+RTP_REV_HEAD_SIZE; 
                len = pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecBuffer2Len; 
                data[0] = 0x60; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset2|0x80; 
     
                data -= RTP_HEAD_SIZE; 
                len  += RTP_HEAD_SIZE;             
                data[0] = 0x80; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_lVideoRedPt; 
                *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lVideoSeq);
                *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulVideoTs); 
                *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lVideoSsrc); 
                
                pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
                pstRtpChl->m_stRtp.m_stVideoEncFec.m_lFecPacketOffset2 = 0; 
            }
        }
        
        pstRtpChl->m_usVideoSendSeqNo = (USHORT)pstRtpChl->m_stRtp.m_lVideoSeq; 
        pstRtpChl->m_stRtp.m_lVideoSeq ++; 
    }

    UNLOCK_ENC(); 
}

void    RTP_EncodeInputAudio(void * rtp, unsigned char * data, long len)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 
    ULONG ulCurTicks; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_ENC(); 
        return; 
    }

    ulCurTicks = CUR_TICKS(); 
    if (pstRtpChl->m_lRtpMode == RTP_STRM_HPMP)
    {
        pstRtpChl->m_stHpmp.m_ulStreamTs += ulCurTicks-pstRtpChl->m_stHpmp.m_ulStreamCurTicks; 
        pstRtpChl->m_stHpmp.m_ulStreamCurTicks = ulCurTicks;     

        RTP_EncodeProcessHpmpData(pstRtpChl, HPMP_DATA_AUDIO, data, len); 
    }
    else 
    {
        if ( pstRtpChl->m_stRtp.m_stAudioEncFec.m_lRedundancy > 0)
               RTP_EncodeRtpFec(&pstRtpChl->m_stRtp.m_stAudioEncFec, data, len); 
             
        data -= RTP_HEAD_SIZE; 
        len  += RTP_HEAD_SIZE; 
     
        data[0] = 0x80; 
        data[1] = (UCHAR)pstRtpChl->m_stRtp.m_lAudioPt; 
        *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lAudioSeq);
        pstRtpChl->m_stRtp.m_ulAudioTs += ulCurTicks-pstRtpChl->m_stRtp.m_ulAudioCurTicks; 
        pstRtpChl->m_stRtp.m_ulAudioCurTicks = ulCurTicks;
        *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulAudioTs); 
        *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lAudioSsrc); 
     
        pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
        
        if ( pstRtpChl->m_stRtp.m_stAudioEncFec.m_lRedundancy > 0)
        {
            if ( pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset1 >= pstRtpChl->m_stRtp.m_stAudioEncFec.m_lRedundancy )
            {
                data = pstRtpChl->m_stRtp.m_stAudioEncFec.m_aucFecBuffer1+RTP_REV_HEAD_SIZE; 
                len = pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecBuffer1Len; 
                data[0] = 0x60; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset1; 
     
                data -= RTP_HEAD_SIZE; 
                len  += RTP_HEAD_SIZE;             
                data[0] = 0x80; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_lAudioRedPt; 
                *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lAudioSeq);
                *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulAudioTs); 
                *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lAudioSsrc); 
                
                pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
                pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset1 = 0; 
            }
     
            if ( pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset2 >= pstRtpChl->m_stRtp.m_stAudioEncFec.m_lRedundancy )
            {
                data = pstRtpChl->m_stRtp.m_stAudioEncFec.m_aucFecBuffer2+RTP_REV_HEAD_SIZE; 
                len = pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecBuffer2Len; 
                data[0] = 0x60; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset2|0x80; 
     
                data -= RTP_HEAD_SIZE; 
                len  += RTP_HEAD_SIZE;             
                data[0] = 0x80; 
                data[1] = (UCHAR)pstRtpChl->m_stRtp.m_lAudioRedPt; 
                *(USHORT *)(data+2) = NTOA_W(pstRtpChl->m_stRtp.m_lAudioSeq);
                *(ULONG *)(data+4) = NTOA_L(pstRtpChl->m_stRtp.m_ulAudioTs); 
                *(ULONG *)(data+8) = NTOA_L(pstRtpChl->m_stRtp.m_lAudioSsrc); 
                
                pstRtpChl->m_pfnRtpEncodeOput(pstRtpChl->m_pEncodeOputUserdata, data, len); 
                pstRtpChl->m_stRtp.m_stAudioEncFec.m_lFecPacketOffset2 = 0; 
            }
        }
        
        pstRtpChl->m_usAudioSendSeqNo = (USHORT)pstRtpChl->m_stRtp.m_lAudioSeq; 
        pstRtpChl->m_stRtp.m_lAudioSeq ++; 
    }

    UNLOCK_ENC(); 
}

void    RTP_SetEncodeVideoParam(void * rtp, long pt, long redpt, long initsq, unsigned long initts, long ssrc, long redundancy)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_RTP)
    {
        UNLOCK_ENC(); 
        return; 
    }

    pstRtpChl->m_stRtp.m_lVideoPt = pt; 
    pstRtpChl->m_stRtp.m_lVideoRedPt = redpt; 
    pstRtpChl->m_stRtp.m_lVideoSeq = initsq; 
    pstRtpChl->m_stRtp.m_lVideoSsrc = ssrc; 
    if ( redundancy > 0 )
    {
        redundancy = (redundancy+199)/redundancy; 
        if ( redundancy < 2) redundancy = 2; 
        else if (redundancy > 20) redundancy = 20; 
    }
    else 
    {
        redundancy = 0; 
    }

    pstRtpChl->m_stRtp.m_stVideoEncFec.m_lRedundancy = redundancy;     
    pstRtpChl->m_stRtp.m_ulVideoTs = initts; 
    pstRtpChl->m_stRtp.m_ulVideoCurTicks = CUR_TICKS(); 

    INIT_ENC_FEC(pstRtpChl->m_stRtp.m_stVideoEncFec); 

    UNLOCK_ENC(); 
}

void    RTP_SetEncodeAudioParam(void * rtp, long pt, long redpt, long initsq, unsigned long initts, long ssrc, long redundancy)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_RTP)
    {
        UNLOCK_ENC(); 
        return; 
    }

    pstRtpChl->m_stRtp.m_lAudioPt = pt; 
    pstRtpChl->m_stRtp.m_lAudioRedPt = redpt; 
    pstRtpChl->m_stRtp.m_lAudioSeq = initsq; 
    pstRtpChl->m_stRtp.m_lAudioSsrc = ssrc; 
    if ( redundancy > 0 )
    {
        redundancy = (redundancy+199)/redundancy; 
        if ( redundancy < 2) redundancy = 2; 
        else if (redundancy > 20) redundancy = 20; 
    }
    else 
    {
        redundancy = 0; 
    }
    
    pstRtpChl->m_stRtp.m_stAudioEncFec.m_lRedundancy = redundancy;     
    pstRtpChl->m_stRtp.m_ulAudioTs = initts; 
    pstRtpChl->m_stRtp.m_ulAudioCurTicks = CUR_TICKS(); 

    INIT_ENC_FEC(pstRtpChl->m_stRtp.m_stAudioEncFec); 

    UNLOCK_ENC(); 
}

void    RTP_SetEncodeParam(void * rtp, long redundancy, long maxpktsize)
{
    ST_RTP_ENC_CHL * pstRtpChl = (ST_RTP_ENC_CHL *)rtp; 

    LOCK_ENC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_HPMP)
    {
        UNLOCK_ENC(); 
        return; 
    }

    if ( redundancy > 0 )
    {
        redundancy = (redundancy+199)/redundancy; 
        if ( redundancy < 2) redundancy = 2; 
        else if (redundancy > 20) redundancy = 20; 
    }
    else 
    {
        redundancy = 0; 
    }
    
    pstRtpChl->m_stHpmp.m_stStreamEncFec.m_lRedundancy = redundancy;

    if ( maxpktsize > 1460)
        maxpktsize = 1460; 
    
    pstRtpChl->m_stHpmp.m_lMaxPktSize = maxpktsize;
    if ( redundancy == 0)
        pstRtpChl->m_stHpmp.m_lMinPktSize = maxpktsize;
    else 
        pstRtpChl->m_stHpmp.m_lMinPktSize = 512;
    pstRtpChl->m_stHpmp.m_lCurPktSize = (maxpktsize+pstRtpChl->m_stHpmp.m_lMinPktSize)/2; 

    pstRtpChl->m_stHpmp.m_lStreamSsrc = RAND(); 
    pstRtpChl->m_stHpmp.m_lStreamSeq = RAND(); 
    pstRtpChl->m_stHpmp.m_ulStreamTs = RAND(); 
    pstRtpChl->m_stHpmp.m_ulStreamCurTicks = CUR_TICKS(); 

    INIT_ENC_FEC(pstRtpChl->m_stHpmp.m_stStreamEncFec); 

    UNLOCK_ENC(); 
}

STATIC ST_RTP_PKT * RTP_DecodeRtpPkt(UCHAR * pucData, LONG lLen)
{
    ST_RTP_PKT * pstRtpPkt; 
    LONG lRtpHeadSize; 

    lRtpHeadSize = RTP_HEAD_SIZE + ((pucData[0]&0xF)<<2); 
    if ( lLen <= lRtpHeadSize )
        return 0; 
    
    pstRtpPkt = (ST_RTP_PKT *)MALLOC(sizeof(ST_RTP_PKT)+lLen-lRtpHeadSize-4); 
    if ( pstRtpPkt == 0)
        return 0; 

    pstRtpPkt->m_ulReceiveTicks = CUR_TICKS(); 
    pstRtpPkt->m_lPt = pucData[1]&0x7F; 
    pstRtpPkt->m_lSeqNo = ((pucData[2]<<8)+pucData[3]); 
    pstRtpPkt->m_lTimestamp = NTOA_L(*(ULONG *)(pucData+4)); 
    pstRtpPkt->m_lSsrc = NTOA_L(*(ULONG *)(pucData+8)); 

    pstRtpPkt->m_lDataLen = lLen-lRtpHeadSize; 
    MEMCPY(pstRtpPkt->m_aucBuffer, pucData+lRtpHeadSize, pstRtpPkt->m_lDataLen); 

    return pstRtpPkt;
}

STATIC ST_RTP_PKT * RTP_DecodeHpmpPkt(UCHAR * pucData, LONG lLen)
{
    ST_RTP_PKT * pstRtpPkt; 

    if ( lLen <= HPMP_HEAD_SIZE )
        return 0; 
    
    pstRtpPkt = (ST_RTP_PKT *)MALLOC(sizeof(ST_RTP_PKT)+lLen-HPMP_HEAD_SIZE-4); 
    if ( pstRtpPkt == 0)
        return 0; 
    
    pstRtpPkt->m_ulReceiveTicks = CUR_TICKS(); 
    pstRtpPkt->m_lTimestamp = ((pucData[0]<<8)+pucData[1]); 
    pstRtpPkt->m_lSeqNo = ((pucData[2]<<8)+pucData[3]); 
    pstRtpPkt->m_lSsrc = ((pucData[4]<<8)+pucData[5]); 

    if ( pucData[6] == 0xFE)
        pstRtpPkt->m_lRedPkt = 1;         
    else 
        pstRtpPkt->m_lRedPkt = 0; 

    pstRtpPkt->m_lDataLen = lLen-HPMP_HEAD_SIZE; 
    MEMCPY(pstRtpPkt->m_aucBuffer, pucData+HPMP_HEAD_SIZE, pstRtpPkt->m_lDataLen); 

    return pstRtpPkt;
}

STATIC ST_RTP_PKT * RTP_CreateRtpPkt(UCHAR * pucData, LONG lLen)
{
    ST_RTP_PKT * pstRtpPkt; 
  
    pstRtpPkt = (ST_RTP_PKT *)MALLOC(sizeof(ST_RTP_PKT)+lLen-4); 
    if ( pstRtpPkt == 0)
        return 0; 

    pstRtpPkt->m_ulReceiveTicks = CUR_TICKS(); 
    pstRtpPkt->m_lDataLen = lLen; 
    MEMCPY(pstRtpPkt->m_aucBuffer, pucData, pstRtpPkt->m_lDataLen); 

    return pstRtpPkt;
}

STATIC VOID RTP_FreeRtpPkt(ST_RTP_PKT * pstRtpPkt)
{
    FREE(pstRtpPkt); 
}

STATIC BOOL RTP_DecodeInputRed(ST_DEC_JBUFF * pstDecJbuff, ST_RTP_PKT * pstRtpPkt)
{
    ST_RTP_PKT *pstTmpPkt1;   
    LONG lTmp; 

    if ( pstRtpPkt->m_aucBuffer[0] != 0x60
        && pstRtpPkt->m_aucBuffer[0] != 0xFE )
        return RTP_FALSE; 
    lTmp = pstRtpPkt->m_aucBuffer[1]&0x7F; 
    if (lTmp > 20)
        return RTP_FALSE; 
    
    pstTmpPkt1 = LINK_TAIL(pstDecJbuff->m_lnkFecPkt2); 
    while ( pstTmpPkt1)
    {
        if (DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt1->m_lSeqNo) > 0)
            break; 
        pstTmpPkt1 = LINK_PREV(pstTmpPkt1, link0); 
    }
    
    LINK_INSERT_NEXT(pstDecJbuff->m_lnkFecPkt2, pstTmpPkt1, pstRtpPkt, link0); 
    return RTP_TRUE; 
}

STATIC BOOL RTP_DecodeInputPkt(ST_RTP_DEC_CHL * pstRtpChl, ST_DEC_JBUFF * pstDecJbuff, ST_RTP_PKT * pstRtpPkt)
{
    LONG lDltaSeqNo, lDltaTimestamp, lBuffSize, i; 
    ST_RTP_PKT *pstTmpPkt1, *pstTmpPkt2; 
   
    if ( pstRtpPkt->m_lSsrc != pstDecJbuff->m_lSsrc )
    {
reinit_jbuff: 
        if ( pstDecJbuff->m_pstLossTmpPkt )
        {
            pstTmpPkt1 = pstDecJbuff->m_pstLossTmpPkt;             
            if ( pstTmpPkt1->m_lSsrc == pstRtpPkt->m_lSsrc )
            {
                lDltaSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt1->m_lSeqNo); 
                lDltaTimestamp = DLTA_VAL(pstRtpPkt->m_lTimestamp - pstTmpPkt1->m_lTimestamp); 

                if ((INRNG1(lDltaSeqNo, 1, 8) && INRNG1(lDltaTimestamp, 0, pstRtpChl->m_lMaxJBufferMs))
                    ||(INRNG1(lDltaSeqNo, -8, -1) && INRNG1(lDltaTimestamp, -pstRtpChl->m_lMaxJBufferMs, 0)))
                {
                    while (1)
                    {
                        pstTmpPkt2 = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
                        if ( pstTmpPkt2 == NULL)
                            break; 
                        LINK_REMOVE_HEAD(pstDecJbuff->m_lnkJbuffPkt, link0); 

                        RTP_FreeRtpPkt(pstTmpPkt2); 
                    }

                    while (1)
                    {
                        pstTmpPkt2 = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
                        if ( pstTmpPkt2 == NULL)
                            break; 
                        LINK_REMOVE_HEAD(pstDecJbuff->m_lnkFecPkt1, link0); 

                        RTP_FreeRtpPkt(pstTmpPkt2); 
                    }

                    while (1)
                    {
                        pstTmpPkt2 = LINK_HEAD(pstDecJbuff->m_lnkFecPkt2); 
                        if ( pstTmpPkt2 == NULL)
                            break; 
                        LINK_REMOVE_HEAD(pstDecJbuff->m_lnkFecPkt2, link0); 

                        RTP_FreeRtpPkt(pstTmpPkt2); 
                    }

                    LINK_ADD_TAIL(pstDecJbuff->m_lnkJbuffPkt, pstTmpPkt1, link0); 
                    if ( lDltaSeqNo > 0 )
                    {
                        LINK_ADD_TAIL(pstDecJbuff->m_lnkJbuffPkt, pstRtpPkt, link0);                         
                    }
                    else 
                    {
                        ASSERT(lDltaSeqNo < 0); 
                        LINK_ADD_HEAD(pstDecJbuff->m_lnkJbuffPkt, pstRtpPkt, link0);
                    }

                    pstDecJbuff->m_pstLossTmpPkt = NULL; 
                    pstDecJbuff->m_lSsrc = pstRtpPkt->m_lSsrc;

                    pstDecJbuff->m_lCurTimestamp = pstTmpPkt1->m_lTimestamp; 
                    pstDecJbuff->m_lLastSeqNo = MAX_LONG; 

                    pstDecJbuff->m_lPktCurPtr = 0; 
                    MEMSET(pstDecJbuff->m_alPktJbuffSize, 0, sizeof(pstDecJbuff->m_alPktJbuffSize)); 
                    pstDecJbuff->m_ulLastPktTicks = pstRtpChl->m_ulCurTicks; 
                    pstDecJbuff->m_lFecCurPtr = 0; 
                    MEMSET(pstDecJbuff->m_alFecJbuffSize, 0, sizeof(pstDecJbuff->m_alFecJbuffSize)); 
                    pstDecJbuff->m_ulLastFecTicks = pstRtpChl->m_ulCurTicks; 

                    lBuffSize = JBUFF_PKT_ADD_SIZE - lDltaTimestamp; 
                    MIN(lBuffSize, pstRtpChl->m_lMaxJBufferMs, lBuffSize); 
                    MAX(lBuffSize, JBUFF_INIT_SIZE, pstDecJbuff->m_alPktJbuffSize[0]); 
                    pstDecJbuff->m_lPktJbuffSize = lBuffSize; 

                    return RTP_TRUE; 
                }
            }

            RTP_FreeRtpPkt(pstTmpPkt1); 
        }

        pstDecJbuff->m_pstLossTmpPkt = pstRtpPkt; 
        return RTP_TRUE; 
    }

    if ( LINK_NODE_NUM(pstDecJbuff->m_lnkJbuffPkt) == 0)
    {
        lDltaSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstDecJbuff->m_lLastSeqNo); 
        lDltaTimestamp = DLTA_VAL(pstRtpPkt->m_lTimestamp - pstDecJbuff->m_lLastTimestamp); 

        if (   ABS(lDltaSeqNo) >= 1024
            || ABS(lDltaTimestamp) >= (pstRtpChl->m_lMaxJBufferMs<<2))
            goto reinit_jbuff; 

        if (pstDecJbuff->m_pstLossTmpPkt )
        {
            RTP_FreeRtpPkt(pstDecJbuff->m_pstLossTmpPkt); 
            pstDecJbuff->m_pstLossTmpPkt = NULL; 
        }

        lBuffSize = JBUFF_PKT_ADD_SIZE - lDltaTimestamp; 
        MIN(lBuffSize, pstRtpChl->m_lMaxJBufferMs, lBuffSize); 
        MAX(lBuffSize, pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr], pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr]); 
        if ( pstRtpChl->m_ulCurTicks - pstDecJbuff->m_ulLastPktTicks >= 1000)
        {
            pstDecJbuff->m_lPktCurPtr ++; 
            pstDecJbuff->m_lPktCurPtr &= 7; 

            pstDecJbuff->m_lPktJbuffSize = 0; 
            for (i=0; i< 8; i++)
            {
                MAX(pstDecJbuff->m_lPktJbuffSize, pstDecJbuff->m_alPktJbuffSize[i], pstDecJbuff->m_lPktJbuffSize); 
            }
            pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr] = 0; 
            pstDecJbuff->m_ulLastPktTicks = pstRtpChl->m_ulCurTicks; 
        }
        else 
        {
            MAX(pstDecJbuff->m_lPktJbuffSize, lBuffSize, pstDecJbuff->m_lPktJbuffSize); 
        }

        if ( lDltaSeqNo > 0 )
        {
            LINK_ADD_TAIL(pstDecJbuff->m_lnkJbuffPkt, pstRtpPkt, link0);            
            return RTP_TRUE; 
        }

        return RTP_FALSE; 
    }

    pstTmpPkt1 = LINK_TAIL(pstDecJbuff->m_lnkJbuffPkt);     
    lDltaSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt1->m_lSeqNo); 
    lDltaTimestamp = DLTA_VAL(pstRtpPkt->m_lTimestamp - pstTmpPkt1->m_lTimestamp); 

    if (   ABS(lDltaSeqNo) >= 1024
        || ABS(lDltaTimestamp) >= (pstRtpChl->m_lMaxJBufferMs<<2))
        goto reinit_jbuff; 

    if (pstDecJbuff->m_pstLossTmpPkt )
    {
        RTP_FreeRtpPkt(pstDecJbuff->m_pstLossTmpPkt); 
        pstDecJbuff->m_pstLossTmpPkt = NULL; 
    }

    lBuffSize = JBUFF_PKT_ADD_SIZE - lDltaTimestamp; 
    MIN(lBuffSize, pstRtpChl->m_lMaxJBufferMs, lBuffSize); 
    MAX(lBuffSize, pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr], pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr]); 
    if ( pstRtpChl->m_ulCurTicks - pstDecJbuff->m_ulLastPktTicks >= 1000)
    {
        pstDecJbuff->m_lPktCurPtr ++; 
        pstDecJbuff->m_lPktCurPtr &= 7; 
    
        pstDecJbuff->m_lPktJbuffSize = 0; 
        for (i=0; i< 8; i++)
        {
            MAX(pstDecJbuff->m_lPktJbuffSize, pstDecJbuff->m_alPktJbuffSize[i], pstDecJbuff->m_lPktJbuffSize); 
        }
        pstDecJbuff->m_alPktJbuffSize[pstDecJbuff->m_lPktCurPtr] = 0; 
        pstDecJbuff->m_ulLastPktTicks = pstRtpChl->m_ulCurTicks; 
    }
    else 
    {
        MAX(pstDecJbuff->m_lPktJbuffSize, lBuffSize, pstDecJbuff->m_lPktJbuffSize); 
    }

    if ( lDltaSeqNo > 0 && lDltaTimestamp >= 0 )
    {
        LINK_ADD_TAIL(pstDecJbuff->m_lnkJbuffPkt, pstRtpPkt, link0);  
        return RTP_TRUE; 
    }
    else if ( lDltaSeqNo < 0 && lDltaTimestamp <= 0)
    {
        pstTmpPkt1 = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt);     
        lDltaSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt1->m_lSeqNo); 
        lDltaTimestamp = DLTA_VAL(pstRtpPkt->m_lTimestamp - pstTmpPkt1->m_lTimestamp); 

        if ( lDltaSeqNo < 0 && lDltaTimestamp <= 0 )
        {
            if ( (DLTA_VAL(pstRtpPkt->m_lSeqNo - pstDecJbuff->m_lLastSeqNo) > 0
                && DLTA_VAL(pstRtpPkt->m_lTimestamp - pstDecJbuff->m_lLastTimestamp) >= 0)
                || pstDecJbuff->m_lLastSeqNo == MAX_LONG )
            {
                LINK_ADD_HEAD(pstDecJbuff->m_lnkJbuffPkt, pstRtpPkt, link0); 
                return RTP_TRUE; 
            }
        }
        else if ( lDltaSeqNo > 0 && lDltaTimestamp >= 0 )
        {
            while (1)
            {
                pstTmpPkt2 = LINK_NEXT(pstTmpPkt1, link0); 
                ASSERT(pstTmpPkt2); 

                lDltaSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt2->m_lSeqNo); 
                if ( lDltaSeqNo <= 0)
                {
                    if ( lDltaSeqNo < 0 && DLTA_VAL(pstRtpPkt->m_lSeqNo - pstTmpPkt1->m_lSeqNo) > 0
                        && DLTA_VAL(pstRtpPkt->m_lTimestamp - pstTmpPkt1->m_lTimestamp) >= 0 
                        && DLTA_VAL(pstRtpPkt->m_lTimestamp - pstTmpPkt2->m_lTimestamp) <= 0 )
                    {
                        LINK_INSERT_NEXT(pstDecJbuff->m_lnkJbuffPkt, pstTmpPkt1, pstRtpPkt, link0);  
                        return RTP_TRUE; 
                    }
                    break; 
                }
                pstTmpPkt1 = pstTmpPkt2; 
            }
        }
    }

    return RTP_FALSE; 
}

STATIC LONG  RTP_FecDecodeProcess(ST_DEC_JBUFF * pstDecJbuff, LONG lSeqNo)
{
    ST_RTP_PKT *pstTmpPkt, *pstPktFec1, *pstPktFec2; 
    ST_RTP_PKT *pastPktFec1[20], *pastPktFec2[20]; 
    LONG lSeqNoFec1, lSeqNoFec2, lSeqNumFec1, lSeqNumFec2, lNumFec1, lNumFec2; 
    LONG lMaxSize, lTmp, i, j, lA, lB; 
    UCHAR aucBuffer1[PACKET_BUFFER_SIZE], aucBuffer2[PACKET_BUFFER_SIZE]; 
    CONST UCHAR *pucMultTable, *pucDivTable; 

    pstPktFec1 = pstPktFec2 = NULL; 
    pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt2); 
    while ( pstTmpPkt)
    {
        lTmp = pstTmpPkt->m_aucBuffer[1]&0x7F; 

        if (DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNo) >= 0
            && DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNo) < lTmp )
        {
            if ( pstTmpPkt->m_aucBuffer[1]&0x80)
            {
                pstPktFec2 = pstTmpPkt; 
                lSeqNoFec2 = (pstTmpPkt->m_lSeqNo - lTmp + 1)&0xFFFF; 
                lSeqNumFec2 = lTmp; 
            }
            else 
            {
                pstPktFec1 = pstTmpPkt; 
                lSeqNoFec1 = (pstTmpPkt->m_lSeqNo - lTmp + 1)&0xFFFF; 
                lSeqNumFec1 = lTmp; 
            }
        }
            
        pstTmpPkt = LINK_NEXT(pstTmpPkt, link0); 
    }

    if ( pstPktFec1 == NULL && pstPktFec2 == NULL)
        return 0; 

    if ( pstPktFec1 )
    {
        MEMSET(pastPktFec1, 0, sizeof(pastPktFec1)); 
        lNumFec1 = 0; 

        pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
        while ( pstTmpPkt)
        {
            lTmp = DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNoFec1); 
            if ( lTmp >= lSeqNumFec1)
                goto cont1; 

            if ( lTmp >= 0)
            {
                pastPktFec1[lTmp] = pstTmpPkt; 
                lNumFec1 ++; 
            }

            pstTmpPkt = LINK_NEXT(pstTmpPkt, link0);
        }

        pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
        while ( pstTmpPkt)
        {
            lTmp = DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNoFec1); 
            if ( lTmp >= lSeqNumFec1)
                break; 
        
            if ( lTmp >= 0)
            {
                pastPktFec1[lTmp] = pstTmpPkt; 
                lNumFec1 ++; 
            }
        
            pstTmpPkt = LINK_NEXT(pstTmpPkt, link0);
        }

cont1: 
        ASSERT(lNumFec1 < lSeqNumFec1); 
        if ( lNumFec1 < lSeqNumFec1-2)
            return 0; 

        if ( lNumFec1 == lSeqNumFec1 -1)
        {
            lTmp = DLTA_VAL(lSeqNo - lSeqNoFec1); 
            ASSERT(pastPktFec1[lTmp] == NULL); 

            aucBuffer1[2] = pstPktFec1->m_aucBuffer[2]; 
            aucBuffer1[3] = pstPktFec1->m_aucBuffer[3]; 

            lMaxSize = pstPktFec1->m_lDataLen - 4; 
            MIN(lMaxSize, PACKET_BUFFER_SIZE-4, lMaxSize); 
            MEMCPY(aucBuffer1+4, pstPktFec1->m_aucBuffer+4, lMaxSize);             

            for ( i=0; i< lSeqNumFec1; i++)
            {
                pstTmpPkt = pastPktFec1[i]; 
                if ( pstTmpPkt == NULL)
                    continue; 

                lTmp = pstTmpPkt->m_lDataLen; 
                aucBuffer1[2] ^= (UCHAR)(lTmp>>8  ); 
                aucBuffer1[3] ^= (UCHAR)(lTmp&0xFF); 

                MIN(lTmp, lMaxSize, lTmp); 
                for ( j=0; j< lTmp-3; j += 4)
                    *(ULONG *)&aucBuffer1[4+j] ^= *(ULONG *)&pstTmpPkt->m_aucBuffer[j]; 
                while ( j< lTmp)
                {
                    aucBuffer1[4+j] ^= pstTmpPkt->m_aucBuffer[j]; 
                    j++; 
                }
            }

            lTmp = (aucBuffer1[2]<<8)+aucBuffer1[3]; 
            MIN(lTmp, lMaxSize, lTmp);

            pstTmpPkt = RTP_CreateRtpPkt(aucBuffer1+4, lTmp); 
            if ( pstTmpPkt == 0)
                return 0; 

            pstTmpPkt->m_lSeqNo = lSeqNo; 
            pstTmpPkt->m_lTimestamp = pstPktFec1->m_lTimestamp;
            pstTmpPkt->m_lSsrc = pstDecJbuff->m_lSsrc; 
            pstTmpPkt->m_lPt = pstDecJbuff->m_lPt; 
            
            LINK_ADD_HEAD(pstDecJbuff->m_lnkJbuffPkt, pstTmpPkt, link0); 
            return 1; 
        }
    }

    if ( pstPktFec2 )
    {
        MEMSET(pastPktFec2, 0, sizeof(pastPktFec2)); 
        lNumFec2 = 0; 

        pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
        while ( pstTmpPkt)
        {
            lTmp = DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNoFec2); 
            if ( lTmp >= lSeqNumFec2)
                goto cont2; 

            if ( lTmp >= 0)
            {
                pastPktFec2[lTmp] = pstTmpPkt; 
                lNumFec2 ++; 
            }

            pstTmpPkt = LINK_NEXT(pstTmpPkt, link0);
        }

        pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
        while ( pstTmpPkt)
        {
            lTmp = DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNoFec2); 
            if ( lTmp >= lSeqNumFec2)
                break; 
        
            if ( lTmp >= 0)
            {
                pastPktFec2[lTmp] = pstTmpPkt; 
                lNumFec2 ++; 
            }
        
            pstTmpPkt = LINK_NEXT(pstTmpPkt, link0);
        }

cont2: 
        ASSERT(lNumFec2 < lSeqNumFec2); 
        if ( lNumFec2 < lSeqNumFec2-2)
            return 0; 

        if ( lNumFec2 == lSeqNumFec2 -1)
        {
            lTmp = DLTA_VAL(lSeqNo - lSeqNoFec2); 
            ASSERT(pastPktFec2[lTmp] == NULL); 
            pucDivTable = m_aucRtpRsDiv[lTmp]; 

            aucBuffer1[2] = pstPktFec2->m_aucBuffer[2]; 
            aucBuffer1[3] = pstPktFec2->m_aucBuffer[3]; 

            lMaxSize = pstPktFec2->m_lDataLen - 4; 
            MIN(lMaxSize, PACKET_BUFFER_SIZE-4, lMaxSize); 
            MEMCPY(aucBuffer1+4, pstPktFec2->m_aucBuffer+4, lMaxSize);             

            for ( i=0; i< lSeqNumFec2; i++)
            {
                pstTmpPkt = pastPktFec2[i]; 
                if ( pstTmpPkt == NULL)
                    continue; 

                pucMultTable = m_aucRtpRsMult[i]; 

                lTmp = pstTmpPkt->m_lDataLen; 
                aucBuffer1[2] ^= pucMultTable[lTmp>>8  ]; 
                aucBuffer1[3] ^= pucMultTable[lTmp&0xFF]; 

                MIN(lTmp, lMaxSize, lTmp); 
                for ( j=0; j< lTmp; j ++)
                    aucBuffer1[4+j] ^= pucMultTable[pstTmpPkt->m_aucBuffer[j]]; 
            }

            aucBuffer1[2] = pucDivTable[aucBuffer1[2]]; 
            aucBuffer1[3] = pucDivTable[aucBuffer1[3]]; 

            lTmp = (aucBuffer1[2]<<8)+aucBuffer1[3]; 
            MIN(lTmp, lMaxSize, lTmp); 

            for ( j=4; j< lTmp+4; j++)
                aucBuffer1[j] = pucDivTable[aucBuffer1[j]]; 

            pstTmpPkt = RTP_CreateRtpPkt(aucBuffer1+4, lTmp); 
            if ( pstTmpPkt == 0)
                return 0; 

            pstTmpPkt->m_lSeqNo = lSeqNo; 
            pstTmpPkt->m_lTimestamp = pstPktFec2->m_lTimestamp;
            pstTmpPkt->m_lSsrc = pstDecJbuff->m_lSsrc;             
            pstTmpPkt->m_lPt = pstDecJbuff->m_lPt; 
            
            LINK_ADD_HEAD(pstDecJbuff->m_lnkJbuffPkt, pstTmpPkt, link0); 
            return 1; 
        }
    }

    if (!pstPktFec1 || !pstPktFec2)
        return 0; 

    ASSERT( lNumFec1 == lSeqNumFec1 -2); 
    ASSERT( lNumFec2 == lSeqNumFec2 -2); 

    for ( i=0; i< lSeqNumFec1; i++)
    {
        if ( pastPktFec1[i] == NULL)
        {
            j = (i + DLTA_VAL(lSeqNoFec1 - lSeqNoFec2)); 
            if ( pastPktFec2[j])
                return 0; 
        }
    }
   
    aucBuffer1[2] = pstPktFec1->m_aucBuffer[2]; 
    aucBuffer1[3] = pstPktFec1->m_aucBuffer[3]; 
    aucBuffer2[2] = pstPktFec2->m_aucBuffer[2]; 
    aucBuffer2[3] = pstPktFec2->m_aucBuffer[3]; 

    MAX(pstPktFec1->m_lDataLen, pstPktFec2->m_lDataLen, lMaxSize); 
    MIN(lMaxSize, PACKET_BUFFER_SIZE, lMaxSize); 
    MEMCPY(aucBuffer1+4, pstPktFec1->m_aucBuffer+4, pstPktFec1->m_lDataLen-4); 
    if ( pstPktFec1->m_lDataLen < lMaxSize)
    {
        MEMSET(aucBuffer1+pstPktFec1->m_lDataLen, 0, lMaxSize-pstPktFec1->m_lDataLen);
    }
    MEMCPY(aucBuffer2+4, pstPktFec2->m_aucBuffer+4, pstPktFec2->m_lDataLen-4); 
    if ( pstPktFec2->m_lDataLen < lMaxSize)
    {
        MEMSET(aucBuffer2+pstPktFec2->m_lDataLen, 0, lMaxSize-pstPktFec2->m_lDataLen);
    }
    lMaxSize -= 4; 
    
    for ( i=0; i< lSeqNumFec1; i++)
    {
        pstTmpPkt = pastPktFec1[i]; 
        if ( pstTmpPkt == NULL)
            continue; 
    
        lTmp = pstTmpPkt->m_lDataLen; 
        aucBuffer1[2] ^= (UCHAR)(lTmp>>8  ); 
        aucBuffer1[3] ^= (UCHAR)(lTmp&0xFF); 
    
        MIN(lTmp, lMaxSize, lTmp); 
        for ( j=0; j< lTmp-3; j += 4)
            *(ULONG *)&aucBuffer1[4+j] ^= *(ULONG *)&pstTmpPkt->m_aucBuffer[j]; 
        while ( j< lTmp)
        {
            aucBuffer1[4+j] ^= pstTmpPkt->m_aucBuffer[j]; 
            j++; 
        }
    }

    lA = lB = MAX_LONG; 
    for ( i=0; i< lSeqNumFec2; i++)
    {
        pstTmpPkt = pastPktFec2[i]; 
        if ( pstTmpPkt == NULL)
        {
            if ( i == DLTA_VAL(lSeqNo - lSeqNoFec2))
                lA = i; 
            else 
                lB = i; 
            continue; 
        }

        pucMultTable = m_aucRtpRsMult[i]; 

        lTmp = pstTmpPkt->m_lDataLen; 
        aucBuffer2[2] ^= pucMultTable[lTmp>>8  ]; 
        aucBuffer2[3] ^= pucMultTable[lTmp&0xFF]; 

        MIN(lTmp, lMaxSize, lTmp); 
        for ( j=0; j< lTmp; j ++)
            aucBuffer2[4+j] ^= pucMultTable[pstTmpPkt->m_aucBuffer[j]]; 
    }
    ASSERT(lA < MAX_LONG && lB < MAX_LONG); 

    pucMultTable = m_aucRtpRsMult[lB]; 
    pucDivTable = m_aucRtpRsDiv[m_aucRtpRsAdd[lA][lB]]; 

    aucBuffer1[2] = pucDivTable[aucBuffer2[2] ^ pucMultTable[aucBuffer1[2]]]; 
    aucBuffer1[3] = pucDivTable[aucBuffer2[3] ^ pucMultTable[aucBuffer1[3]]]; 
    
    lTmp = (aucBuffer1[2]<<8)+aucBuffer1[3]; 
    MIN(lTmp, lMaxSize, lTmp); 
    
    for ( j=4; j< lTmp+4; j++)
        aucBuffer1[j] = pucDivTable[aucBuffer2[j] ^ pucMultTable[aucBuffer1[j]]]; 

    pstTmpPkt = RTP_CreateRtpPkt(aucBuffer1+4, lTmp); 
    if ( pstTmpPkt == 0)
        return 0; 

    pstTmpPkt->m_lSeqNo = lSeqNo; 
    pstTmpPkt->m_lTimestamp = pstPktFec2->m_lTimestamp;
    pstTmpPkt->m_lSsrc = pstDecJbuff->m_lSsrc; 
    pstTmpPkt->m_lPt = pstDecJbuff->m_lPt; 

    LINK_ADD_HEAD(pstDecJbuff->m_lnkJbuffPkt, pstTmpPkt, link0); 
    return 1; 
}

STATIC VOID RTP_DecodeOputHpmp(ST_RTP_DEC_CHL * pstRtpChl, BOOL bErrInd)
{
    if ( pstRtpChl->m_stHpmp.m_lDataType == HPMP_DATA_VIDEO)
    {
        pstRtpChl->m_stHpmp.m_pucBuffer[0] = 0; 
        pstRtpChl->m_stHpmp.m_pucBuffer[1] = 0; 
        pstRtpChl->m_stHpmp.m_pucBuffer[2] = 0; 
        pstRtpChl->m_stHpmp.m_pucBuffer[3] = 1; 

        if ( pstRtpChl->m_pfnRtpDecodeVideoOput)
            pstRtpChl->m_pfnRtpDecodeVideoOput(pstRtpChl->m_pVideoUserData, (unsigned char*)pstRtpChl->m_stHpmp.m_pucBuffer, pstRtpChl->m_stHpmp.m_lDataLen+4, bErrInd); 
    }
    else if ( pstRtpChl->m_stHpmp.m_lDataType == HPMP_DATA_AUDIO)
    {
        if ( pstRtpChl->m_pfnRtpDecodeAudioOput)
            pstRtpChl->m_pfnRtpDecodeAudioOput(pstRtpChl->m_pAudioUserData, (unsigned char*)pstRtpChl->m_stHpmp.m_pucBuffer+4, pstRtpChl->m_stHpmp.m_lDataLen); 
    }
    else 
    {
    }
}

STATIC VOID RTP_DecodeOput(ST_RTP_DEC_CHL * pstRtpChl, ST_RTP_PKT * pstRtpPkt)
{
    UCHAR * pucData; 
    LONG i, lStartOffset, lLastSeqNo, lDataLen; 
    
    if ( pstRtpChl->m_lRtpMode == RTP_STRM_RTP)
    {
        *(UCHAR*)(pstRtpPkt->m_aucBuffer - 12) = (UCHAR)0x80; 
        *(UCHAR*)(pstRtpPkt->m_aucBuffer - 11) = (UCHAR)pstRtpPkt->m_lPt; 
        *(USHORT *)(pstRtpPkt->m_aucBuffer-10) = NTOA_W(pstRtpPkt->m_lSeqNo); 
        *(ULONG  *)(pstRtpPkt->m_aucBuffer-8 ) = NTOA_L(pstRtpPkt->m_lTimestamp); 
        *(ULONG  *)(pstRtpPkt->m_aucBuffer-4 ) = NTOA_L(pstRtpPkt->m_lSsrc); 

        if ( pstRtpPkt->m_lPt == pstRtpChl->m_stRtp.m_lVideoPt)
        {
            pstRtpChl->m_usVideoRecvSeqNo = (USHORT)pstRtpPkt->m_lSeqNo; 
            if ( pstRtpChl->m_pfnRtpDecodeVideoOput)
                pstRtpChl->m_pfnRtpDecodeVideoOput(pstRtpChl->m_pVideoUserData, pstRtpPkt->m_aucBuffer, pstRtpPkt->m_lDataLen, 0); 
        }
        else 
        {
            pstRtpChl->m_usAudioRecvSeqNo = (USHORT)pstRtpPkt->m_lSeqNo; 
            if ( pstRtpChl->m_pfnRtpDecodeAudioOput)
                pstRtpChl->m_pfnRtpDecodeAudioOput(pstRtpChl->m_pAudioUserData, pstRtpPkt->m_aucBuffer, pstRtpPkt->m_lDataLen); 
        }
    }
    else 
    {
        pstRtpChl->m_usVideoRecvSeqNo = 
        pstRtpChl->m_usAudioRecvSeqNo = (USHORT)pstRtpPkt->m_lSeqNo; 

        pucData = pstRtpPkt->m_aucBuffer+2; 
        lDataLen = pstRtpPkt->m_lDataLen-2; 
        lLastSeqNo = pstRtpChl->m_stHpmp.m_lLastSeqNo; 
        pstRtpChl->m_stHpmp.m_lLastSeqNo = pstRtpPkt->m_lSeqNo; 

        i = (pstRtpPkt->m_aucBuffer[0]<<8)+pstRtpPkt->m_aucBuffer[1]; 
        lStartOffset = i>>4; 
        
        if ( pstRtpChl->m_stHpmp.m_pucBuffer)
        {
            if (DLTA_VAL(pstRtpPkt->m_lSeqNo - lLastSeqNo) != 1)
            {
cont: 
                RTP_DecodeOputHpmp(pstRtpChl, 1); 

                FREE(pstRtpChl->m_stHpmp.m_pucBuffer); 
                pstRtpChl->m_stHpmp.m_pucBuffer = NULL; 

                pucData += lStartOffset; 
                lDataLen -= lStartOffset; 
            }
            else 
            {
                i = pstRtpChl->m_stHpmp.m_lDataTotalLen - pstRtpChl->m_stHpmp.m_lDataLen;                    
                if ( i > lDataLen )
                    i = lDataLen; 
                if ( lStartOffset < i)
                    goto cont; 

                MEMCPY(pstRtpChl->m_stHpmp.m_pucBuffer + 4 + pstRtpChl->m_stHpmp.m_lDataLen, pucData, i); 
                pstRtpChl->m_stHpmp.m_lDataLen += i; 
                pucData += i; 
                lDataLen -= i; 

                if ( pstRtpChl->m_stHpmp.m_lDataLen < pstRtpChl->m_stHpmp.m_lDataTotalLen)
                    return; 
                ASSERT(pstRtpChl->m_stHpmp.m_lDataLen == pstRtpChl->m_stHpmp.m_lDataTotalLen); 
                
                RTP_DecodeOputHpmp(pstRtpChl, 0); 
                
                FREE(pstRtpChl->m_stHpmp.m_pucBuffer); 
                pstRtpChl->m_stHpmp.m_pucBuffer = NULL; 
            }            
        }
        else 
        {
            pucData += lStartOffset; 
            lDataLen -= lStartOffset; 
        }
        if ( lDataLen <= 0)
            return; 

        while (1)
        {
            if ( lDataLen < 4)
                return; 

            pstRtpChl->m_stHpmp.m_lDataTimestamp = pstRtpPkt->m_lTimestamp; 
            pstRtpChl->m_stHpmp.m_lDataType = pucData[0]; 
            pstRtpChl->m_stHpmp.m_lDataTotalLen = (pucData[1]<<16)+(pucData[2]<<8)+pucData[3]; 
            if ( pstRtpChl->m_stHpmp.m_lDataTotalLen > 2000000)
                return; 
            pstRtpChl->m_stHpmp.m_pucBuffer = MALLOC(pstRtpChl->m_stHpmp.m_lDataTotalLen+4); 
            if ( pstRtpChl->m_stHpmp.m_pucBuffer == NULL)
                return; 

            pucData += 4; 
            lDataLen -= 4; 

            i = pstRtpChl->m_stHpmp.m_lDataTotalLen; 
            if ( i > lDataLen)
                i = lDataLen; 
            
            MEMCPY(pstRtpChl->m_stHpmp.m_pucBuffer + 4, pucData, i); 
            
            pstRtpChl->m_stHpmp.m_lDataLen = i; 
            pucData += i; 
            lDataLen -= i; 

            if ( pstRtpChl->m_stHpmp.m_lDataLen < pstRtpChl->m_stHpmp.m_lDataTotalLen)
                return; 

            RTP_DecodeOputHpmp(pstRtpChl, 0); 
            
            FREE(pstRtpChl->m_stHpmp.m_pucBuffer); 
            pstRtpChl->m_stHpmp.m_pucBuffer = NULL; 
        }; 
    }
}

STATIC VOID RTP_DecodeProcess(ST_RTP_DEC_CHL * pstRtpChl, ST_DEC_JBUFF * pstDecJbuff)
{
    ST_RTP_PKT *pstRtpPkt, *pstTmpPkt; 
    LONG lStartTimestamp, lSeqNo, lTmp, lMin, lMax, i; 

    if ( pstDecJbuff->m_pstLossTmpPkt )
    {
        if (pstRtpChl->m_ulCurTicks - pstDecJbuff->m_pstLossTmpPkt->m_ulReceiveTicks >= 1000)
        {
            RTP_FreeRtpPkt(pstDecJbuff->m_pstLossTmpPkt); 
            pstDecJbuff->m_pstLossTmpPkt = NULL; 
        }
    }
    
cont: 
    pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
    if ( pstRtpPkt )
    {
        pstTmpPkt = LINK_TAIL(pstDecJbuff->m_lnkJbuffPkt); 
        ASSERT(pstTmpPkt); 

        lTmp = DLTA_VAL(pstTmpPkt->m_lTimestamp - pstDecJbuff->m_lCurTimestamp); 
        if ( lTmp > 100 || lTmp < -100)
            pstDecJbuff->m_lCurTimestamp = pstTmpPkt->m_lTimestamp; 

        lStartTimestamp = pstDecJbuff->m_lCurTimestamp - pstRtpChl->m_lJbuffSize; 
        do 
        {
            if ( DLTA_VAL(pstRtpPkt->m_lTimestamp - lStartTimestamp) > 0 )
                break; 

            if ( DLTA_VAL(pstRtpPkt->m_lSeqNo - pstDecJbuff->m_lLastSeqNo) != 1
                && pstDecJbuff->m_lLastSeqNo != MAX_LONG )
            {
                if ( 0 == (RTP_FecDecodeProcess(pstDecJbuff, (pstDecJbuff->m_lLastSeqNo+1)&0xFFFF)))
                {
                    if ( pstRtpChl->m_lJbuffSize < pstDecJbuff->m_lPktJbuffSize)
                    {
                        pstRtpChl->m_lJbuffSize += 10; 
                        break; 
                    }
                    if ( pstRtpChl->m_lJbuffSize < pstDecJbuff->m_lFecJbuffSize)
                    {
                        pstRtpChl->m_lJbuffSize += 10; 
                        break; 
                    }
                }
                else 
                {
                    pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
                    ASSERT(pstRtpPkt); 
                }
            }

            RTP_DecodeOput(pstRtpChl, pstRtpPkt); 
            
            pstDecJbuff->m_lLastSeqNo = pstRtpPkt->m_lSeqNo; 
            pstDecJbuff->m_lLastTimestamp = pstRtpPkt->m_lTimestamp; 

            LINK_REMOVE_HEAD(pstDecJbuff->m_lnkJbuffPkt, link0); 
            LINK_ADD_TAIL(pstDecJbuff->m_lnkFecPkt1, pstRtpPkt, link0);

            pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
        } while (pstRtpPkt); 
    }

    if ( pstDecJbuff->m_lLastSeqNo == MAX_LONG)
    {
        ASSERT(LINK_NODE_NUM(pstDecJbuff->m_lnkFecPkt1) == 0); 
        if ( LINK_NODE_NUM(pstDecJbuff->m_lnkJbuffPkt) == 0)
        {
            while ( LINK_NODE_NUM(pstDecJbuff->m_lnkFecPkt2) >= 4)
            {
                pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt2);
                ASSERT(pstRtpPkt); 
                
                if ( 1 == RTP_FecDecodeProcess(pstDecJbuff, pstRtpPkt->m_lSeqNo))
                    goto cont; 
                
                LINK_REMOVE_HEAD(pstDecJbuff->m_lnkFecPkt2, link0); 
                RTP_FreeRtpPkt(pstRtpPkt); 

                pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt2); 
            }
        }
    }
    else 
    {
        pstRtpPkt = LINK_TAIL(pstDecJbuff->m_lnkFecPkt2); 
        while ( pstRtpPkt )
        {
            lSeqNo = DLTA_VAL(pstRtpPkt->m_lSeqNo - pstDecJbuff->m_lLastSeqNo); 
            if ( lSeqNo >= 256)
            {
                pstTmpPkt = pstRtpPkt; 
                pstRtpPkt = LINK_PREV(pstRtpPkt, link0); 

                LINK_REMOVE_NODE(pstDecJbuff->m_lnkFecPkt2, pstTmpPkt, link0); 
                RTP_FreeRtpPkt(pstTmpPkt); 
                continue; 
            }
            
            pstRtpPkt = LINK_PREV(pstRtpPkt, link0);             
            if ( lSeqNo <= 0 )
                break; 
        }

        if ( pstRtpPkt)
        {
            lSeqNo = (pstRtpPkt->m_lSeqNo - (pstRtpPkt->m_aucBuffer[1]&0x7F)+1)&0xFFFF; 
            lMin = lMax = 0; 

            pstTmpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
            while ( pstTmpPkt)
            {
                if (DLTA_VAL(pstTmpPkt->m_lSeqNo - pstRtpPkt->m_lSeqNo) > 0)
                    break; 
                if (DLTA_VAL(pstTmpPkt->m_lSeqNo - lSeqNo) >= 0)
                {
                    lTmp = pstRtpPkt->m_ulReceiveTicks - pstTmpPkt->m_ulReceiveTicks; 
                    MIN(lTmp, lMin, lMin); 
                    MAX(lTmp, lMax, lMax); 
                }
                pstTmpPkt = LINK_NEXT(pstTmpPkt, link0); 
            }

            lTmp = (lMax-lMin) + JBUFF_FEC_ADD_SIZE; 
            MIN(lTmp, pstRtpChl->m_lMaxJBufferMs, lTmp); 

            MAX(lTmp, pstDecJbuff->m_alFecJbuffSize[pstDecJbuff->m_lFecCurPtr], pstDecJbuff->m_alFecJbuffSize[pstDecJbuff->m_lFecCurPtr]); 
            if ( pstRtpChl->m_ulCurTicks - pstDecJbuff->m_ulLastFecTicks >= 1000)
            {
                pstDecJbuff->m_lFecCurPtr ++; 
                pstDecJbuff->m_lFecCurPtr &= 7; 
            
                pstDecJbuff->m_lFecJbuffSize = 0; 
                for (i=0; i< 8; i++)
                {
                    MAX(pstDecJbuff->m_lFecJbuffSize, pstDecJbuff->m_alFecJbuffSize[i], pstDecJbuff->m_lFecJbuffSize); 
                }
                pstDecJbuff->m_alFecJbuffSize[pstDecJbuff->m_lFecCurPtr] = 0; 
                pstDecJbuff->m_ulLastFecTicks = pstRtpChl->m_ulCurTicks; 
            }
            else 
            {
                MAX(pstDecJbuff->m_lFecJbuffSize, lTmp, pstDecJbuff->m_lFecJbuffSize); 
            }

            lSeqNo = pstRtpPkt->m_lSeqNo; 
            do 
            {
                pstTmpPkt = pstRtpPkt; 
                pstRtpPkt = LINK_PREV(pstRtpPkt, link0); 

                LINK_REMOVE_NODE(pstDecJbuff->m_lnkFecPkt2, pstTmpPkt, link0); 
                RTP_FreeRtpPkt(pstTmpPkt); 
            } while ( pstRtpPkt ); 
        }
        else 
        {
            lSeqNo = (pstDecJbuff->m_lLastSeqNo - 20)&0xFFFF; 
        }

        pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
        while ( pstRtpPkt)
        {
            if (DLTA_VAL(pstRtpPkt->m_lSeqNo - lSeqNo) > 0)
                break; 

            LINK_REMOVE_HEAD(pstDecJbuff->m_lnkFecPkt1, link0); 
            RTP_FreeRtpPkt(pstRtpPkt); 

            pstRtpPkt = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1);                 
        }

        if ( LINK_NODE_NUM(pstDecJbuff->m_lnkJbuffPkt) == 0)
        {
            if ( LINK_NODE_NUM(pstDecJbuff->m_lnkFecPkt2) >= 2)
            {              
                if ( 1 == RTP_FecDecodeProcess(pstDecJbuff, (pstDecJbuff->m_lLastSeqNo+1)&0xFFFF))
                    goto cont; 

                if ( LINK_NODE_NUM(pstDecJbuff->m_lnkFecPkt2) >= 4)
                    pstDecJbuff->m_lLastSeqNo = (pstDecJbuff->m_lLastSeqNo+1)&0xFFFF; 
            }
        }
    }
}

void *  RTP_CreateDecodeChl(int strmmode, long maxjbuffms)
{
    ST_RTP_DEC_CHL * pstRtpChl; 

    LOCK_DEC(); 
    if ( m_stRtpCB.m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_DEC(); 
        return 0; 
    }

    pstRtpChl = (ST_RTP_DEC_CHL *)MALLOC(sizeof(ST_RTP_DEC_CHL)); 
    if ( pstRtpChl == 0)
    {
        UNLOCK_DEC(); 
        return 0; 
    }
    MEMSET(pstRtpChl, 0, sizeof(ST_RTP_DEC_CHL)); 

    pstRtpChl->m_lRtpMode = strmmode; 
    if ( maxjbuffms < 200)
        maxjbuffms = 200; 
    if ( maxjbuffms > 2000)
        maxjbuffms = 2000;
    
    pstRtpChl->m_lMaxJBufferMs = maxjbuffms;
    if ( pstRtpChl->m_lRtpMode == RTP_STRM_RTP)
    {
        INIT_DEC_JBUFF(pstRtpChl->m_stRtp.m_stVideoJbuffer, pstRtpChl->m_lMaxJBufferMs); 
        INIT_DEC_JBUFF(pstRtpChl->m_stRtp.m_stAudioJbuffer, pstRtpChl->m_lMaxJBufferMs); 
    }
    else 
    {
        pstRtpChl->m_lRtpMode = RTP_STRM_HPMP; 
        INIT_DEC_JBUFF(pstRtpChl->m_stHpmp.m_stStreamJbuffer, pstRtpChl->m_lMaxJBufferMs); 
    }
    
    pstRtpChl->m_ulCurTicks = CUR_TICKS(); 
    
    pstRtpChl->m_lJbuffSize = JBUFF_INIT_SIZE; 
    pstRtpChl->m_ulLastDecJbuffTicks = pstRtpChl->m_ulCurTicks; 

    LINK_ADD_TAIL(m_stRtpCB.m_lnkRtpDecChl, pstRtpChl, link0); 
    pstRtpChl->m_ulState = RTP_STATE_INUSE; 

    UNLOCK_DEC(); 
    return pstRtpChl; 
}

STATIC VOID RTP_FreeDecodeJbuff(ST_DEC_JBUFF * pstDecJbuff)
{
    ST_RTP_PKT * pstRtpPkt1, *pstRtpPkt2; 

    pstRtpPkt1 = LINK_HEAD(pstDecJbuff->m_lnkJbuffPkt); 
    while ( pstRtpPkt1)
    {
        pstRtpPkt2 = pstRtpPkt1; 
        pstRtpPkt1 = LINK_NEXT(pstRtpPkt1, link0); 
        RTP_FreeRtpPkt(pstRtpPkt2); 
    }

    pstRtpPkt1 = LINK_HEAD(pstDecJbuff->m_lnkFecPkt1); 
    while ( pstRtpPkt1)
    {
        pstRtpPkt2 = pstRtpPkt1; 
        pstRtpPkt1 = LINK_NEXT(pstRtpPkt1, link0); 
        RTP_FreeRtpPkt(pstRtpPkt2); 
    }

    pstRtpPkt1 = LINK_HEAD(pstDecJbuff->m_lnkFecPkt2); 
    while ( pstRtpPkt1)
    {
        pstRtpPkt2 = pstRtpPkt1; 
        pstRtpPkt1 = LINK_NEXT(pstRtpPkt1, link0); 
        RTP_FreeRtpPkt(pstRtpPkt2); 
    }

    if ( pstDecJbuff->m_pstLossTmpPkt)
        RTP_FreeRtpPkt(pstDecJbuff->m_pstLossTmpPkt); 
}

void    RTP_DeleteDecodeChl(void * rtp)
{
    ST_RTP_DEC_CHL * pstRtpChl = (ST_RTP_DEC_CHL *)rtp; 

    LOCK_DEC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_DEC(); 
        return; 
    }

    if ( pstRtpChl->m_lRtpMode == RTP_STRM_RTP)
    {
        RTP_FreeDecodeJbuff(&pstRtpChl->m_stRtp.m_stVideoJbuffer); 
        RTP_FreeDecodeJbuff(&pstRtpChl->m_stRtp.m_stAudioJbuffer); 
    }
    else 
    {
        RTP_FreeDecodeJbuff(&pstRtpChl->m_stHpmp.m_stStreamJbuffer); 
        if ( pstRtpChl->m_stHpmp.m_pucBuffer)
            FREE(pstRtpChl->m_stHpmp.m_pucBuffer); 
    }

    LINK_REMOVE_NODE(m_stRtpCB.m_lnkRtpDecChl, pstRtpChl, link0); 
    pstRtpChl->m_ulState = 0; 
    FREE(pstRtpChl); 

    UNLOCK_DEC(); 
    return; 
}

void    RTP_DecodeInput(void * rtp, unsigned char * data, long len)
{
    ST_RTP_DEC_CHL * pstRtpChl = (ST_RTP_DEC_CHL *)rtp; 
    ST_RTP_PKT * pstRtpPkt; 
    BOOL bResult; 
    
    LOCK_DEC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE)
    {
        UNLOCK_DEC(); 
        return; 
    }

    if ( pstRtpChl->m_lRtpMode == RTP_STRM_RTP)
    {
        pstRtpPkt = RTP_DecodeRtpPkt(data, len);
        if (!pstRtpPkt)
            goto end; 

        if ( pstRtpPkt->m_lPt == pstRtpChl->m_stRtp.m_lVideoPt)
        {
            pstRtpPkt->m_lTimestamp /= 90; 
            bResult = RTP_DecodeInputPkt(pstRtpChl, &pstRtpChl->m_stRtp.m_stVideoJbuffer, pstRtpPkt);
        }
        else if ( pstRtpPkt->m_lPt == pstRtpChl->m_stRtp.m_lVideoRedPt)
        {
            pstRtpPkt->m_lTimestamp /= 90; 
            bResult = RTP_DecodeInputRed(&pstRtpChl->m_stRtp.m_stVideoJbuffer, pstRtpPkt);
        }
        else if ( pstRtpPkt->m_lPt == pstRtpChl->m_stRtp.m_lAudioPt)
        {
            bResult = RTP_DecodeInputPkt(pstRtpChl, &pstRtpChl->m_stRtp.m_stAudioJbuffer, pstRtpPkt);
        }
        else if ( pstRtpPkt->m_lPt == pstRtpChl->m_stRtp.m_lAudioRedPt)
        {
            bResult = RTP_DecodeInputRed(&pstRtpChl->m_stRtp.m_stAudioJbuffer, pstRtpPkt);
        }
        else 
        {
            bResult = RTP_FALSE; 
        }

        if (!bResult)
            RTP_FreeRtpPkt(pstRtpPkt); 
    }
    else 
    {
        pstRtpPkt = RTP_DecodeHpmpPkt(data, len);
        if (!pstRtpPkt)
            goto end; 

        if ( pstRtpPkt->m_lRedPkt)
        {
            bResult = RTP_DecodeInputRed(&pstRtpChl->m_stHpmp.m_stStreamJbuffer, pstRtpPkt); 
        }
        else 
        {
            bResult = RTP_DecodeInputPkt(pstRtpChl, &pstRtpChl->m_stHpmp.m_stStreamJbuffer, pstRtpPkt); 
        }

        if (!bResult)
            RTP_FreeRtpPkt(pstRtpPkt); 
    }

end: 
    UNLOCK_DEC(); 
}

void    RTP_SetDecodeVideoParam(void * rtp, long pt, long redpt, ON_RTP_DECODE_VIDEO_OPUT pfnRtpDecodeVideoOput, void * puserdata)
{
    ST_RTP_DEC_CHL * pstRtpChl = (ST_RTP_DEC_CHL *)rtp; 

    LOCK_DEC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_RTP)
    {
        UNLOCK_DEC(); 
        return; 
    }

    pstRtpChl->m_stRtp.m_lVideoPt = pt; 
    pstRtpChl->m_stRtp.m_lVideoRedPt = redpt; 
    pstRtpChl->m_pfnRtpDecodeVideoOput = pfnRtpDecodeVideoOput; 
    pstRtpChl->m_pVideoUserData = puserdata; 
    pstRtpChl->m_stRtp.m_stVideoJbuffer.m_lPt = pt; 

    UNLOCK_DEC(); 
}

void    RTP_SetDecodeAudioParam(void * rtp, long pt, long redpt, ON_RTP_DECODE_AUDIO_OPUT pfnRtpDecodeAudioOput, void * puserdata)
{
    ST_RTP_DEC_CHL * pstRtpChl = (ST_RTP_DEC_CHL *)rtp; 

    LOCK_DEC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_RTP)
    {
        UNLOCK_DEC(); 
        return; 
    }

    pstRtpChl->m_stRtp.m_lAudioPt = pt; 
    pstRtpChl->m_stRtp.m_lAudioRedPt = redpt; 
    pstRtpChl->m_pfnRtpDecodeAudioOput = pfnRtpDecodeAudioOput; 
    pstRtpChl->m_pAudioUserData = puserdata; 
    pstRtpChl->m_stRtp.m_stAudioJbuffer.m_lPt = pt; 

    UNLOCK_DEC(); 
}

void    RTP_SetDecodeParam(void * rtp, ON_RTP_DECODE_VIDEO_OPUT pfnRtpDecodeVideoOput, void * puserdatavideo, ON_RTP_DECODE_AUDIO_OPUT pfnRtpDecodeAudioOput, void * puserdataaudio)
{
    ST_RTP_DEC_CHL * pstRtpChl = (ST_RTP_DEC_CHL *)rtp; 

    LOCK_DEC(); 
    if (!pstRtpChl || m_stRtpCB.m_ulState != RTP_STATE_INUSE
        || pstRtpChl->m_ulState != RTP_STATE_INUSE || pstRtpChl->m_lRtpMode != RTP_STRM_HPMP)
    {
        UNLOCK_DEC(); 
        return; 
    }

    pstRtpChl->m_pfnRtpDecodeVideoOput = pfnRtpDecodeVideoOput; 
    pstRtpChl->m_pVideoUserData = puserdatavideo; 
    pstRtpChl->m_pfnRtpDecodeAudioOput = pfnRtpDecodeAudioOput; 
    pstRtpChl->m_pAudioUserData = puserdataaudio; 
    pstRtpChl->m_stHpmp.m_stStreamJbuffer.m_lPt = 0; 

    UNLOCK_DEC(); 
}

VOID RTP_Schd()
{
    ST_RTP_ENC_CHL * pstEncChl; 
    ST_RTP_DEC_CHL * pstDecChl;
    ULONG ulCurTicks; 
    LONG i;

    if (m_stRtpCB.m_ulState != RTP_STATE_INUSE)
        return; 

    ulCurTicks = CUR_TICKS(); 

    LOCK_ENC(); 
    pstEncChl = LINK_HEAD(m_stRtpCB.m_lnkRtpEncChl); 
    while ( pstEncChl)
    {
        if ( pstEncChl->m_lRtpMode == RTP_STRM_HPMP)
        {
            if ( pstEncChl->m_stHpmp.m_lCurDataLen > 0)
            {
                if (ulCurTicks - pstEncChl->m_stHpmp.m_ulLastSendTicks >= 20)
                {
                    RTP_EncodeProcessHpmpPkt(pstEncChl); 
                }
            }
        }

        pstEncChl = LINK_NEXT(pstEncChl, link0); 
    }
    UNLOCK_ENC(); 

    LOCK_DEC(); 
    pstDecChl = LINK_HEAD(m_stRtpCB.m_lnkRtpDecChl); 
    while (pstDecChl)
    {
        i = ulCurTicks - pstDecChl->m_ulCurTicks; 
        pstDecChl->m_ulCurTicks = ulCurTicks; 

        if ( ulCurTicks - pstDecChl->m_ulLastDecJbuffTicks >= 200)
        {
            pstDecChl->m_lJbuffSize -= JBUFF_DEC_SIZE; 
            if ( pstDecChl->m_lJbuffSize < JBUFF_INIT_SIZE)
                pstDecChl->m_lJbuffSize = JBUFF_INIT_SIZE; 

            pstDecChl->m_ulLastDecJbuffTicks = ulCurTicks; 
        }
        
        if ( pstDecChl->m_lRtpMode == RTP_STRM_RTP)
        {
            pstDecChl->m_stRtp.m_stVideoJbuffer.m_lCurTimestamp += i; 
            pstDecChl->m_stRtp.m_stAudioJbuffer.m_lCurTimestamp += i; 
    
            RTP_DecodeProcess(pstDecChl, &pstDecChl->m_stRtp.m_stVideoJbuffer); 
            RTP_DecodeProcess(pstDecChl, &pstDecChl->m_stRtp.m_stAudioJbuffer); 
        }
        else 
        {
            ASSERT( pstDecChl->m_lRtpMode == RTP_STRM_HPMP); 

            pstDecChl->m_stHpmp.m_stStreamJbuffer.m_lCurTimestamp += i; 
            RTP_DecodeProcess(pstDecChl, &pstDecChl->m_stHpmp.m_stStreamJbuffer); 
        }
        
        pstDecChl = LINK_NEXT(pstDecChl, link0); 
    }
    UNLOCK_DEC(); 
}

#if 0
#if defined(WIN32) || defined(WINCE)
STATIC DWORD WINAPI RTP_Thread(LPVOID lParam)
{
    while (1)
    {
        SLEEP(1);         
        RTP_Schd();         
    }
    return 0; 
}
#endif 
#endif 

void    RTP_Start()
{
    if (m_stRtpCB.m_ulState == RTP_STATE_INUSE)
    {
        ++m_stRtpCB.m_ref_counts;
        return; 
    }

    MEMSET(&m_stRtpCB, 0, sizeof(ST_RTP_CB)); 
    
    LINK_INIT(m_stRtpCB.m_lnkRtpEncChl); 
    LINK_INIT(m_stRtpCB.m_lnkRtpDecChl); 

    INIT_LOCK();
    SRAND(CUR_TICKS()); 

#if 0
#if defined(WIN32) || defined(WINCE)
    m_stRtpCB.m_hThread = CreateThread(NULL, 128000, RTP_Thread, NULL, 0, NULL); 
    if ( m_stRtpCB.m_hThread == 0)
        return; 
    SetThreadPriority(m_stRtpCB.m_hThread, THREAD_PRIORITY_TIME_CRITICAL); 
#endif 
#endif 

    m_stRtpCB.m_ulState = RTP_STATE_INUSE;     
    ++m_stRtpCB.m_ref_counts;
}

void    RTP_Stop()
{
    ST_RTP_ENC_CHL * pstEncChl; 
    ST_RTP_DEC_CHL * pstDecChl; 

    if (m_stRtpCB.m_ulState != RTP_STATE_INUSE)
        return; 

    LOCK_ENC();
    --m_stRtpCB.m_ref_counts;
    if(m_stRtpCB.m_ref_counts)
    {/* still have user */
        return;
    }
    pstEncChl = LINK_HEAD(m_stRtpCB.m_lnkRtpEncChl); 
    while ( pstEncChl)
    {
        RTP_DeleteEncodeChl(pstEncChl); 
        pstEncChl = LINK_HEAD(m_stRtpCB.m_lnkRtpEncChl); 
    }
    UNLOCK_ENC(); 

    LOCK_DEC(); 
    pstDecChl = LINK_HEAD(m_stRtpCB.m_lnkRtpDecChl); 
    while ( pstDecChl)
    {
        RTP_DeleteDecodeChl(pstDecChl); 
        pstDecChl = LINK_HEAD(m_stRtpCB.m_lnkRtpDecChl); 
    }
    UNLOCK_DEC(); 

#if 0
#if defined(WIN32) || defined(WINCE)
    TerminateThread(m_stRtpCB.m_hThread, 0); 
#endif 
#endif 
    m_stRtpCB.m_ulState = 0; 
}

