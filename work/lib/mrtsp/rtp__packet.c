/*!
\file       rtp_pack.c
\brief      use to do define basic function for get rtp repackets or unpackets

 ----history----
\author     huangyifan
\version    0.01
\desc       create
 ----history----
 \author    huangyifan
 \version   0.02
 \desc      modify
*/

#define mmodule_name "lib.mlib.mrtsp.rtp__packet"
#include "mcore/mcore.h"
#include "rtp__packet.h"
#include "mmedia_format/mmedia_format.h"
#include "math.h"

#define fix_length              4

/*!
func    rtp_create_packet_header
\brief  create a rtp packet header
\param  data[in]                payload of the rtp
\param  payload_type[in]        type of payload
\param  time_stamp[in]          time stamp of packet
\param  ssrc[in]                synchronization source (SSRC) identifier of packet
\param  marker[in]              application marker symbol
\param  seq_num[in]             the seq number of this packet
\return result of execute
        #0                      success
        #other                  failed
*/
long  rtp_create_packet_header( unsigned char     *data, 
                                unsigned char     payload_type, 
                                unsigned long     time_stamp, 
                                unsigned long     ssrc, 
                                unsigned long     marker,
                                unsigned long     seq_num)
{
    /* fill rtp header, the padding target, extend target..etc is ignored */
    data[0] = 0x80;   /* version */
    data[1] = (unsigned char)((marker? 0x80 : 0x00) | payload_type);   /* marker & payload type */

    /* sequence number, take up 2 bytes */
    data[2] = (unsigned char)((seq_num >> 8) & 0xff); 
    data[3] = (unsigned char)(seq_num & 0xff);

    /* time stamp, take up 4 bytes */
    data[4] = (unsigned char)((time_stamp >> 24) & 0xff); 
    data[5] = (unsigned char)((time_stamp >> 16) & 0xff);
    data[6] = (unsigned char)((time_stamp >> 8)  & 0xff);
    data[7] = (unsigned char)(time_stamp & 0xff);

    /* ssrc, take up 4 bytes */
    data[8] = (unsigned char)((ssrc >> 24) & 0xff);
    data[9] = (unsigned char)((ssrc >> 16) & 0xff);
    data[10] = (unsigned char)((ssrc >> 8)  & 0xff);
    data[11] = (unsigned char)(ssrc & 0xff);                                
    return 0;
}

long  rtcp_create_packet_header( unsigned char	  *data, 
								unsigned char	  data_type, 
								unsigned long	  time_stamp,
								unsigned long 	  sample_time,
								int 			  use_sample_time,
								unsigned long	  ssrc, 
								unsigned long	  seq_count,
								unsigned long	  sample_total){
	unsigned long ntpMSW, ntpLSW;
    data[0] = 0x80;   /* version */
	data[1] = data_type;   /* RTCP */
	switch(data[1]){
		case 200:		//SR
            /* The length of this RTCP packet in 32-bit words minus one */
            data[2] = 0x00;
            data[3] = 0x06;

            /* ssrc */
            data[4] = (unsigned char)((ssrc >> 24) & 0xff);
            data[5] = (unsigned char)((ssrc >> 16) & 0xff);
            data[6] = (unsigned char)((ssrc >> 8)  & 0xff);
            data[7] = (unsigned char)(ssrc & 0xff);

			/* ntp */
			if(use_sample_time){
				ntpMSW = sample_time / 1000;
				ntpLSW = (sample_time % 1000) * pow(2, 32) / 1000;
			}
            else{
				NTPtime64(&ntpMSW, &ntpLSW);
            }
			/* MSW */
            data[8] = (unsigned char)((ntpMSW >> 24) & 0xff);
            data[9] = (unsigned char)((ntpMSW >> 16) & 0xff);
            data[10] = (unsigned char)((ntpMSW >> 8)  & 0xff);
            data[11] = (unsigned char)(ntpMSW & 0xff);
			/* LSW */
			data[12] = (unsigned char)((ntpLSW >> 24) & 0xff);
            data[13] = (unsigned char)((ntpLSW >> 16) & 0xff);
            data[14] = (unsigned char)((ntpLSW >> 8)  & 0xff);
            data[15] = (unsigned char)(ntpLSW & 0xff);

			/* timestamp */
			data[16] = (unsigned char)((time_stamp >> 24) & 0xff);
            data[17] = (unsigned char)((time_stamp >> 16) & 0xff);
            data[18] = (unsigned char)((time_stamp >> 8)  & 0xff);
            data[19] = (unsigned char)(time_stamp & 0xff);

			/* SPC */
			data[20] = (unsigned char)((seq_count >> 24) & 0xff);
            data[21] = (unsigned char)((seq_count >> 16) & 0xff);
            data[22] = (unsigned char)((seq_count >> 8)  & 0xff);
            data[23] = (unsigned char)(seq_count & 0xff);

			/* SOC */
			data[24] = (unsigned char)((sample_total >> 24) & 0xff);
            data[25] = (unsigned char)((sample_total >> 16) & 0xff);
            data[26] = (unsigned char)((sample_total >> 8)  & 0xff);
            data[27] = (unsigned char)(sample_total & 0xff);						
			break;
		default:
			return 1;
	}	
}

