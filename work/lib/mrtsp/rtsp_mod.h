/*!
\file       rtsp_mod.h
\brief      rtsp connection control  unit

 ----history----
\author     xionghuatao
\date       2009-11-09
\version    0.01
\desc       create

$Author: xionghuatao $
$Id: rtsp_mod.h,v 1.14 2009-11-09 17:28:27 xionghuatao Exp $
*/

#if defined(__cplusplus)
extern "C" {
#endif/* defined(__cplusplus) */

#ifndef __RTSP_MOD_H__
#define __RTSP_MOD_H__

#if defined(mrtsp_ssl_enable) && mrtsp_ssl_enable
#include "openssl/melib_openssl.h"
#include "openssl/ssl.h"
#endif

struct rtsp_session;
struct rtsp_module;
struct rtsp_support_method;
struct rtp_channel;
struct url_scheme;
struct sdp_block;
//struct mlock_simple_obj;
//#define mlock_simple struct mlock_simple_obj

/* rtsp server default port */
#define RTSP_SERVER_DEFAULT_PORT    554
#define RTSP_RTP_DEFAULT_UDP_PORT   6970
#define RTSP_RTCP_DEFAULT_UDP_PORT  6971
#define RTSP_MAX_CONN_SIZE          0x30

typedef enum rtsp_http_certification_type
{
    http_basic_certification = 0,
    http_abstract_certification
}rtsp_http_certification_type;


typedef enum rtsp_session_type {
    erst_client_play = 1,
    erst_client_record,
    erst_server_play,
    erst_server_record
} rtsp_session_type;

typedef enum {
    ersms_state_init = 0,
    ersms_state_ready,
    ersms_state_playing,
    ersms_state_recording
} rtsp_session_machine_state;

typedef enum  {
    erss_status_idle = 0,
    erss_status_init,
    erss_status_options,
    erss_status_describe,
    erss_status_announce,    
    erss_status_setup,
    erss_status_play,
    erss_status_playing,
    erss_status_record,
    erss_status_recording,
    erss_status_teardown,
    erss_status_pause,
    erss_status_unauthorized
} rtsp_session_status;

typedef struct rtsp_req_params 
{
    struct len_str  full_url;
    struct len_str  method;
    struct len_str  params;
} _rtsp_req_params;

typedef struct rtsp_fd_desc
{
    long            fd;                            
    unsigned long   port;                        
} _rtsp_fd_desc;

typedef long (* rtsp_on_media_data) (struct rtsp_session *session, unsigned char *data, unsigned long length, unsigned long timestamp, struct len_str  *format);
typedef long (* rtsp_server_on_req) (struct rtsp_session *session, struct rtsp_req_params *params);

typedef struct rtsp_params
{
    struct
    {
        unsigned long           udp_min;                    /*!< min udp port, if 0 use 10000 */
        unsigned long           udp_max;                    /*!< max udp port, if 0 use 20000 */
        unsigned long           rtsp_port;                  /*!< rtsp server port, if 0 will not open rtsp server port */     
        unsigned long           max_conns;                  /*!< max connection counts, if 0 use default:32 */
    }net;
    struct
    {
        rtsp_on_media_data      client_play_data;        /*!< for client, deal with the data , which the {play} client have received from server */
        rtsp_on_media_data      server_record_data;      /*!< for server, deal with the data , which the server have received from the {record} client */ 
        
        rtsp_server_on_req      server_play_req;         /*!< for server, deal with the play request */
        rtsp_server_on_req      server_record_req;       /*!< for server, deal with the record request */
        
        rtsp_server_on_req      server_play_close;       /*!< for server, deal with the server play channel */
        rtsp_server_on_req      server_record_close;     /*!< for server, deal with the server record channel */
        rtsp_server_on_req      client_play_close;       /*!< for client, deal with the client play channel */
        rtsp_server_on_req      client_record_close;     /*!< for client, deal with the client record channel */
        
        rtsp_server_on_req      server_play_ctrl;        /*!< for server, on idr */
        rtsp_server_on_req      client_record_ctrl;      /*!< for client, on idr */
    } events;

	#if mrtsp_ssl_enable
    struct{
        int ssl_enable;
        struct len_str *cert;
        struct len_str *chain;
        struct len_str *prik;
    } ssl;
	#endif
} _rtsp_params;

#define rtsp_x_session_type_none    0
#define rtsp_x_session_type_get     1
#define rtsp_x_session_type_post    2

/*! \brief get rtsp session recv header */
#define rtsp_session_get_recv_msg_pt(_session) (((rtsp_x_session_type_get == (_session)->x_session.type)\
                                                 && (_session)->x_session.ref\
                                                 && (_session)->x_session.ref->hmsg)?\
                                                        &(_session)->x_session.ref->hmsg:\
                                                        &(_session)->hmsg)
