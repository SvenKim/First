/*!
\file       rtsp_session.h
\brief      rtsp unit

 ----history----
\author     xionghuatao
\date       2009-11-12
\version    0.01
\desc       create

$Author: xionghuatao $
$Id: rtsp_session.h,v 1.14 2009-11-09 17:28:27 xionghuatao Exp $
*/

#if defined(__cplusplus)
extern "C" {
#endif/* defined(__cplusplus) */

#ifndef __RTSP_SESSION_H__
#define __RTSP_SESSION_H__

/*
#undef  print_level
#define print_level 3 
//*/
    
struct rtp_channel;
struct rtsp_session;
struct rtsp_module;
struct rtp_packet;

#define RTSP_BUFFERSIZE         (0x8000)
#define RTSP_RTP_BUFFERSIZE     (0x4000)
#define RTSP_RTCP_BUFFERSIZE    (0x400)
#define RTSP_SESSION_TIMEOUT    15

#define RTSP_PACKAGE_NAME      "MMSS"
#if defined(WIN64)
#   define rtsp_platform        "Win64"
#elif defined(WIN32)
#   define rtsp_platform        "Win32"
#elif defined(__linux__)
#   define rtsp_platform        "Linux"
#else
#   define rtsp_platform        "Universal"
#endif

#define RTSP_PACKAGE_VERSION   "0.1 (Build/2011.11.04; Platform/"rtsp_platform"; Release/ShenzhenMining; state/beta; )"
//#define RTSP_PACKAGE_NAME      "DSS"
//#define RTSP_PACKAGE_VERSION   "5.5.5 (Build/489.16; Platform/Win32; Release/Darwin; state/beta; )"

#define RTSP_EL                 "\r\n"
#define RTSP_VERSION            "RTSP/1.0"

/* error code define */
#define ERR_NO_ERROR            0
#define ERR_GENERIC             -1
#define ERR_ALLOC_FAILED        -2
#define ERR_NOT_FOUND           -3
#define ERR_CONN_CLOSED         -4
#define ERR_FATAL               -5
#define ERR_INPUT_PARAM         -6  

typedef enum { ertt_rtp_avp_udp, ertt_rtp_avp_tcp } rtp_transport_type;   
typedef enum { erst_rtsp, erst_rtp, erst_rtcp } rtsp_sock_type;  
typedef enum { ermt_video, ermt_audio } rtp_media_type;
    
typedef struct rtsp_pair { unsigned long rtp; unsigned long rtcp; } _rtsp_pair; 

/*! \brief rtsp data */
typedef struct rtsp_data
{
    struct
    {
        struct rtsp_data    *prev;              /*!< prev data in sock */
        struct rtsp_data    *next;              /*!< next data in sock */
        struct rtsp_sock    *owner;             /*!< the owner sock */
    }in_list;
    unsigned long           len;                /*!< data length */
    unsigned char           *data;              /*!< the data */
    struct sockaddr_in      addr;               /*!< dest address, just for udp */
} _rtsp_data;

/*! \brief rtsp sock */
typedef struct rtsp_sock
{   
    rtsp_sock_type          type;               /*!< rtsp sock type */
    void                    *owner_refer;       /*!< refer to sock's owner, if type eq erst_rtsp , this refer to rtsp session or module,
                                                     if type eq (erst_rtp or erst_rtcp), refer to rtp channel */
    long                    fd;                 /*!< socket handle */
    
    struct
    {
		long				cache_size;			/*!< rtsp send memory cache size */
        unsigned long       counts;             /*!< data counts */
        struct rtsp_data    *list;              /*!< data list */
        unsigned long       index;              /*!< data be send index in current data */
    }send;
    
    struct {                  
        char                *buffer;            /*!< recv buffer */
        unsigned long       size;               /*!< buffer size */
        unsigned long       index;              /*!< index of data be dealed with  */        
        unsigned long       length;             /*!< data length */        
    } recv;  
}_rtsp_sock;
#define rtsp_sock_format_s  "%p{%s}"
#define rtsp_sock_format(_sock) (_sock), ((_sock)?((erst_rtsp == (_sock)->type)?"rtsp":((erst_rtp == (_sock)->type)?"rtp":((erst_rtcp == (_sock)->type)?"rtcp":"unknown"))):"null")

typedef struct rtsp_support_method
{
    struct 
    {
        struct rtsp_session         *session;
        struct rtsp_support_method  *prev;
        struct rtsp_support_method  *next;
    } in_list;

    char            method[20];
    unsigned long   len;
}_rtsp_support_method;

/*! \brief rtp channel */
typedef struct rtp_channel
{         
    struct 
    {
        struct rtsp_session    *session;
        struct rtp_channel     *prev;
        struct rtp_channel     *next;       
    } in_list;     
    
    char                        *media_url;                 /*!< media url description of video or audio, e.g. rtsp://192.168.2.104/sample_100kbit.mp4/trade=3 */
    unsigned long               payload_type;               /*!< the media stream payload , for example, 96(video), 97(audio) */
    unsigned long               start_rtptime;              /*!< start rtp time */ 
    unsigned long               rtptime;                    /*!< the rtp time */
    unsigned long               start_seq;                  /*!< start seq for rtp stream */    
    unsigned long               seq;                        /*!< rtp package sequence num */
    unsigned long               ssrc;                       /*!< the ssrc, identifies the synchronization source */   
    rtp_transport_type          transport_type;             /*!< rtp transport type usd udp or tcp, value either ertt_rtp_avp_udp or ertt_rtp_avp_tcp */
    rtp_media_type              media_type;                 /*!< the media type for video stream or audio stream */
	
    struct rtp_packet           *pack;                      /*!< rtp decode to nal */
    
    union
    {
        struct 
        { 
            struct rtsp_pair    local_port;                 /*!< the udp ports local used */
            struct sockaddr_in  rtp_addr;                   /*!< the remote rtp received addr */
            struct sockaddr_in  rtcp_addr;                  /*!< the remote rtcp received addr */
            struct rtsp_sock    *rtp_sock;                  /*!< rtp refer sock */
            struct rtsp_sock    *rtcp_sock;                 /*!< rtcp refer sock */
        }udp;
        struct 
        {            
            struct rtsp_pair    interleaved_id;             /*!< rtp/rtcp channel, i.e. the rtsp interframe channels */
        }tcp;
    } transport;  
} _rtp_channel;  
#define rtp_channel_format_s    "%p"
#define rtp_channel_format(_chl) (_chl)

typedef long (*rtsp__on_req)(struct rtsp_session *session);
typedef long (*rtsp__on_rsp)(struct rtsp_session *session);

long rtsp__rtp_get_port_pair( struct rtsp_module *mod, struct rtsp_fd_desc *rtp, struct rtsp_fd_desc *rtcp);

long rtsp__on_tcp_recv( struct rtsp_session *session ); 
long rtsp__on_tcp_send( struct rtsp_session *session );
long rtsp__on_udp_recv(struct rtsp_sock *sock, struct sockaddr_in addr);
long rtsp__on_udp_send(struct rtsp_sock *sock);

long rtsp__on_rtsp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events);
long rtsp__on_rtp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events);
long rtsp__on_rtcp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events);