/*!
func    rtp_encode_create_packet
\brief  create a rtp video packet struct
\param  seq_num[in]             packet's sequence number
\param  time_stamp[in]          time stamp for rtp packet
\param  payload_type[in]        payload type of media
\param  ssrc[in]                ssrc of packet
\param  mtu[in]                 maximum transport unit size 
\param  sample[in]              data of nal
\param  sample_len[in]          length of nal
\return rtp video packet struct 
        #NULL                   create packet struct failed
        #other                  success.
*/
struct rtp_packet* rtp_encode_create_packet(unsigned short   seq_num,
                                           unsigned long     time_stamp,
                                           unsigned char     payload_type,
                                           long              ssrc,
                                           unsigned long     mtu,
                                           unsigned char     *sample,
                                           unsigned long     sample_len)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtp_encode_create_packet(seq_num[%ld], time_stamp[%ld], payload_type[%ld], ssrc[%ld], mtu[%ld], sample[%p], sample_len[%ld])"
#define func_format()   (long)seq_num, time_stamp, (long)payload_type, ssrc, mtu, sample, sample_len

    struct rtp_data     *tmp_data = NULL;
    unsigned char       nal_type, nal_hdr, fu_SE_flag, *nal, *nal_pos;
    unsigned long       prev_zero_counts = 0, total_remain_len = 0, nal_remain_len = 0, copy_size = 0, payload_size, fu_a_pack_mtu = mtu - RTP_HEADER_SIZE  - 2;
    struct rtp_data     *data = NULL;
    struct rtp_packet   *packet = NULL;

    print_log0(detail, ".");

    if((NULL == sample) || (sample_len < 4) || (mtu < (RTP_HEADER_SIZE + 2)))
    {
        print_log0(err, "failed with invalid param.");
        return NULL;
    }
    packet = (struct rtp_packet*)calloc(1, sizeof(struct rtp_packet));
    if(NULL == packet)
    {
        print_log0(err, "failed when malloc rtp_packet.");
        return NULL;
    }

    for(total_remain_len = sample_len, nal = sample; total_remain_len > 4; total_remain_len -= payload_size + 4, nal += payload_size + 4)
    {
        nal_hdr = nal[fix_length];
        nal_type = nal_hdr & 0x1f;
        payload_size = mbytes_read_int_big_endian_4b(nal);//(nal[0] << 24) | (nal[1] << 16) | (nal[2] << 8) | nal[3];
        if((payload_size + 4) > total_remain_len)
        {
            print_log0(err, "meet invalid nal.");
            return packet;
        }

        if((0x07 == nal_type) || (0x08 == nal_type))
        {/* skip SPS PPS */
            print_log0(detail, "skip sps|pps.");
//            continue;
        }

        if(mtu >= (payload_size + RTP_HEADER_SIZE))
        {/* there is not need to frag sample    */
            tmp_data = (struct rtp_data*)calloc(1, sizeof(struct rtp_data) +  RTP_HEADER_SIZE + payload_size);
            if(NULL == tmp_data)
            {
                while((tmp_data = packet->subs.list))
                {
                    mlist_del(packet->subs, tmp_data, out_list);
                    free(tmp_data);
                }
                free(packet);
                print_log0(err, "failed when malloc() rtp_data.");
                return NULL;
            }
            tmp_data->data = ((unsigned char*)tmp_data) + sizeof(struct rtp_data);
#if 0
            tmp_data->len = RTP_HEADER_SIZE + mfmt_video_h264_nal_to_raw(nal + fix_length, payload_size, tmp_data->data + RTP_HEADER_SIZE, payload_size, &prev_zero_counts);
#else
            memcpy(tmp_data->data + RTP_HEADER_SIZE , nal + fix_length, payload_size);
            tmp_data->len = RTP_HEADER_SIZE + payload_size;
#endif
            rtp_create_packet_header(tmp_data->data, payload_type, time_stamp, ssrc, 1, seq_num);
            mlist_add(packet->subs, tmp_data, out_list);            
            seq_num += 1;
        }
        else
        {
            for(fu_SE_flag = 0x80, nal_pos = nal + 5, nal_remain_len = payload_size - 1;
                nal_remain_len;
                nal_pos += copy_size, nal_remain_len -= copy_size, fu_SE_flag = 0)
            {/* still have remained len */
                copy_size = (nal_remain_len > fu_a_pack_mtu)?fu_a_pack_mtu:nal_remain_len;

                tmp_data = (struct rtp_data*)malloc(sizeof(struct rtp_data) + RTP_HEADER_SIZE + 2 + copy_size);
                if(NULL == tmp_data)
                {
                    while((tmp_data = packet->subs.list))
                    {
                        mlist_del(packet->subs, tmp_data, out_list);
                        free(tmp_data);
                    }
                    free(packet);
                    print_log0(err, "failed when malloc() rtp_data.");
                    return NULL;
                }
                tmp_data->data = ((unsigned char*)tmp_data) + sizeof(struct rtp_data);

#if 0
                tmp_data->len = RTP_HEADER_SIZE + 2 + mfmt_video_h264_nal_to_raw(nal_pos, copy_size, tmp_data->data + RTP_HEADER_SIZE + 2, copy_size, &prev_zero_counts);
#else
                memcpy(tmp_data->data + RTP_HEADER_SIZE + 2, nal_pos, copy_size);
                tmp_data->len = RTP_HEADER_SIZE + 2 + copy_size;
#endif
                if(nal_remain_len == copy_size){ fu_SE_flag = 0x40; };
                rtp_create_packet_header(tmp_data->data, payload_type, time_stamp, ssrc, fu_SE_flag & 0x40, seq_num);
                tmp_data->data[RTP_HEADER_SIZE] = (nal_hdr & 0xe0) | 0x1c;
                tmp_data->data[RTP_HEADER_SIZE + 1] = fu_SE_flag | nal_type;

                mlist_add(packet->subs, tmp_data, out_list);  
                seq_num += 1;
            }
        }
    }

    print_log2(detail, "ret[%p{%ld}]", packet, packet?packet->subs.counts:0);
    return packet;
}