#define rtsp_session_get_recv_msg(_session)     (*rtsp_session_get_recv_msg_pt(_session))

typedef struct rtsp_session_desc
{
    struct  
    {
        unsigned long   type:1;
        unsigned long   url:1;
        unsigned long   user:1;
        unsigned long   password:1;
        unsigned long   sdp:1;
        unsigned long   refer:1;
    }flag;
    struct len_str  type;       /*!< "play","record", if default is play */
    struct len_str  url;        /*!< url */
    struct len_str  user;       /*!< username */
    struct len_str  password;   /*!< password */
    struct len_str  sdp;        /*!< sdp info */
    void            *refer;
}_rtsp_session_desc;
#define rtsp_session_desc_format_s  "%p"
#define rtsp_session_desc_format(_desc) (_desc)

#define rtsp_session_close_reason_by_user           1
#define rtsp_session_close_reason_by_teardown_reply 2

struct sample_buff_node{
	size_t len;
	unsigned long timestamp;
	unsigned char *data;
	struct {
		struct sample_buff_node *prev;
		struct sample_buff_node *next;
	}node;
};

typedef struct rtsp_session
{    
    struct 
    {
        struct rtsp_module      *mod;
        struct rtsp_session     *prev;
        struct rtsp_session     *next;        
    } in_list;     
    
    struct rtsp_sock            *sock;                      /*!< sock refer */

    rtsp_session_type           session_type;               /*!< rtsp session type */
    rtsp_session_machine_state  machine_state;              /*!< rtsp session machine state */
    rtsp_session_status         status;                     /*!< session method state */

    void                        *refer;                     /*!< refer data */  

    unsigned long               close_reason;               /*!< close reson: 1:user, 2:inner.... */

	#if mrtsp_ssl_enable
    SSL *ssl;
	#endif
	
    unsigned long               callback_times;             /*!< now in callback times */

    unsigned long               have_send_sdp;              /*!< have send out sdp */
    struct sockaddr_in          remote_addr;                /*!< the ip addr */    
    char                        session_id[32];             /*!< session id ,setup create this */  
    unsigned long               tick;                       /*!< the time tick of session */
    unsigned long               prev_set_tick;              /* chenyong heatbeat */
    unsigned long               cseq;                       /*!< the rtsp message sequence , refer to method request */ 
    struct http_msg             *hmsg;                      /*!< parse the message received and store it, if rtsp method request */

    unsigned long               req_counts;                 /*!< request counts */
    struct  
    {
        unsigned long           type;                       /*!< rtsp-xession type */
        struct rtsp_session     *ref;                       /*!< refer rtsp session */
        struct len_str          cookie;                     /*!< x-sessioncookie, if post all data will be trans into get */
    }x_session;
    
    rtsp_on_media_data          on_media_data;              
    rtsp_server_on_req          on_req;
    rtsp_server_on_req          on_close;
    rtsp_server_on_req          on_ctrl;                    /* such as idr */

    struct rtp_channel          *video_channel;             /*!< video rtp channel */
    struct rtp_channel          *audio_channel;             /*!< audio rtp channel */
	unsigned long rtp_prior_audio_timestamp;				/* prior audio rtp timestamp */
	unsigned long rtp_audio_count;							/* audio rtp count */
	unsigned long rtp_video_count;							/* video rtp count */
	unsigned long rtp_audio_octet;							/* audio octet */
	unsigned long rtp_video_octet;							/* video octet */
	unsigned long rtp_audio_ssrc;							/* audio ssrc */
	unsigned long rtp_video_ssrc;							/* video ssrc */
	unsigned long rtp_fiducial_audio_timestamp;				/* the fiducial timestamp of audio */
	struct timeval rtp_fiducial_audio_time;					/* the fiducial time of audio */
	int rtp_allowed_latency;								/* allowed latency of rtp */
	
	struct {
		size_t counts;
		struct sample_buff_node *list;
	}audio_sample_buff;										/* audio sample buffer */

	unsigned long last_video_sample_time;					/* sample time of last recived video */

    struct  
    {
        struct rtsp_support_method      *list;                      
        unsigned long                   counts;    
        unsigned long                   query_flag;         /*!< is send query method request,such as options ? */
    } methods;
    
    struct 
    {
        struct url_scheme       desc;                       
        char                    *data;
        unsigned long           len;        
    } url;                                                  /*!< the media url such as  rtsp://192.168.2.104:554/sample_100kbit.mp4 */

    struct  
    {
        struct len_str          user;
        struct len_str          password;
    }authorzation;
    
    struct
    {        
        struct sdp_block        *desc;
        char                    *data;  
        unsigned long           len;
        unsigned long           stream_counts;        
    } sdp;                                                  /*!< sdp information */
    
    struct 
    {
        struct rtp_channel      *list;                      
        unsigned long           counts;    
        unsigned long           rtp_started;                /*!< is rtp channels ready to work? */

        unsigned long           tcp_priority;               /*!< tcp priority */
    } rtp_sessions;                                         /*!< rtp channels */

    struct
    {
        char digest_realm[64];
        char nonce[64];
        int enable;
        rtsp_http_certification_type type;
        char username[64];
        char password[64];
    } http_auth;
    unsigned long prev_timestamp; //chenyong add 2019-1-11
    unsigned long time_stamp;
//    void                        *sort_channel;              /*!< rtp package sort channel */
} _rtsp_session;
#define rtsp_session_format_s   "%p{url[%s]}"
#define rtsp_session_format(_session) (_session), ((_session) && (_session)->url.len)?(_session)->url.data:NULL

