/*!
\file       rtp__os.h
\brief      rtmp module

 ----history----
\author     chengzhiyong
\date       2007-03-16
\version    0.01
\desc       create

$Author: chengzhiyong $
$Id: http.h,v 1.14 2008-09-02 08:27:27 chengzhiyong Exp $
*/
#ifndef _RTP_OS_H
#define _RTP_OS_H
#pragma pack(1)

#if !defined(WIN32) && !defined(WINCE)
typedef  signed   char  CHAR; 
typedef  unsigned char  UCHAR;
typedef  signed   short SHORT; 
typedef  unsigned short USHORT;
typedef  signed   long  LONG;
typedef  unsigned long  ULONG;
typedef  void           VOID;
typedef  signed   long  BOOL;

#include <stdio.h>
#include <stdlib.h>

#if !defined(__rtthread__)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#endif 

#include "mcore/mcore.h"
/* #include "mcore/lock.h"
#include "mcore/thread.h"
#include "mcore/net_ex.h"
*/

#include "openssl/melib_openssl.h"
#include "openssl/ssl.h"

#pragma pack(1)

#define  TRUE               1
#define  FALSE              0

#define  CUR_TICKS()        mtime_tick()

#define  LOCK_ENTITY        lock_simple

#if 0
#define     INIT_LOCK()     {   \
    lock_simple_init(&m_stRtpCB.m_lockEnc);    \
    lock_simple_init(&m_stRtpCB.m_lockDec);    \
    }

#define  LOCK_ENC()         lock_simple_wait(&m_stRtpCB.m_lockEnc)
#define  LOCK_DEC()         lock_simple_wait(&m_stRtpCB.m_lockDec)
#define  UNLOCK_ENC()       lock_simple_release(&m_stRtpCB.m_lockEnc)
#define  UNLOCK_DEC()       lock_simple_release(&m_stRtpCB.m_lockDec)
#else 
#define  INIT_LOCK()
#define  LOCK_ENC()
#define  LOCK_DEC()
#define  UNLOCK_ENC()
#define  UNLOCK_DEC() 
#endif 

#define  MEMSET             memset
#define  MEMCPY             memcpy
#define  MALLOC             malloc
#define  FREE               free
#define  ABS                abs
#define  RAND               rand
#define  SRAND              srand

#define  INRNG1(val, min, max)       ((val) >= (min) && (val) <= (max))

#define  SLEEP(n)           thread_sleep(n)

#define  CONST              const
#define  REGISTER           register
#define  STATIC             static

#define  MAX_LONG           0x7FFFFFFF
#define  MAX_SHORT          0x7FFF
#define  MAX_CHAR           0x7F
#define  MAX_ULONG          0xFFFFFFFF
#define  MAX_USHORT         0xFFFF
#define  MAX_UCHAR          0xFF

#undef ASSERT
#define ASSERT(p)

#if 1 /* LITTLE_ENDIAN */
#define NTOA_L(val)     ((((val)<<24)&0xFF000000)|(((val)<<8)&0xFF0000)|(((val)>>8)&0xFF00)|(((val)>>24)&0xFF))
#define NTOA_W(val)     (USHORT)((((val)<<8)&0xFF00)|(((val)>>8)&0xFF))
#define NVAL_L(pucVal)  (((pucVal)[0]<<24)|((pucVal)[1]<<16)|((pucVal)[2]<<8)|((pucVal)[3]<<0))
#define NVAL_W(pucVal)  (((pucVal)[0]<<8)|((pucVal)[1]<<0))
#else 
#define NTOA_L(val)     (val)
#define NTOA_W(val)     (USHORT)(val)
#define NVAL_L(pucVal)  (*(ULONG * )(pucVal))
#define NVAL_W(pucVal)  (*(USHORT *)(pucVal))
#endif 