/*!
func    rtp_destroy_packet
\brief  destroy a rtp packet struct
\param  packet[in]            the rtp packet struct to be destroyed
\return execute status
        #0                          success in destroy
        #other                      failed
*/
long rtp_destroy_packet( struct rtp_packet* packet )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtp_destroy_packet(packet[%p{%ld}])"
#define func_format()   packet, packet?packet->subs.counts:0

    struct rtp_data *data;

    print_log0(detail, ".");

    if(NULL == packet)
    {
        print_log0(err, "failed with invalid param.");
        return 0;
    }
    while((data = packet->subs.list))
    {
        mlist_del(packet->subs, data, out_list);
        free(data);
    }
    if (packet->data) free(packet->data);
    free(packet);
    return 0;
}


/*!
func    rtp_decode_create_packet
\brief  create a rtp packet struct
\return a rtp packet struct 
        #NULL                   create failed because memory not enough
        #other                  success.
*/
struct rtp_packet* rtp_decode_create_packet(void)
{
    struct  rtp_packet *packet = (struct  rtp_packet*)calloc(1, sizeof(struct rtp_packet));
    if(NULL == packet)
    {
        print_err("err: rtp_decode_create_packet() failed because memory not enough. %s:%d\n", __FILE__, __LINE__);
        return NULL;
    }
    return packet;
}