typedef struct rtsp_module
{  
    /* connections epolling */
    long                            epoll_fd;           /*!< epoll polling handle, i.e. ,epoll file description   */
    unsigned long                   max_conn_counts;    /*!< the max size the epoll can connect to clients */
    struct netx_event               *events;            /*!< the events that epoll have been touched off */    
   
    unsigned long                   start_tick;         /*!< server start time  */
    unsigned long                   now_tick;           /*!< current time */
    unsigned long                   time_out;           /*!< the limit time out .the server would willingly  close the connection */      
    
    mlock_simple                    *lock;
    struct in_addr                  local_addr;         /*!< the server ip address */

	#if mrtsp_ssl_enable
	int ssl_enable;
    char *cert;
    char *chain;
    char *prik;
    SSL_CTX *ssl_ctx;
	#endif
	
    /* server open ports */
    struct rtsp_fd_desc             rtsp_server;        /*!< server socket on tcp 554 */
    struct rtsp_fd_desc             rtp_server;         /*!< rtp sock[udp], consider that send media data on this port 6970 */
    struct rtsp_fd_desc             rtcp_server;        /*!< rtcp sock[udp] send rtcp message on this port, default 6971 */

    struct rtsp_sock                *rtsp_sock;
    struct rtsp_sock                *rtp_sock;
    struct rtsp_sock                *rtcp_sock;

    /* connections info storage */    
    struct rtsp_session             *first_conn;        /*!< point to the first connection allocation */    
    struct
    {
        unsigned long               counts;         
        struct rtsp_session         *list;              /*!< free connections */
    } frees;
    struct 
    {               
        unsigned long               counts;                  
        struct rtsp_session         *list;              /*!< used connetions */
    } useds;
    
    struct 
    {
        unsigned long               min_udp_port;
        unsigned long               max_udp_port;
        unsigned long               used_index;
    } udp_ports;

    struct rtsp_params              params;             /*!< the callback functions */
    void                            *refer;             /*!< refer data */   
} _rtsp_module;
#define rtsp_module_format_s    "%p"
#define rtsp_module_format(_mod)    (_mod)