#define MAX(a, b, maxVal)   {   \
    REGISTER LONG _lTmp1, _lTmp2;   \
    _lTmp1 = (b)-(a);   \
    _lTmp2 = _lTmp1>>31;    \
    _lTmp1 &= _lTmp2;   \
    maxVal = (b) - _lTmp1;    }

#define MIN(a, b, minVal)   {   \
    REGISTER LONG _lTmp1, _lTmp2;   \
    _lTmp1 = (b)-(a);   \
    _lTmp2 = _lTmp1>>31;    \
    _lTmp1 &= _lTmp2;   \
    minVal = (a) + _lTmp1;  }

#define DEF_LINK_NODE(stName, linkName) \
    struct {\
        struct stName * pstPrev; \
        struct stName * pstNext; \
            } linkName 
#define LINK_ENTITY(stName) \
    struct { \
        struct stName * pstHead; \
        struct stName * pstTail; \
        LONG lNodeNum; }

#define LINK_ENTITY_SIZE    12

#define LINK_INIT(stEntity) {\
        (stEntity).pstHead = NULL; \
        (stEntity).pstTail = NULL; \
        (stEntity).lNodeNum = 0; }

#define LINK_HEAD(stEntity)     ((stEntity).pstHead)
#define LINK_TAIL(stEntity)     ((stEntity).pstTail)
#define LINK_NODE_NUM(stEntity) ((stEntity).lNodeNum)

#define LINK_NEXT(pstNode, linkName)      ((pstNode)->linkName.pstNext)
#define LINK_PREV(pstNode, linkName)      ((pstNode)->linkName.pstPrev)

#define LINK_ADD_HEAD(stEntity, pstNode, linkName) {\
        (pstNode)->linkName.pstPrev = NULL; \
        (pstNode)->linkName.pstNext = (stEntity).pstHead; \
        (stEntity).pstHead = (pstNode); \
         if ((pstNode)->linkName.pstNext) {\
             (pstNode)->linkName.pstNext->linkName.pstPrev = (pstNode); }\
         else {\
             (stEntity).pstTail = (pstNode); }\
         (stEntity).lNodeNum ++; }
#define LINK_ADD_TAIL(stEntity, pstNode, linkName) {\
        (pstNode)->linkName.pstNext = NULL; \
        (pstNode)->linkName.pstPrev = (stEntity).pstTail; \
        (stEntity).pstTail = (pstNode); \
        if ((pstNode)->linkName.pstPrev) { \
            (pstNode)->linkName.pstPrev->linkName.pstNext = (pstNode); }\
        else { \
            (stEntity).pstHead = (pstNode); }\
        (stEntity).lNodeNum ++; }
#define LINK_REMOVE_HEAD(stEntity, linkName) {\
        if ((stEntity).pstHead ) {\
            (stEntity).pstHead = (stEntity).pstHead->linkName.pstNext; \
            if ((stEntity).pstHead) {\
                (stEntity).pstHead->linkName.pstPrev = NULL; }\
            else { \
                (stEntity).pstTail = NULL; }\
            ASSERT((stEntity).lNodeNum > 0); \
        (stEntity).lNodeNum --; }}
#define LINK_REMOVE_TAIL(stEntity, linkName) {\
        if ((stEntity).pstTail ) {\
            (stEntity).pstTail = (stEntity).pstTail->linkName.pstPrev; \
            if ((stEntity).pstTail) {\
                (stEntity).pstTail->linkName.pstNext = NULL; }\
            else { \
                (stEntity).pstHead = NULL; }\
            ASSERT((stEntity).lNodeNum > 0); \
        (stEntity).lNodeNum --; }}