/*!
func    rtp_decode_delete
\brief  remove packet data from  rtp packet struct
\param  packet[in]              rtp packet struct
\return result of remove
        #0                      success.
        #other                  failed.
*/
long rtp_decode_delete(struct rtp_packet *packet)
{
    struct rtp_data *data;
    if(NULL == packet)
    {
        print_err("err: rtp_decode_delete() delete nothing because not elements to be deleted. %s:%d\n", __FILE__, __LINE__);
        return  -2;
    }

    while((data = packet->subs.list))
    {/* delete all the elements in single rtp_packet */
        mlist_del(packet->subs, data, out_list);
        free(data);
    }    
    /* delete the decode packet */
    if( packet->data)
        free(packet->data);
    packet->data = NULL;
    packet->len = 0;
    
    return 0;    
}

/*!
func    rtp_decode_append
\brief  repack the packet to a nal which may unpack in the transportation
\param  rtp_cb[in]            rtp packet struct
\param  data[in]              packet data th be repack
\param  len[in]               length of packet data
\param  errind[in]            indicate this packet was error or not        
\return result of execution
        #1                    no nal generate
        #0                    can get a packet from the param rtp_pt
        #other                failed
*/
long rtp_decode_append( struct rtp_packet *packet, unsigned char *data, unsigned long len, long errind )
{
#undef  func_format_s   
#undef  func_format
#define func_format_s   "rtp_decode_append(packet[%p{subs[%ld], seq[%ld]}], data[%p{seq[%ld]}], len[%ld], errind[%ld])"
#define func_format()   packet, packet?packet->subs.counts:0, (long)(packet?packet->last_seq_num:0), \
                        data, ((unsigned long)((data && (len >= 4))?((data[2] << 8) | data[3]):0)), len, errind

    unsigned char   *buf  = NULL;
    struct rtp_data *tmp_data  = NULL;
    unsigned long   buf_size;
    unsigned short  cur_seq_num  = (data && (len >= 4))?(((unsigned short)data[2] << 8) | ((unsigned short)data[3])):0;
    
    if((NULL == packet) || (NULL == data) || (RTP_HEADER_SIZE > len))
    {
        print_log0(err, "failed with invalid param.");
        return -1;
    }

    /* check type */
    if((data[RTP_HEADER_SIZE] & 0x1f) < 28)
    {/*  single nal in one rtp packet */
        unsigned long time_stamp = 0, ssrc = 0;

        if(errind)
        {/* abandon error packet */
            print_log0(err, "packet error.");
            return 1;
        }

        /* one packet holds single nal */
        buf_size = len - RTP_HEADER_SIZE + fix_length + mfmt_video_h264_sps_max_len + mfmt_video_h264_pps_max_len + 8; //chenyong modify 2018-11-29
        buf = (unsigned char*)malloc(buf_size);
        if(NULL == buf)
        {
            print_log1(err, "failed because malloc(%ld).", buf_size);
            return -3;
        }
            
        buf[0] = (unsigned char)((len - RTP_HEADER_SIZE) >> 24);
        buf[1] = (unsigned char)(((len - RTP_HEADER_SIZE) >> 16) & 0xff);
        buf[2] = (unsigned char)(((len - RTP_HEADER_SIZE) >> 8) & 0xff);
        buf[3] = (unsigned char)((len - RTP_HEADER_SIZE) & 0xff);
        memcpy(buf + fix_length, data + RTP_HEADER_SIZE, len - RTP_HEADER_SIZE);
        packet->time_stamp = mbytes_read_int_big_endian_4b(&data[4]);//(((unsigned long)data[4]) << 24) | (((unsigned long)data[5]) << 16) | (((unsigned long)data[6]) << 8) | ((unsigned long)data[7]);
        packet->ssrc = mbytes_read_int_big_endian_4b(&data[8]);//(((unsigned long)data[8]) << 24) | (((unsigned long)data[9]) << 16) | (((unsigned long)data[10]) << 8) | ((unsigned long)data[11]);
        packet->data = buf;
        packet->len = len - RTP_HEADER_SIZE + fix_length;
        packet->last_seq_num = cur_seq_num;
        return 0;
    }
    else
    {
        if((data[RTP_HEADER_SIZE + 1] & 0x80) || errind || ((unsigned short)(packet->last_seq_num + (unsigned short)1) != cur_seq_num))
        {/* start new packet */
            while((tmp_data = packet->subs.list))
            {
                mlist_del(packet->subs, tmp_data, out_list);
                free(tmp_data);
            }
            
            if(errind || ((unsigned short)(packet->last_seq_num + (unsigned short)1) != cur_seq_num))
            {
                print_log1(warn, "meet error, ignore [%ld] packs.", packet->subs.counts);
                return 1;
            }
            
        }
        
        if(data[RTP_HEADER_SIZE + 1] & 0x40)
        {/* the end of nal , end bit is true */
            if(0 == packet->subs.counts)
            {/* a pack end arrived, but have not data, err */
                print_log0(warn, "ending pack arrived, but have any ready data now.");
                return 1;
            }
            else
            {
                unsigned long index = fix_length, nal_size = 0;
                tmp_data = packet->subs.list;
            
                do 
                {
                    nal_size += tmp_data->len;
                } while ( (tmp_data = tmp_data->out_list.next) != packet->subs.list );

                nal_size += len - RTP_HEADER_SIZE - 2; /* last pack fragment */
                buf_size = nal_size + fix_length + mfmt_video_h264_sps_max_len + mfmt_video_h264_pps_max_len + 8;/* for nal len */ //chenyong modify 2018-11-29
                buf = (unsigned char*)malloc(buf_size);
                if(NULL == buf)
                {
                    print_log1(err, "failed because malloc(%ld).", buf_size);
                    return -3;
                }
            
                while((tmp_data = packet->subs.list))
                {/* copy the prev save data to tmp buffer */
                    memcpy(buf + index, tmp_data->data, tmp_data->len);
                    index += tmp_data->len;
                    /* delete the copied data  packet in video_packet */
                    mlist_del(packet->subs, tmp_data, out_list);
                    free(tmp_data);
                }
            
                /* copy the last chunk data to tmp buffer  */
                buf[0] = (unsigned char)(nal_size >> 24);
                buf[1] = (unsigned char)((nal_size >> 16) & 0xff);
                buf[2] = (unsigned char)((nal_size >> 8) & 0xff);
                buf[3] = (unsigned char)(nal_size & 0xff);
                memcpy(buf + index, data + RTP_HEADER_SIZE + 2, len - RTP_HEADER_SIZE - 2);
            
                packet->data = buf;
                packet->len = nal_size + fix_length;
                packet->time_stamp = mbytes_read_int_big_endian_4b(&data[4]);// (((unsigned long)data[4]) << 24) | (((unsigned long)data[5]) << 16) | (((unsigned long)data[6]) << 8) | ((unsigned long)data[7]);
                packet->ssrc = mbytes_read_int_big_endian_4b(&data[8]);//(((unsigned long)data[8]) << 24) | (((unsigned long)data[9]) << 16) | (((unsigned long)data[10]) << 8) | ((unsigned long)data[11]); 
                packet->last_seq_num = cur_seq_num;
                return 0;   /* get a nal , return 1 */
            }
        }
        else
        {/* the first packet of the nal, add the header(four chars) and first char of nal  */
            if(data[RTP_HEADER_SIZE + 1] & 0x80)
            {/* only when the packet is the start packet and first packet of nal it is useful or it will be an error packet */
                buf_size = len - RTP_HEADER_SIZE - 2 + 1 + sizeof(struct rtp_data);
                tmp_data = (struct rtp_data*)malloc(buf_size);
                if(NULL == tmp_data)
                {
                    print_log1(err, "failed because malloc(%ld).", buf_size);
                    return -2;
                }
                memset(tmp_data, 0, sizeof(struct rtp_data));
                tmp_data->data = ((unsigned char*)tmp_data) + sizeof(struct rtp_data);
                tmp_data->data[0] = (data[RTP_HEADER_SIZE + 0] & 0x60) | (data[RTP_HEADER_SIZE + 1] & 0x1f);
                memcpy(tmp_data->data + 1, data + RTP_HEADER_SIZE + 2, len - RTP_HEADER_SIZE - 2);
                tmp_data->len = len - RTP_HEADER_SIZE - 2 + 1;
                mlist_add(packet->subs, tmp_data, out_list);
                packet->last_seq_num = cur_seq_num;
            }
            else
            {
                if(packet->subs.counts)
                {/* middle packet of the nal */
                    buf_size = (len - RTP_HEADER_SIZE - 2) + sizeof(struct rtp_data);
                    tmp_data = (struct rtp_data*)malloc(buf_size);
                    if(NULL == tmp_data)
                    {
                        print_log1(err, "failed because malloc(%ld).", buf_size);
                        return -2;
                    }
                    memset(tmp_data, 0, sizeof(struct rtp_data));
                    tmp_data->data = ((unsigned char*)tmp_data) + sizeof(struct rtp_data);
                    /* add data to tmp_data  */
                    memcpy(tmp_data->data, data + RTP_HEADER_SIZE + 2, len - RTP_HEADER_SIZE - 2);
                    tmp_data->len = len - RTP_HEADER_SIZE - 2;
                    mlist_add(packet->subs, tmp_data, out_list);
                    packet->last_seq_num = cur_seq_num;
                }
                else
                {/* error data, ingore */
                    print_log0(warn, "meet middle data but missing first pack.");
                    return 1;
                }
            }
        }
    }

    return 1;
}