long rtsp__on_tcp_recv_data(struct rtsp_session *session);
long rtsp__on_rtsp_msg_data(struct rtsp_session *session);
long rtsp__on_rtp_interleaved_data(struct rtsp_session *session, unsigned char *data, unsigned long len);

struct rtp_channel *rtsp__rtp_channel_create( struct rtsp_session *session );
long rtsp__rtp_channel_destroy( struct rtp_channel *channel );

long rtsp__on_sdp(struct rtsp_session *session);

long rtsp__add_rtp_package(struct rtp_channel *channel, unsigned char *data, unsigned long len, long data_type /* data_type: erst_rtp , erst_rtcp */);
long rtsp__add_rtsp_package(struct rtsp_session *session, unsigned char *data, unsigned long len);

void rtsp__on_rtp_recv_data(struct rtp_channel *channel, unsigned char *data, unsigned long len);
void rtsp__on_rtcp_recv_data(struct rtp_channel *channel, unsigned char *data, unsigned long len);
void rtsp__on_rtp_video_out(struct rtsp_session *session, unsigned char *data, long len, long errind );
void rtsp__on_rtp_audio_out(struct rtsp_session *session, unsigned char *data, long len);

struct rtsp_session *rtsp__session_connect(struct rtsp_module *mod, struct len_str *url);
long rtsp__close_session(struct rtsp_session *session);