#define LINK_REMOVE_NODE(stEntity, pstNode, linkName) {\
        if ((pstNode)->linkName.pstPrev) {\
            (pstNode)->linkName.pstPrev->linkName.pstNext = (pstNode)->linkName.pstNext; }\
        else {\
            ASSERT((stEntity).pstHead == (pstNode)); \
            (stEntity).pstHead = (pstNode)->linkName.pstNext;}\
        if ((pstNode)->linkName.pstNext) {\
            (pstNode)->linkName.pstNext->linkName.pstPrev = (pstNode)->linkName.pstPrev; }\
        else {\
            ASSERT((stEntity).pstTail == (pstNode)); \
            (stEntity).pstTail = (pstNode)->linkName.pstPrev;}\
        ASSERT((stEntity).lNodeNum > 0); \
        (stEntity).lNodeNum --; } 
#define LINK_INSERT_PREV(stEntity, pstNode1, pstNode2, linkName) {\
        if (!pstNode1) {    \
            LINK_ADD_TAIL(stEntity, pstNode2, linkName);    }   \
        else if ((pstNode1)->linkName.pstPrev )  {  \
            (pstNode2)->linkName.pstPrev = (pstNode1)->linkName.pstPrev; \
            (pstNode1)->linkName.pstPrev->linkName.pstNext = (pstNode2); \
            (pstNode2)->linkName.pstNext = (pstNode1);  \
            (pstNode1)->linkName.pstPrev = (pstNode2);  \
            (stEntity).lNodeNum ++; }   \
        else {\
            ASSERT((stEntity).pstHead == (pstNode1));   \
            (stEntity).pstHead = (pstNode2); \
            (pstNode2)->linkName.pstNext = (pstNode1);  \
            (pstNode2)->linkName.pstPrev = NULL; \
            (pstNode1)->linkName.pstPrev = (pstNode2);  \
            (stEntity).lNodeNum ++; }} 
#define LINK_INSERT_NEXT(stEntity, pstNode1, pstNode2, linkName) {\
        if (!pstNode1) {    \
            LINK_ADD_HEAD(stEntity, pstNode2, linkName);    }   \
        else if ((pstNode1)->linkName.pstNext )  {  \
            (pstNode2)->linkName.pstNext = (pstNode1)->linkName.pstNext; \
            (pstNode1)->linkName.pstNext->linkName.pstPrev = (pstNode2); \
            (pstNode2)->linkName.pstPrev = (pstNode1);  \
            (pstNode1)->linkName.pstNext = (pstNode2);  \
            (stEntity).lNodeNum ++; }   \
        else {\
            ASSERT((stEntity).pstTail == (pstNode1));   \
            (stEntity).pstTail = (pstNode2);    \
            (pstNode2)->linkName.pstPrev = (pstNode1);  \
            (pstNode2)->linkName.pstNext = NULL;    \
            (pstNode1)->linkName.pstNext = (pstNode2);  \
            (stEntity).lNodeNum ++; }}
#define LINK_COPY_LINK(stEntity1, stEntity2, linkName)  {\
        if ((stEntity1).pstTail) {  \
            ASSERT((stEntity1).lNodeNum > 0);   \
            if ((stEntity2).pstHead) {  \
                ASSERT((stEntity2).lNodeNum > 0);   \
                (stEntity1).pstTail->linkName.pstNext = (stEntity2).pstHead;    \
                (stEntity2).pstHead->linkName.pstPrev = (stEntity1).pstTail;    \
                (stEntity1).pstTail = (stEntity2).pstTail;  \
                (stEntity1).lNodeNum += (stEntity2).lNodeNum;   \
                (stEntity2).pstHead = (stEntity2).pstTail = NULL;   \
                (stEntity2).lNodeNum = 0; }}    \
        else {  \
            ASSERT((stEntity1).lNodeNum == 0);  \
            ASSERT((stEntity1).pstHead == NULL);    \
            (stEntity1).pstHead = (stEntity2).pstHead;  \
            (stEntity1).pstTail = (stEntity2).pstTail;  \
            (stEntity1).lNodeNum = (stEntity2).lNodeNum;\
            (stEntity2).pstHead = (stEntity2).pstTail = NULL;   \
            (stEntity2).lNodeNum = 0; }}

#pragma pack()
#endif             