#if !defined(_MSC_VER) /* for linux, !!!!these linese must before include stdio */
#   if defined(__APPLE_CC__)
        struct timeval;
        int gettimeofday(struct timeval * __restrict, void * __restrict);
#   endif
#endif

/**
 * @return NTP 64-bits timestamp in host byte order.
 */
void NTPtime64 (unsigned long *sec, unsigned long *psec232)
{
#if 0
(_POSIX_TIMERS > 0)
    struct timespec ts;

    clock_gettime (CLOCK_REALTIME, &ts);
#else
    struct timeval tv;
    struct
    {
        uint32_t tv_sec;
        uint32_t tv_nsec;
    } ts;

    gettimeofday (&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif

    /* Convert nanoseconds to 32-bits fraction (232 picosecond units) */
//    uint64_t t = (uint64_t)(ts.tv_nsec) << 32;
//    t /= 1000000000;
	*psec232 = (unsigned long)(((float)ts.tv_nsec) * pow(2, 32) / 1000000000);

    /* There is 70 years (incl. 17 leap ones) offset to the Unix Epoch.
     * No leap seconds during that period since they were not invented yet.
     */
//    assert (t < 0x100000000);
    *sec = ((unsigned long)(70 * 365 + 17) * 24 * 60 * 60) + ts.tv_sec;
//    return t;
}

