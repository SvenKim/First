/*!
\file       rtp_get_packet.h
\brief      use to do declare basic function and data struct for get rtp packet

  ----history----
 \author     huangyifan
 \date       2009-12-07
 \version    0.01
 \desc       create
 ----history----
 \author     huangyifan
 \date       2009-12-16
 \version    0.02
 \desc       modify
  
*/

#if defined(__cplusplus)
extern "C" {
#endif/* defined(__cplusplus) */

#ifndef __RTP_PACKET_H__
#define __RTP_PACKET_H__

#define RTP_HEADER_SIZE   12

typedef struct rtp_data
{
    struct
    {
        struct rtp_data     *prev;
        struct rtp_data     *next;
    }out_list;
    unsigned long           len;
    unsigned char           *data;
}_rtp_data;

typedef struct rtp_packet
{
    struct  
    {
        struct rtp_packet   *prev;
        struct rtp_packet   *next;
    }out_list;
    unsigned long           time_stamp;     /*!< time stamp                 */
    unsigned long           ssrc;
    unsigned short          last_seq_num;

    unsigned long           len;
    unsigned char           *data;

    struct  
    {
        unsigned long       counts;
        struct rtp_data     *list;
    }subs;
    
}_rtp_packet;

/*	interface functions declare for packet and re-packet */
long  rtp_create_packet_header( unsigned char     *data, 
                                unsigned char     payload_type, 
                                unsigned long     time_stamp, 
                                unsigned long     ssrc, 
                                unsigned long     marker,
                                unsigned long     seq_num);

long  rtcp_create_packet_header( unsigned char	  *data, 
								unsigned char	  data_type, 
								unsigned long	  time_stamp,
								unsigned long 	  sample_time,
								int 			  use_sample_time,
								unsigned long	  ssrc, 
								unsigned long	  seq_count,
								unsigned long	  sample_total);


long  rtp_encode_packet_header( unsigned char *data,
                                unsigned char payload_type,
                                unsigned long time_stamp,
                                long ssrc, 
                                unsigned marker, 
                                unsigned short seq_num );

struct rtp_packet* rtp_encode_create_packet( unsigned short   seq_num,
                                            unsigned long     time_stamp,
                                            unsigned char     payload_type,
                                            long              ssrc,
                                            unsigned long     mtu,
                                            unsigned char     *sample,
                                            unsigned long     sample_len);
long rtp_destroy_packet( struct rtp_packet *rtp_pt );
                            
struct rtp_packet* rtp_decode_create_packet( void );
long rtp_decode_append( struct rtp_packet *rtp_pt, unsigned char *data, unsigned long len, long errind );
long rtp_decode_delete( struct rtp_packet *rtp_pt);
void NTPtime64 (unsigned long *sec, unsigned long *psec232);


#endif

#if defined(__cplusplus)
}
#endif/* defined(__cplusplus) */