#define rtsp_mod_get_refer(t_mod)                   ((t_mod)->refer)
#define rtsp_mod_set_refer(t_mod, t_refer)          ((t_mod)->refer = t_refer)
#define rtsp_session_get_refer(t_session)           ((t_session)->refer)
#define rtsp_session_set_refer(t_session, t_refer)  ((t_session)->refer = (t_refer)) 
#define rtsp_session_get_mod(t_session )            ((t_session)->in_list.mod)
#define rtsp_session_set_mod(t_session, t_mod)      ((t_session)->in_list.mod = (t_mod))
#define rtsp_session_get_sock(t_session)            ((t_session)->sock)
#define rtsp_session_set_sock(t_session, t_sock)    ((t_session)->sock = (t_sock))
#define rtp_channel_get_session(t_channel)          ((t_channel)->in_list.session)
#define rtp_channel_set_session(t_channel, t_session)   ((t_channel)->in_list.session = (t_session))
        
/*!
func    rtsp_mod_create
\brief  create net module
\param  params[in]              callback routines, must be not null, if null, rtsp module create failed
\return the pointer to net module
        #NULL                   failed
        #other                  net module
*/
struct rtsp_module *rtsp_mod_create (struct rtsp_params  *params);

/*!
func    rtsp_mod_destroy
\brief  destroy net module, terminate the rtsp server
\param  mod [in]                the net module created by rtsp_mod_create()
\return the result of destroying the module 
        #0                      succeed
        #other                  error code
*/
long rtsp_mod_destroy (struct rtsp_module *mod);

/*!
func    rtsp_mod_wait
\brief  deal with the epoll waiting
\param  mod[in]                 the net module created by rtsp_mod_create()
\param  timeout[in]             the time out value(ms). the parameter pass to netx_wait(session->state, event, timeout)
\returnwait result 
        #0                      succeed
        #other                  error code
*/
long rtsp_mod_wait (struct rtsp_module *mod,  unsigned long timeout);

/*!
func    rtsp_session_create
\brief  connect to rtsp server, to play|record media
\param  mod [in]                the net module create by rtsp_mod_create()
\param  url [in]                the rtsp url , such as  rtsp://192.168.2.104:554/sample_100kbit.mp4
\return create result 
        #NULL                   failed
        #the pointer            succeed
*/
struct rtsp_session *rtsp_session_create(struct rtsp_module *mod, struct rtsp_session_desc *desc);

/*!
func    rtsp_session_destroy
\brief  close the rtsp session, which create by rtsp_client_play or rtsp_client_record
\param  session[in]            the pointer to rtsp session 
\return create result 
        #0                      succeed
        #other                  error code
*/
long rtsp_session_destroy (struct rtsp_session *session);

/*!
func    rtsp_session_send_media_data
\brief  send media data, only for client record and server play
\param  session[in]             rtsp session  
\param  data[in]                the media data need to be sent
\param  length[in]              media data size
\param  timestamp[in]           data timestamp
\param  data_format[in]         video or audio , create it by len_str _from_const ("video") and vice versa       
\return send result 
        #0                      succeed
        #other                  error code
*/
long rtsp_session_send_media_data ( struct rtsp_session *session, unsigned char *data, unsigned long length, unsigned long timestamp, struct len_str  *format, int has_video_is);

/*!
func    rtsp_session_update
\brief  set session data, such as user, password, refer, sdp
\param  session[in]             the session
\param  data[in]                the datas
\return create result 
        #NULL                   failed
        #the pointer            succeed
*/
long rtsp_session_update(struct rtsp_session *session, struct rtsp_session_desc *desc);

/*!
func    rtsp_session_ctrl
\brief  ctrl, such as idr, mbw
\param  session[in]             rtsp session  
\return invoke result 
        #0                      succeed
        #other                  error code
*/
long rtsp_session_ctrl(struct rtsp_session *session, struct len_str *method, struct len_str *params);

#endif /* !defined(__RTSP_MOD_H__) */
#if defined(__cplusplus)
}
#endif/* defined(__cplusplus) */