/* for server */
long rtsp__on_request(struct rtsp_session *session);
long rtsp__send_err_reply( struct rtsp_session *session, long status, char *addon );
long rtsp__on_describe( struct rtsp_session *session );
long rtsp__send_describe_reply( struct rtsp_session *session );
long rtsp__on_get( struct rtsp_session *session );
long rtsp__send_get_reply( struct rtsp_session *session );
long rtsp__on_set_parameter( struct rtsp_session *session );
long rtsp__send_set_parameter_reply( struct rtsp_session *session );
long rtsp__on_post( struct rtsp_session *session );
long rtsp__send_post_reply( struct rtsp_session *session );
long rtsp__on_options( struct rtsp_session *session );
long rtsp__send_options_reply( struct rtsp_session *session );
long rtsp__on_setup( struct rtsp_session  *session );
long rtsp__send_setup_reply( struct rtsp_session *session, struct rtp_channel *channel);
long rtsp__on_play( struct rtsp_session *session );
long rtsp__send_play_reply( struct rtsp_session *session );
long rtsp__on_teardown( struct rtsp_session  *session );
long rtsp__send_teardown_reply( struct rtsp_session *session );
long rtsp__on_announce( struct rtsp_session *session);
long rtsp__send_announce_reply(struct rtsp_session *session);
long rtsp__on_record( struct rtsp_session *session );
long rtsp__send_record_reply( struct rtsp_session *session);
long rtsp__on_pause( struct rtsp_session *session );
long rtsp__send_pause_reply( struct rtsp_session *session );

/* both server and client can receive idr request, this is only for server play and client record */
long rtsp__on_ctrl (struct rtsp_session *session);
/* both sever and client can send idr request, this is only for server record and client play */ 
long rtsp__req_ctrl (struct rtsp_session *session, struct len_str *method, struct len_str *params);

/* for client */
long rtsp__on_response(struct rtsp_session *session);
long rtsp__on_options_reply(struct rtsp_session *session);
long rtsp__on_describe_reply(struct rtsp_session *session);
long rtsp__on_announce_reply(struct rtsp_session *session);
long rtsp__on_setup_reply(struct rtsp_session *session);
long rtsp__on_play_reply(struct rtsp_session *session);
long rtsp__on_record_reply(struct rtsp_session *session);
long rtsp__on_teardown_reply(struct rtsp_session *session);
long rtsp__on_unauthorized(struct rtsp_session *session);


long rtsp__req_describe( struct rtsp_session *session );
long rtsp__req_options( struct rtsp_session *session );
long rtsp__req_set_parameter( struct rtsp_session *session );

long rtsp__req_setup( struct rtsp_session *session, struct rtp_channel *channel);
long rtsp__req_play( struct rtsp_session *session );
long rtsp__req_teardown( struct rtsp_session *session );
long rtsp__req_announce( struct rtsp_session *session );
long rtsp__req_record( struct rtsp_session *session );

#endif /* defined(__RTSP_SESSION_H__) */
#if defined(__cplusplus)
}
#endif/* defined(__cplusplus) */
