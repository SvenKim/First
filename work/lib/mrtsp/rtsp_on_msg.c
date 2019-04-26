/*!
\file       rtsp_on_msg.c
\brief      rtsp unit

----history----
\author     xionghuatao
\date       2009-11-09
\version    0.01
\desc       create

$Author: xionghuatao $
$Id: rtsp.c,v 1.14 2009-11-09 17:28:27 xionghuatao Exp $
*/

#define mmodule_name "lib.mlib.mrtsp.rtsp_on_msg"
#include "mcore/mcore.h"
#include "msdp/msdp.h"
#include "rtsp_mod.h"
#include "rtsp__session.h"
#include "rtp__packet.h"
#include "rtp__mod.h"

static const struct len_str rtsp__s_idr = {len_str_def_const("idr")};

#define SOAP_WSSE_NONCELEN (128) //chenyong


void rtsp__get_http_auth_args(struct rtsp_session *session , char *p)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__get_http_auth_args(session[%p], p[%s]) "
#define func_format()   session,p

    int n = 0;
    p += strlen ("WWW-Authenticate: ");

    #define RETRIEVE_FIELD(_p, _name) do { \
        char *start; \
        char *s; \
        if ((start = strstr (p, _name)) && (s = strchr (start, '"'))) \
        { \
            char *end = strchr (s + 1, '"'); \
            if (n = (end - s - 1)) \
            { \
                memcpy (_p, (s+ 1), n); \
                _p[n] = 0; \
            } \
            else \
            { \
                _p[0] = 0; \
            } \
        } \
    } while (0)

    if (strstr (p, "WWW-Authenticate: Digest"))
    {
        RETRIEVE_FIELD(session->http_auth.digest_realm, "realm");
        RETRIEVE_FIELD(session->http_auth.nonce, "nonce");
        session->http_auth.type = http_abstract_certification; //abstract Certification;
    }
    else
    {
        RETRIEVE_FIELD(session->http_auth.digest_realm, "realm");
        session->http_auth.type = http_basic_certification; //basic Certification;
    }

}





int rtsp__calc_response(struct rtsp_session *session, unsigned char *response, char *url,char *method)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__calc_response(session[%p], response[%s], url[%s], method[%s]) "
#define func_format()   session,response,url,method

    char buf[256];
    char HA1[256];
    char md5_HA1[16];
    char md5_HA2[16];
    char HA2[256];
    int n;
    char *qop="auth";
    int i = 0;


    if (session->authorzation.user.len == 0)
    {
       print_log0(err, "rtsp__calc_response() fail.");
       return -1;
    }

    //HA1
    n = sprintf(buf, "%s:%s:%s", session->authorzation.user.data, session->http_auth.digest_realm, session->authorzation.password.data);
    md5_ex_encrypt_hex(buf, n, HA1);
    //md5_ex_encrypt(HA1, n, md5_HA1);

    //HA2
    n = sprintf(buf, "%s:%s", method, url);
    md5_ex_encrypt_hex(buf, n, HA2);
   // md5_ex_encrypt(HA2,n,md5_HA2);

    //response
    n = sprintf (buf ,"%s:%s:%s",HA1,session->http_auth.nonce,HA2);
   md5_ex_encrypt_hex (buf, n, response);
    //md5_ex_encrypt(response,n,md5_HA1);
    //md5_ex_encrypt_hex(md5_HA1, sizeof(md5_HA1), response);

    return 0;
}

int rtsp_add_authorition(struct rtsp_session *session, char * authorition, char *method)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_add_authorition(session[%p], authorition[%p], method[%s]) "
#define func_format()   session, authorition, method

    char respon[128];

    if (NULL == authorition)
    {
        print_log0(err, "rtsp_add_authorition() fail.");
        return -1;
    }
    //print_log2(err, "####rtsp_add_authorition() username:[%s],password:[%s]",session->authorzation.user.data, session->authorzation.password.data);
    if (session->http_auth.enable)//chenyong
    {
        if (session->http_auth.type == http_abstract_certification)
        {
            rtsp__calc_response(session, respon,session->url.data, method);
            sprintf(authorition, "Authorization: Digest username=\"%s\", "
                "realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\""RTSP_EL,
            (session->authorzation.user.len > 0) ? session->authorzation.user.data:"",
            session->http_auth.digest_realm, session->http_auth.nonce,
            session->url.data, respon);

        }
        else
        {
            sprintf(authorition, "%s%s"RTSP_EL, "Authorization: Basic ", "YWRtaW46YWRtaW4=");
        }
    }
    return 0;
}

void rtsp__get_session_id( char *ssrc, unsigned long len )
{
    ssrc[len] = 0;
    while (len--)
    {
        ssrc[len] = (char)(((unsigned long)rand()) % 10 + 48); 
    }  
}

const char *rtsp__add_time_stamp( )
{    
    static char dest[128];     
    time_t      now = time(NULL);
    strftime(dest, 128, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    return (const char*)dest;
}

long rtsp__rtp_get_port_pair( struct rtsp_module *mod, struct rtsp_fd_desc *rtp, struct rtsp_fd_desc *rtcp)
{
    unsigned end, step = 2;  

    rtcp->fd = -1;
    while(step--)
    {
        end = step?mod->udp_ports.max_udp_port:mod->udp_ports.used_index;
        rtp->port = step?mod->udp_ports.used_index:mod->udp_ports.min_udp_port;
        for(rtcp->port = rtp->port + 1; rtcp->port < end; rtp->port += 2, rtcp->port += 2)
        {
            if ((0 <= (rtp->fd = netx_open(SOCK_DGRAM, &mod->local_addr, rtp->port, 0)))
                && (0 <= (rtcp->fd = netx_open(SOCK_DGRAM, &mod->local_addr, rtcp->port, 0))))
            {
                long size = 0x100000; 
                mod->udp_ports.used_index = rtp->port + 2;
                setsockopt(rtp->fd, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(int)); 
                return 0 ;
            }
            if(0 < rtp->fd){ netx_close(rtp->fd); };
        }
    }    
    return -1;          
}

typedef struct rtsp_transport_header
{
    struct rtsp_pair           client_port;
    struct rtsp_pair           server_port;    
    struct rtsp_pair           channel_nld;  
    long                       ssrc;    
    rtp_transport_type         type;          
} _rtsp_transport_header;

long rtsp__parse_transport_field( struct len_str *transport, struct rtsp_transport_header *header)
{
    struct len_str    name, value;
    char              *s= transport->data, *s_end = s + transport->len; 
    unsigned long     flag = 0;
   
    header->type = ertt_rtp_avp_udp;       
    for( name.data = s, value.data = NULL ;s < s_end; ++s)
    {
        switch(*s)
        {
        case ',':
            { 
                flag = 2;                
                break;
            }
        case ';':      
            {/* sub field end */                
                flag = 1;
                break;
            }
        case '=':
            {/* field name end */
                name.len = s - name.data;
                value.data = s + 1;
                break;
            }       
        default:
            {
                if ((flag = ( (s + 1) == s_end)))
                {
                   s++;
                }
                break;
            }
        }
        /* end of sub string */
        if (flag)
        {
            if(0 == (value.len = value.data?(s - value.data):0))
            {/* just have name */
                name.len = s - name.data;
                if((0 == len_str_cmp_const(&name, "RTP/AVP")) 
                    || (0 == len_str_cmp_const(&name, "RTP/AVP/UDP")))
                {
                    header->type = ertt_rtp_avp_udp;                    
                }
                else if(0 == len_str_cmp_const(&name, "RTP/AVP/TCP"))
                {
                    header->type = ertt_rtp_avp_tcp;
                }                  
            }
            else
            {/* have name and value */
                int rtp_v = 0, rtcp_v = 0;
                if(0 == len_str_cmp_const(&name, "client_port"))
                {               
                    if (2 != sscanf( value.data, "%d-%d", &rtp_v, &rtcp_v))
                    {
                        return -1;
                    }
                    header->client_port.rtp = (unsigned long)rtp_v;
                    header->client_port.rtcp = (unsigned long)rtcp_v;
                }
                else if(0 == len_str_cmp_const(&name,"server_port"))
                {                       
                    if (2 != sscanf( value.data, "%d-%d", &rtp_v, &rtcp_v))
                    {
                        return -1;
                    }
                    header->server_port.rtp = (unsigned long)rtp_v;
                    header->server_port.rtcp = (unsigned long)rtcp_v;
                }
                else if(0 == len_str_cmp_const(&name, "mode"))
                {
                    
                }
                else if(0 == len_str_cmp_const(&name, "ssrc"))
                {                      
                    if (1 != sscanf(value.data, "%8lx", &header->ssrc))
                    {
                        return -1;
                    }
                }   
                else if(0 == len_str_cmp_const(&name, "interleaved"))
                {
                    int rtp_v = 0, rtcp_v = 0;
                    if (2 != sscanf( value.data, "%d-%d", &rtp_v, &rtcp_v))
                    {
                        return -1;
                    }
                    header->channel_nld.rtp = (unsigned long)rtp_v;
                    header->channel_nld.rtcp = (unsigned long)rtcp_v;
                }
            }
            name.data = s + 1;
            value.data = NULL;
            if(2 == flag)
            {
                return 0;
            }               
        }        
    }
    return 0;
}


long rtsp__check_support_method (struct rtsp_session *session, struct len_str *method)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__check_support_method(session["rtsp_session_format_s"], method["len_str_format_s"])"
#define func_format()   rtsp_session_format(session), len_str_format(method)

    long                        flag = 0;
    struct rtsp_support_method  *tmp_node = session->methods.list;
    struct len_str              tmp_method;      

    print_log0(debug, ".");
    do 
    {
        if (tmp_node)
        {
            tmp_method.len = tmp_node->len;
            tmp_method.data = tmp_node->method; 

            if (0 == len_str_casecmp(&tmp_method, method))
            {
                flag = 1;
                break;
            }
        }
        else 
        {
            print_log0(err, "method has NULL node.");
            return -1;
        }
    } while (NULL != (tmp_node = tmp_node->in_list.next) && (tmp_node != session->methods.list));

    if (0 == flag)
    {
        print_log0(err, "the remote do not support this method.");
        return -1;
    }

    print_log0(debug, "check succeed.");
    return 0; 
}

long rtsp__add_channel_sock(struct rtsp_module *mod, struct rtp_channel *channel )
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__add_channel_sock(mod["rtsp_module_format_s"], channel["rtp_channel_format_s"])"
#define func_format()   rtsp_module_format(mod), rtp_channel_format(channel)

    struct rtsp_sock   *sock;      
    struct netx_event  evt;
    long               step = 2;
    
    print_log0(debug, ".");

    if ((ertt_rtp_avp_udp != channel->transport_type))
    {
        print_log0(debug, "transport type is not udp.");
        return 0;    
    }

    evt.events = netx_event_et | netx_event_in;
    while (step--)
    {
        sock = step?channel->transport.udp.rtp_sock:channel->transport.udp.rtcp_sock;  
        sock->type = step?erst_rtp:erst_rtcp;
        sock->owner_refer = channel;
        evt.data.ptr = sock;             
        if (netx_ctl(mod->epoll_fd, netx_ctl_add, sock->fd, &evt))
        {
            print_log0(err, "add udp fd to epoll polling failed.");
            return -1;
        }        
    }        
    return 0;
}

/*
struct sdp_xfield *sdp_get_media_attr( struct sdp_media *media, char *name, unsigned long len )
{
    unsigned long       i = 0, fieldCount = 0;
    struct sdp_xfield   *fields = NULL;
    
    if ((NULL == media) || (NULL == name))
    {
        print_warn("[%s] invalid parameter when get the media attribe. %s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        return NULL;
    }
    
    fieldCount = media->media_layer.attr.counts;
    fields = media->media_layer.attr.list;    
    
    for(i = 0; i < fieldCount; i++)
    {
        if((fields->key.len == len) && (0 == memcmp(fields->key.data, name, len)))
        {
            return fields;
        }
        fields = fields->out_list.next;
    }
    return NULL;
}
*/

long rtsp__on_sdp(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_sdp(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtp_channel   *channel;
    struct sdp_xfield    *control_id;
    struct sdp_media     *media = session->sdp.desc->medias.list;  
    struct len_str       *sdp_str;
    
    print_log0(debug, ".");

    if ( session->sdp.desc && session->sdp.desc->medias.counts )
    {
        do 
        {
            if (NULL == (channel = rtsp__rtp_channel_create(session)))
            {
                print_log0(err, "failed when rtsp__rtp_channel_create().");
                return -1;;                    
            }
            if(session->rtp_sessions.tcp_priority)
            {
                channel->transport_type = ertt_rtp_avp_tcp;
            }
            if ( 0 == len_str_cmp_const(&(media->media_layer.announce.media_descr), "video"))
            {
                int64_t value = 0;
                sdp_str = &(media->media_layer.announce.format);
                channel->pack = rtp_decode_create_packet();
                channel->media_type = ermt_video;
                session->video_channel = channel;                
                pack_str_to_num(sdp_str->data, sdp_str->data + sdp_str->len, &value);                                
                channel->payload_type = (unsigned long)value;
/*                RTP_SetDecodeVideoParam(session->sort_channel, channel->payload_type, 127, 
                                        (ON_RTP_DECODE_VIDEO_OPUT)rtsp__on_rtp_video_out, session ); */
            }
            else if( 0 == len_str_cmp_const(&(media->media_layer.announce.media_descr), "audio") )
            {
                int64_t value = 0;
                sdp_str = &(media->media_layer.announce.format);  
                channel->pack = rtp_decode_create_packet();
                channel->media_type = ermt_audio;
                session->audio_channel = channel;
                pack_str_to_num(sdp_str->data, sdp_str->data + sdp_str->len, &value);
                channel->payload_type = (unsigned long)value;
/*                RTP_SetDecodeAudioParam(session->sort_channel, channel->payload_type, 127,
                                        (ON_RTP_DECODE_AUDIO_OPUT)rtsp__on_rtp_audio_out, session);*/
            }
            else
            {
                print_log0(err, "meet unknown media type.");
            }            
            sprintf(channel->media_url, "%*.*s/", 0, (int)session->url.len, session->url.data); 
            if ((control_id = sdp_get_media_attr(media, "control", sizeof("control") - 1)))
            {                                       
                strncat(channel->media_url, control_id->value.data, control_id->value.len);
            }                       
        } while ( (media = media->out_list.next) && (media != session->sdp.desc->medias.list));     
    }
    else
    {  
        print_log0(err, "parse the sdp information , have no any media stream.");
        return -1;
    }    
    return 0;
}

long rtsp__add_rtsp_package(struct rtsp_session *session, unsigned char *data, unsigned long len)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__add_rtsp_package(session["rtsp_session_format_s"], data[%s{%*.*s%s}], len[%ld])"
#define func_format()   rtsp_session_format(session), data, 0, (len > 32)?32:len, data, (len > 32)?"...":"", len

    struct rtsp_module *mod = rtsp_session_get_mod(session);
    struct rtsp_sock   *sock = session->sock; 
    struct rtsp_data   *sdata;    
    struct netx_event  evt = {0};     
    
    print_log0(debug, ".");

    if ( (NULL == (sdata =  (struct rtsp_data*)calloc(1, sizeof(struct rtsp_data)))
        || (NULL == (sdata->data = (unsigned char*)malloc(len + 1)))))
    {
        print_log0(err, "alloc memory failed.");
        if (sdata)  free(sdata);    
        return ERR_ALLOC_FAILED;
    }    
    memcpy(sdata->data, data, len);      
    sdata->len = len;   
    mlist_add(sock->send, sdata, in_list);
    sock->send.cache_size += sdata->len;
	
    evt.events = netx_event_et | netx_event_in | netx_event_out;
    evt.data.ptr = session->sock;                           
    if ( mod->epoll_fd && netx_ctl(mod->epoll_fd, netx_ctl_mod, sock->fd, &evt))
    {             
        print_log0(err, "failed when add sock into poll.");
        return ERR_GENERIC;
    } 

    if(data[0] != 0x24)
    {
        print_log1(detail, "\r\n-----------rstp.send---------------\r\n%s\r\n" ,data);   
    }
    return ERR_NO_ERROR;
}

long rtsp__add_rtp_package(struct rtp_channel *channel, unsigned char *data, unsigned long len, long data_type)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__add_rtp_package(channel["rtp_channel_format_s"], data[%p], len[%ld], data_type[%ld])"
#define func_format()   rtp_channel_format(channel), data, len, data_type

    struct rtsp_session     *session = rtp_channel_get_session(channel);
    struct rtsp_module      *mod = rtsp_session_get_mod(session);
    struct rtsp_sock        *sock;
    struct rtsp_data        *sdata;    
    struct netx_event       evt = {0};     

    print_log0(detail, ".");
    
    if ( (NULL == (sdata =  calloc(1, sizeof(struct rtsp_data)))
        || (NULL == (sdata->data =  malloc(len + 5)))))
    {
        print_log0(err, "failed when malloc().");
        if (sdata)  free(sdata);    
        return ERR_ALLOC_FAILED;
    }    

    if ( ertt_rtp_avp_tcp == channel->transport_type)
    {
        unsigned short channel_ntlvd;
        channel_ntlvd =  (unsigned short)(( erst_rtcp == data_type) ? channel->transport.tcp.interleaved_id.rtcp:channel->transport.tcp.interleaved_id.rtp);           
        sdata->data[0] = 0x24;
        sdata->data[1] = (unsigned char ) channel_ntlvd;
        sdata->data[2] = (unsigned char )( (len >> 8) & 0xff );
        sdata->data[3] = (unsigned char )( len & 0xff );
        sdata->len += 4;       
        sock = session->sock;
        evt.events = netx_event_et | netx_event_in | netx_event_out;
    }
    else
    {
        sock = ( erst_rtcp == data_type)? mod->rtcp_sock: mod->rtp_sock;
        sdata->addr = channel->transport.udp.rtp_addr;
        evt.events = netx_event_et | netx_event_out;        
    }
    memcpy(sdata->data + sdata->len, data, len);      
    sdata->len += len;   
    mlist_add(sock->send, sdata, in_list); 
    sock->send.cache_size += sdata->len;
	
#define	cache_size_limit (1*1024*1024)		//cache_size limit	
	while(sock->send.cache_size > cache_size_limit)
	{
		sdata = sock->send.list;
		sock->send.cache_size -= sdata->len;		
		mlist_del(sock->send, sdata, in_list);
		free(sdata->data);
		free(sdata);
		sock->send.index = 0;
	}
	
    evt.data.ptr = sock;                           
    if ( mod->epoll_fd && netx_ctl(mod->epoll_fd, netx_ctl_mod, sock->fd, &evt))
    {             
        print_log0(err, "failed when mod epoll fd.");    
        return ERR_GENERIC;
    }     
    return ERR_NO_ERROR;
}

long rtsp__parse_public_field (struct len_str *field_data, struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__parse_public_field(field_data["len_str_format_s"], session["rtsp_session_format_s"])"
#define func_format()   len_str_format(field_data), rtsp_session_format(session)

    char                *s= field_data->data, *s_end = s + field_data->len; 
    unsigned long       flag;
    struct len_str      method;
    struct rtsp_support_method  *tmp_node = NULL;
    
    print_log0(debug, ".");

    for (method.data = s; s < s_end; ++s)
    {
        switch (*s)
        {
        case ' ':
            {
                if (method.data == s)
                {
                    method.data = s + 1;
                }
                flag = 0;
                break;
            }
        case ',':
            {
                flag = 1;
                break;
            }
        default:
            {
                if ((flag = (s + 1) == s_end))
                {
                   s++;
                }
                break;
            }
        }

        if (flag)
        {
            method.len = s - method.data;

            if (NULL == (tmp_node = (struct rtsp_support_method *) calloc(1, sizeof(struct rtsp_support_method))))
            {
                print_log0(err, "failed when alloc memory.");
                return -1;
            }

            tmp_node->in_list.session = session;
            tmp_node->len = method.len;
            memcpy(tmp_node->method, method.data, method.len);
            mlist_add(session->methods, tmp_node, in_list);
            method.data = s + 1;

            if (flag == 2)
            {
                return 0;
            }
        }
    }
    return 0;
}

long rtsp__on_options_reply(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_options_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct len_str     *p_public;    
    struct http_msg    *hmsg = rtsp_session_get_recv_msg(session);

    print_log0(debug, ".");

    if ( NULL == (p_public = http_msg_get_known_header(hmsg, ehmht_public))
        || rtsp__parse_public_field(p_public, session))
    {/* dont's error here, to pass in most system */
        print_log0(warn, "failed when get public field.");
    }

    session->methods.query_flag = 2;

    if (erst_client_play == session->session_type )
    {      
        return rtsp__req_describe(session);      
    }
    else if (erst_client_record == session->session_type )
    {     
        return rtsp__req_announce(session);                       
    }        
    else if (ersms_state_playing == session->machine_state || ersms_state_recording == session->machine_state)
    {   
        return rtsp__req_ctrl(session, (struct len_str*)&rtsp__s_idr, NULL);
    }
    return 0;
}  

long rtsp__on_describe_reply(struct rtsp_session *session)
{              
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_describe_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct http_msg    *hmsg = rtsp_session_get_recv_msg(session);
    unsigned long       content_len = http_msg_get_content_len(hmsg);
    unsigned long stat;

    print_log0(debug, ".");



    if(0 >= (long)content_len)
    {
        if ((stat = http_msg_get_rsp_code_value(hmsg)) == 401)
        {
            return rtsp__on_unauthorized(session);
        }

        print_log0(err, "missing sdp.");
        return -1;
    }
    session->sdp.len = content_len;

    if((NULL == (session->sdp.data = (char*)malloc(content_len + 1)))
        ||(content_len != http_msg_get_content(hmsg, content_len + 1, session->sdp.data)))
    {
        print_log1(err, "%s failed.", session->sdp.data?"malloc":"get content");
        return -1;
    }
    session->sdp.data[content_len] = 0;
    session->sdp.desc = sdp_create(session->sdp.data, session->sdp.len);
    if(NULL == session->sdp.desc)
    {
        print_log0(err, "faield when sdp_create().");
    }
  
    if(rtsp__on_sdp(session))
    {
        print_log0(err, "faield when rtsp_on_sdp().");
        return -1;
    }
    return rtsp__req_setup(session, session->rtp_sessions.list);   
}

long rtsp__on_announce_reply(struct rtsp_session *session)
{   
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_announce_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");

    if ( 0 >= session->rtp_sessions.counts)
    {
        print_log0(err, "missing rtp channel.");
        return -1;
    }
    return rtsp__req_setup(session, session->rtp_sessions.list);
}

long rtsp__on_setup_reply(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_setup_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_transport_header transport = {0};
    struct len_str     *p_session, *p_transport;    
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct rtp_channel  *last_channel = session->rtp_sessions.list,
                        *next_channel = last_channel->in_list.next;
    unsigned long       step = session->sdp.stream_counts;

    print_log0(debug, ".");

    while (--step)
    {
        last_channel = last_channel->in_list.next;
        next_channel = last_channel->in_list.next;
    }
    
    if (( p_session = http_msg_get_known_header(hmsg, ehmht_session)))
    {                    
        memcpy(&(session->session_id[0]), p_session->data, p_session->len);    
        session->session_id[p_session->len] = 0; 
    }
    
    if ( NULL == (p_transport = http_msg_get_known_header(hmsg, ehmht_transport))
        || (0 != rtsp__parse_transport_field(p_transport, &transport)) )
    {
        print_log1(err, "%s transport info.", p_transport?"invalid":"missing");
        return -1;
    }

    if ( ertt_rtp_avp_udp == transport.type )
    {
        struct  sockaddr_in     addr = session->remote_addr;         
        last_channel->transport_type = transport.type;                  
        addr.sin_port = htons( (unsigned short) transport.server_port.rtp);
        last_channel->transport.udp.rtp_addr = addr;
        addr.sin_port = htons( (unsigned short) transport.server_port.rtcp);
        last_channel->transport.udp.rtcp_addr = addr;                                        
    }
    else if( ertt_rtp_avp_tcp == transport.type )
    {
        last_channel->transport_type = transport.type;      
        last_channel->transport.tcp.interleaved_id = transport.channel_nld;            
    }  

    /* set replay change the machine state to ready */
    session->machine_state = ersms_state_ready;

    if ( next_channel != session->rtp_sessions.list)
    {
        return rtsp__req_setup(session, next_channel);
    }
    else
    {
        return (erst_client_play == session->session_type )? rtsp__req_play(session):rtsp__req_record(session);
    }   
    return 0;
}   

long rtsp__on_play_reply(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_play_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");

    session->rtp_sessions.rtp_started = 1;    
    session->machine_state = ersms_state_playing;

    return 0;
}

long rtsp__on_record_reply(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_record_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long                ret;
    struct rtsp_module  *mod = rtsp_session_get_mod(session);

    print_log0(debug, ".");

    session->rtp_sessions.rtp_started = 1;    
    session->machine_state = ersms_state_recording;
    
    if(NULL == session->on_ctrl)
    {
        print_log0(warn, "missing on ctrl callback.");
    }
    else
    {
        struct rtsp_req_params  req_params = {0};
        len_str_set_const(&req_params.method, "idr");

        ++session->callback_times;
        mlock_simple_release(mod->lock);
        ret = session->on_ctrl(session, &req_params);
        mlock_simple_wait(mod->lock);
        --session->callback_times;

        if(ret)
        {
            print_log1(warn, "on_ctrl ret[%ld].", ret);
        }
    }
    return 0;
}

long rtsp__on_teardown_reply(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_teardown_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");

    session->machine_state  = ersms_state_init;
    session->close_reason   = rtsp_session_close_reason_by_teardown_reply;
    return 0;
    //return rtsp__close_session(session);
}

long rtsp__on_unauthorized(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_unauthorized(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char response[128];

    //get realm  nonce
    rtsp__get_http_auth_args(session, session->hmsg->headers.start_line.text.data);
    session->http_auth.enable = 1;
    //strcpy(session->http_auth.username, "admin");
    //strcpy(session->http_auth.password, "12345678a");
    rtsp__req_describe (session);
    //rtsp__req_options(session);
    return 0;
}

long rtsp__on_response(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_response(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long ret;
    static struct { long last_cmd; rtsp__on_rsp on_reply; } cmd_list[] = 
    {       
        {erss_status_options,   rtsp__on_options_reply},
        {erss_status_describe,  rtsp__on_describe_reply},
        {erss_status_announce,  rtsp__on_announce_reply},
        {erss_status_setup,     rtsp__on_setup_reply},
        {erss_status_play,      rtsp__on_play_reply},
        {erss_status_record,    rtsp__on_record_reply},
        {erss_status_teardown,  rtsp__on_teardown_reply}
    };
    unsigned long   i = 0;

    print_log0(debug, ".");

    for ( ; i < 7; i++)
    {
        if ( session->status == cmd_list[i].last_cmd)
        {
            ret = cmd_list[i].on_reply(session);
            if(ret)
            {
                print_log0(err, "faield when on_reply().");
                return ret;
            }
            return 0;
        }
    }
    
    print_log0(debug, "unkonwn reponse.");
    return -1;
}

long rtsp__req_options( struct rtsp_session *session )
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_options(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[512];
    unsigned long len  = 0;
    char authorition[512];

    print_log0(debug, ".");

    session->cseq++;
    rtsp_add_authorition(session, authorition, "OPTIONS");
    len = sprintf( response, "OPTIONS %s %s"RTSP_EL
                             "CSeq: %ld"RTSP_EL
                             "%s"
                             RTSP_EL,
                             session->url.data ? session->url.data : "*", RTSP_VERSION, session->cseq,
                             session->http_auth.enable? authorition : "");

    session->status = erss_status_options;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);
}

long rtsp__req_set_parameter( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_set_parameter(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[512];
    unsigned long len  = 0;
    char authorition[512];

    print_log0(debug, ".");

    session->cseq++;
    rtsp_add_authorition(session, authorition, "SET_PARAMETER");

    len = sprintf(response, "SET_PARAMETER %s %s"RTSP_EL
                       "CSeq: %ld"RTSP_EL
                       "%s"
                       RTSP_EL,
                       session->url.data ? session->url.data : "*", RTSP_VERSION,
                       session->cseq, session->http_auth.enable? authorition : "");

    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);
}

long rtsp__req_describe(struct rtsp_session *session )
{             
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_describe(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long            len, ret, authorization_len = 0;
    unsigned long   user_pass_len   = session->authorzation.user.len + 1 + session->authorzation.password.len,
                    user_pass_size  = user_pass_len + 1,
                    authorization_size  = (user_pass_len * 4 / 3) + 4,
                    response_size   = 768 + session->url.len,
                    total_size      = response_size + user_pass_size + authorization_size;
    char            *buf            = (char*)malloc(total_size),
                    *user_pass      = buf + response_size,
                    *authorization  = user_pass + user_pass_size;
    char authorition[512];
    if(NULL == buf)
    {
        print_log1(err, "failed when malloc(%ld) failed.", total_size);
        return -1;
    }
    print_log0(debug, ".");

    user_pass[0]     = 0;
    authorization[0] = 0;
    if(session->authorzation.user.len || session->authorzation.password.len)
    {
        memcpy(user_pass, session->authorzation.user.data, session->authorzation.user.len);
        user_pass[session->authorzation.user.len] = ':';
        memcpy(&user_pass[session->authorzation.user.len+1],
               session->authorzation.password.data,
               session->authorzation.password.len);
        user_pass[session->authorzation.user.len + 1 + session->authorzation.password.len] = 0;

        authorization_len = base64_encode((unsigned char*)user_pass, user_pass_len,
                                          (unsigned char*)authorization, authorization_size);
        if((0 >= authorization_len) || (authorization_len > (long)authorization_size))
        {
            free(buf);
            print_log0(err, "encode authorization failed.");
            return -1;
        }
    }

    session->cseq++;

    authorization_len = 0;//chenyong debug
    rtsp_add_authorition(session, authorition, "DESCRIBE");
    len = sprintf(buf, "DESCRIBE %s %s"RTSP_EL
                       "CSeq: %ld"RTSP_EL
                       "Accept: application/sdp"RTSP_EL
                       "%s%s%s"
                       "%s"
                       RTSP_EL,
                       session->url.data, RTSP_VERSION, session->cseq,
                       (authorization_len?"Authorization: Basic ":""), (authorization_len?authorization:""), (authorization_len?RTSP_EL:""),
                       session->http_auth.enable ? authorition : "");

    session->status = erss_status_describe;
    ret = rtsp__add_rtsp_package(session, (unsigned char*)buf, len );
    free(buf);
    return ret;
}

long rtsp__req_announce (struct rtsp_session *session)
{                 
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_announce(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[7000];    
    unsigned long len;
    
    print_log0(debug, ".");

    session->status = erss_status_announce;
    if(!(session->refer && session->sdp.data))
    {
        print_log0(info, "still have not valid session.refer and sdp waiting it.");
        return 0;
    }
    if(NULL == session->sdp.desc) 
        session->sdp.desc = sdp_create(session->sdp.data,session->sdp.len);
    if(rtsp__on_sdp(session)) 
    {
        print_log0(err, "failed when rtsp__on_sdp().");
        return -1;
    }
    session->have_send_sdp = 1;
    session->cseq++;        
    len = sprintf(response, "ANNOUNCE %s %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            "Content-Type: %s"RTSP_EL
                            "Content-Length: %ld"RTSP_EL
                            RTSP_EL"%s", 
                            session->url.data, RTSP_VERSION, session->cseq, "application/sdp",
                            session->sdp.len, session->sdp.data);
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);   
}

long rtsp__req_setup(struct rtsp_session *session, struct rtp_channel *channel)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_setup(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char                    response[768], transport[300], *mode_str = NULL;
    unsigned long           len, status_flag = (erss_status_setup == session->status);                            
    struct rtsp_fd_desc     rtp, rtcp;    
    struct rtsp_module      *mod = rtsp_session_get_mod(session);
    char authorition[512];

    print_log0(debug, ".");

    if(erst_client_play == session->session_type)
    {   
        if(ertt_rtp_avp_tcp == channel->transport_type)
        {/* transport by tcp */
            unsigned long maxnum;
            maxnum = ( channel == session->rtp_sessions.list)?0:channel->in_list.prev->transport.tcp.interleaved_id.rtcp + 1;             
            channel->transport.tcp.interleaved_id.rtp = maxnum;
            channel->transport.tcp.interleaved_id.rtcp = maxnum + 1;
        }
        else
        {
            if(rtsp__rtp_get_port_pair(rtsp_session_get_mod(session), &rtp, &rtcp ))
            {
                print_log0(err, "failed when rtsp_rtp_get_port_pair().");
                rtsp__send_err_reply(session, 500, NULL);
                return -1;
            }   
        
            channel->transport.udp.local_port.rtp = rtp.port;
            channel->transport.udp.local_port.rtcp = rtcp.port;         
        
            if ( (channel->transport.udp.rtp_sock = (struct rtsp_sock *) calloc(1, sizeof(struct rtsp_sock)))
                && (channel->transport.udp.rtcp_sock = (struct rtsp_sock *) calloc(1, sizeof(struct rtsp_sock))))
            {
                channel->transport.udp.rtp_sock->fd = rtp.fd;    
                channel->transport.udp.rtcp_sock->fd = rtcp.fd;                  
            }
        }
        mode_str = "play";  
    }
    else
    {
        channel->ssrc = (unsigned long) rand();
        channel->start_seq = (unsigned long) rand();  
        channel->seq = channel->start_seq;
        channel->start_rtptime = (unsigned long) rand();    
        channel->rtptime = channel->start_rtptime;
        
        if(ertt_rtp_avp_tcp == channel->transport_type)
        {/* transport by tcp */
            unsigned long maxnum;
            maxnum = ( channel == session->rtp_sessions.list)?0:channel->in_list.prev->transport.tcp.interleaved_id.rtcp + 1;             
            channel->transport.tcp.interleaved_id.rtp = maxnum;
            channel->transport.tcp.interleaved_id.rtcp = maxnum + 1;
        }
        else
        {
            rtp.port = mod->rtp_server.port;
            rtcp.port = mod->rtcp_server.port;
        }
        mode_str = "record";
    }   

    if(ertt_rtp_avp_tcp == channel->transport_type)
    {
        sprintf(transport, "Transport: RTP/AVP/TCP;interleaved=%ld-%ld;unicast;mode=%s;",
                channel->transport.tcp.interleaved_id.rtp, channel->transport.tcp.interleaved_id.rtcp, mode_str); 
    }
    else
    {
        sprintf(transport, "Transport: RTP/AVP;unicast;client_port=%ld-%ld;mode=%s,RTP/AVP/TCP;unicast;mode=%s;",
                rtp.port, rtcp.port, mode_str, mode_str);
    }
    session->cseq++;
    rtsp_add_authorition(session, authorition, "SETUP");
    //status_flag = 0; //chenyong add

    len = sprintf(response, "SETUP %s %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            "%s"RTSP_EL
                            "%s%s%s"
                            "%s"
                            RTSP_EL,
                            channel->media_url, RTSP_VERSION, session->cseq,
                            transport,
                            status_flag?"Session: ":"", status_flag?session->session_id:"", status_flag?RTSP_EL:"",
                            session->http_auth.enable ? authorition : "");

    session->status = erss_status_setup;
    session->sdp.stream_counts++;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len );
}

long rtsp__req_play( struct rtsp_session *session )
{       
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_play(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char       response[640];
    unsigned   len;
    char authorition[512];

    print_log0(debug, ".");

    if (ersms_state_ready > session->machine_state)
    {
        print_log0(err, "failed when check session state, because it is neither ready nor playing state.");
        return -1;
    }

    session->cseq++;
    rtsp_add_authorition(session, authorition, "PLAY");
    len = sprintf( response, "PLAY %s %s"RTSP_EL
                             "CSeq: %ld"RTSP_EL
                             "Session: %s"RTSP_EL
                             "Range: 0.0-"RTSP_EL
                             "%s"
                             RTSP_EL,
                             session->url.data, RTSP_VERSION, session->cseq, session->session_id,
                             session->http_auth.enable ? authorition : "");
    session->status = erss_status_play;
    if (session->video_channel) { rtsp__add_channel_sock(session->in_list.mod, session->video_channel); };
    if (session->audio_channel) { rtsp__add_channel_sock(session->in_list.mod, session->audio_channel); };
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len );
}

long rtsp__req_record( struct rtsp_session *session )
{   
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_record(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char        response[400];
    unsigned    len;  
     
    print_log0(debug, ".");

    if (ersms_state_ready > session->machine_state)
    {
        print_log0(err, "failed when check session state, because it is neither ready nor recording state.");
        return -1;
    }

    session->cseq++;  
    len = sprintf(response, "RECORD %s %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL
                            "Range: 0.0-"RTSP_EL
                            RTSP_EL,
                            session->url.data, RTSP_VERSION, session->cseq, session->session_id);
    session->status = erss_status_record;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);
}

long rtsp_req_teardown( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_req_teardown(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[640];
    unsigned long len = 0;
    char authorition[512];

    print_log0(debug, ".");

    session->cseq++;
    rtsp_add_authorition(session, authorition, "TERRDOWN");
    len = sprintf(response, "TERRDOWN %s %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL
                            "%s"
                            RTSP_EL,
                            session->url.data, RTSP_VERSION, session->cseq, session->session_id,
                            session->http_auth.enable ? authorition : "");
    session->status = erss_status_teardown;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);
}

long rtsp__req_ctrl (struct rtsp_session *session, struct len_str *method, struct len_str *params)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__req_ctrl(session["rtsp_session_format_s"], method["len_str_format_s"], params["len_str_format_s"])"
#define func_format()   rtsp_session_format(session), len_str_format(method), len_str_format(params)

    char            request[200];
    unsigned long   len = 0;

    print_log0(debug, ".");

    if((NULL == session) || (NULL == method) || (0 == method->len))
    {
        print_log0(err, "invalid param.");
        return 0;
    }
    
    if(0 == session->methods.counts)
    {
        if(session->methods.query_flag)
        {
            print_log0(warn, "remote support methods, because the option reply do not arrive.");
            return 0;
        }
        else
        {
            if((session->status != erss_status_options) && rtsp__req_options(session))
            {        
                print_log0(err, "failed when invoke rtsp_req_options.");
                return -1;
            }
            session->methods.query_flag = 1;
        }
    }
    else
    {
        struct len_str rtsp__s_ctrl = {len_str_def_const("CTRL")};
        if (rtsp__check_support_method(session, &rtsp__s_ctrl))
        {
            print_log0(warn, "remote do not support ctrl.");
            return 0;
        }
    }

    if (erst_client_play == session->session_type )
    {
        if (ersms_state_playing != session->machine_state)
        {                        
            print_log0(err, "the session is not playing state.");
            return 0;
        }
    }                                              
    else if (erst_server_record == session->session_type)
    {
        if (ersms_state_recording != session->machine_state)
        {
            print_log0(err, "the session is not recording state.");
            return 0;
        }
    }
    else
    {
        print_log0(warn, " neither client play nor server record.");
        return 0;
    }

    session->cseq++;
    len = sprintf(request,  "CTRL %s %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL
                            "Method: %*.*s"RTSP_EL
                            "Params: %*.*s"RTSP_EL
                            RTSP_EL,
                            session->url.data, RTSP_VERSION, session->cseq, session->session_id,
                            0, (int)(method->len), method->data,
                            0, (int)(params?params->len:0), params?params->data:NULL);
    return rtsp__add_rtsp_package(session, (unsigned char*)request, len);
}

/* for server */
#define DoNothing()   
char *rtsp__get_status(int err)
{
    static struct {
        char *token;
        long code;
    } status[] = {
        { "Continue", 100}, 
        { "OK", 200}, 
        { "Created", 201}, 
        { "Accepted", 202},
        { "Non-Authoritative Information", 203}, 
        { "No Content", 204}, 
        { "Reset Content", 205}, 
        { "Partial Content", 206},
        { "Multiple Choices", 300}, 
        { "Moved Permanently", 301}, 
        { "Moved Temporarily", 302},
        { "Bad Request", 400}, 
        { "Unauthorized", 401}, 
        { "Payment Required", 402}, 
        { "Forbidden", 403}, 
        { "Not Found", 404}, 
        { "Method Not Allowed", 405}, 
        { "Not Acceptable", 406}, 
        { "Proxy Authentication Required", 407}, 
        { "Request Time-out", 408}, 
        { "Conflict", 409}, 
        { "Gone", 410}, 
        { "Length Required", 411}, 
        { "Precondition Failed", 412}, 
        { "Request Entity Too Large", 413}, 
        { "Request-URI Too Large", 414 }, 
        { "Unsupported Media Type", 415}, 
        { "Bad Extension", 420}, 
        { "Invalid Parameter", 450}, 
        { "Parameter Not Understood", 451}, 
        { "Conference Not Found", 452},
        { "Not Enough Bandwidth", 453}, 
        { "Session Not Found", 454}, 
        { "Method Not Valid In This State", 455},
        { "Header Field Not Valid for Resource", 456}, 
        { "Invalid Range", 457}, 
        { "Parameter Is Read-Only", 458}, 
        { "Unsupported transport", 461}, 
        { "Internal Server Error", 500}, 
        { "Not Implemented", 501}, 
        { "Bad Gateway", 502}, 
        { "Service Unavailable", 503}, 
        { "Gateway Time-out", 504}, 
        { "RTSP Version Not Supported", 505},
        { "Option not supported", 551}, 
        { "Extended Error:", 911},
        { NULL, -1}
    };
    
    long i;
    for (i = 0; status[i].code != err && status[i].code != -1; ++i)
    {
        DoNothing();
    }    
    return status[i].token;
}

long rtsp__send_err_reply(struct rtsp_session *session, long err_code, char *desc_add)
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_err_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[200];
    unsigned long len = 0;   

    print_log0(debug, ".");

    len = sprintf(response, "%s %ld %s"RTSP_EL
                            "CSeq: %ld"RTSP_EL
                            RTSP_EL"%s",
                            RTSP_VERSION, err_code, rtsp__get_status(err_code), session->cseq,
                            (NULL != desc_add)?desc_add:"" );
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len );
}

long rtsp__on_request(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_request(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    static struct  msg_item {
        struct {unsigned len;  char *data;} msg;
        rtsp__on_req                         on_msg;
    } msg_list[] = {
        { {len_str_def_const("OPTIONS")},        rtsp__on_options},
		{ {len_str_def_const("SET_PARAMETER")},  rtsp__on_set_parameter}, // add by chenyong 2018-8-10
        { {len_str_def_const("DESCRIBE")},       rtsp__on_describe},
        { {len_str_def_const("ANNOUNCE")},       rtsp__on_announce},
        { {len_str_def_const("SETUP")},          rtsp__on_setup},
        { {len_str_def_const("PLAY")},           rtsp__on_play},
        { {len_str_def_const("RECORD")},         rtsp__on_record},
        { {len_str_def_const("PAUSE")},          rtsp__on_pause},
        { {len_str_def_const("TEARDOWN")},       rtsp__on_teardown},
        { {len_str_def_const("CTRL")},           rtsp__on_ctrl},
        { {len_str_def_const("GET")},            rtsp__on_get},
        { {len_str_def_const("POST")},           rtsp__on_post}
    };
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str      *method = &hmsg->headers.start_line.desc.req.method;
    unsigned long       i = 0;

    print_log0(debug, ".");

    for ( i = 0; i < sizeof(msg_list)/sizeof(struct msg_item); i++)
    {
        if (0 == len_str_cmp(method, &msg_list[i].msg))
        {
			print_log1(warn, "method : %s", msg_list[i].msg.data);
            return msg_list[i].on_msg(session);
        }
    }

    rtsp__send_err_reply(session, 405, NULL);
    print_log1(err, "unknown req["len_str_format_s"].", len_str_format(method));
    return ERR_GENERIC;
}

long rtsp__on_ctrl(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_ctrl(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long                    ret;
    struct rtsp_module      *mod = rtsp_session_get_mod(session);
    struct http_msg      *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str          *method = http_msg_get_known_header(hmsg, ehmht_method),
                            *params = http_msg_get_known_header(hmsg, ehmht_params);
    struct rtsp_req_params  req_params = {0};

    print_log0(debug, ".");

    if(NULL == method)
    {
        print_log0(err, "missing method.");
        rtsp__send_err_reply(session, 400, NULL);
        return -1;
    }

    req_params.method = *method;
    if(params){ req_params.params = *params; };

    if(erst_client_record == session->session_type)
    {
        if(ersms_state_recording != session->machine_state)
        {
            print_log0(warn, "client record session meet something wrong when check session machine state.");
            return 0;
        }

        if(NULL == session->on_ctrl)
        {
            print_log0(err, "missing on_ctrl callback.");
            return -1;
        }
        else
        {
            ++session->callback_times;
            mlock_simple_release(mod->lock);
            ret = session->on_ctrl(session, &req_params);
            mlock_simple_wait(mod->lock);
            --session->callback_times;
            if(ret)
            {
                print_log0(err, "client record session invoke on_idr failed.");
                return -1;
            }
        } 
    }
    else if(erst_server_play == session->session_type)
    {
        if(ersms_state_playing != session->machine_state)
        {
            print_log0(warn, "server play session meet something wrong when check session machine state.");
            return 0;
        }

        if(NULL == session->on_ctrl)
        {
            print_log0(err, "missing on_ctrl callback.");
            return -1;
        }
        else
        {
            ++session->callback_times;
            mlock_simple_release(mod->lock);
            ret = session->on_ctrl(session, &req_params);
            mlock_simple_wait(mod->lock);
            --session->callback_times;
            if(ret)
            {
                print_log0(err, "client server play  invoke on_ctrl failed.");
                return -1;
            }
        } 
    }
    else 
    {
        print_log0(warn, "failed because CTRL method is not supported for this session.");
        return -1;
    }
    return 0;
}

long rtsp__on_set_parameter( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_set_parameter(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)
	print_log0(debug, ".");

	return rtsp__send_set_parameter_reply(session);

}

long rtsp__send_set_parameter_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_set_parameter_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)
	char           response[300];
    unsigned long  len = 0;

    print_log0(debug, ".");


    len = sprintf(response, "%s %d %s"RTSP_EL
                            "Cseq: %ld"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Session: %s"RTSP_EL
                            RTSP_EL,
                            RTSP_VERSION, 200, rtsp__get_status(200), session->cseq,
                            RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION, session->session_id);

	return rtsp__add_rtsp_package( session, (unsigned char*)response, len );

}
long rtsp__on_get(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_get(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");

    return rtsp__send_get_reply(session);    
}

long rtsp__send_get_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_get_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char           response[300];
    unsigned long  len = 0;   

    print_log0(debug, ".");

    len = sprintf(response, "%s %d %s"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Connection: close"RTSP_EL
                            "Date: %s"RTSP_EL
                            "Cache-Control: no-store"RTSP_EL
                            "Pragma: no-cache"RTSP_EL
                            "Content-Type: application/x-rtsp-tunnelled"RTSP_EL
                            RTSP_EL,
                            "HTTP/1.0", 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION,
                            rtsp__add_time_stamp());    

    return rtsp__add_rtsp_package( session, (unsigned char*)response, len );
}

long rtsp__on_post(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_post(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");
    /* \todo: xxxxxxxxxxxxx, now just allowed pass POST for apple rtsp-tunnelling, but infact apple said,
        don't send back ack for post, so ignore on prev-call-chain pos. so this line allways will not happen */
    return rtsp__send_post_reply(session);    
}

long rtsp__send_post_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_post_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char           response[300];
    unsigned long  len = 0;   
    
    print_log0(debug, ".");

    len = sprintf(response, "%s %d %s"RTSP_EL
        "Server: %s/%s"RTSP_EL
        "Content-Type: application/x-rtsp-tunnelled"RTSP_EL
        RTSP_EL,
        "HTTP/1.0", 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION);    

    return rtsp__add_rtsp_package( session, (unsigned char*)response, len );
}


long rtsp__on_options( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_options(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct netx_event   evt = {0};
    struct len_str     *p_cseq;   
    int                 cseq_num = 0;
    
    print_log0(debug, ".");

    if ( (NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq)))
        || (1 != sscanf(p_cseq->data, "%d", &cseq_num)) )
    {        
        print_log0(err, "missing request num(CReq).");
        return rtsp__send_err_reply(session, 400, NULL);
    }
    session->cseq = (unsigned long)cseq_num;
    return rtsp__send_options_reply(session);    
}

long rtsp__send_options_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_options_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char           response[300];
    unsigned long  len = 0;   
    char           *support_method;

    print_log0(debug, ".");

    if (erst_server_play == session->session_type || erst_server_record == session->session_type)
    {
        support_method = "DESCRIBE, SETUP, TEARDOWN, PLAY, OPTIONS, ANNOUNCE, RECORD, CTRL";
    }
    else if (erst_client_play == session->session_type || erst_client_record == session->session_type)
    {
        support_method = "OPTIONS, CTRL";
    }

    len = sprintf(response, "%s %d %s"RTSP_EL
                            "Cseq: %ld"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Public: %s"RTSP_EL
                            RTSP_EL,
                            RTSP_VERSION, 200, rtsp__get_status(200), session->cseq,
                            RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION, support_method);    
    return rtsp__add_rtsp_package( session, (unsigned char*)response, len );
}

long rtsp__on_describe( struct rtsp_session *session )
{  
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_describe(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module  *mod = session->in_list.mod;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str     *p_cseq, *full_uri, *p_accept;   
    struct rtsp_req_params req_params;
    int                 int_v = 0;
    
    print_log0(debug, ".");
    if(session->refer && session->sdp.data && session->sdp.len)
    { /* channel created, sdp information prepared */
        if (NULL == session->sdp.desc){
            session->sdp.desc = sdp_create(session->sdp.data, session->sdp.len);
        }

        session->have_send_sdp = 1;
        if(rtsp__on_sdp(session))
        {
            print_log0(err, "failed when rtsp__on_sdp().");
            rtsp__send_err_reply(session, 500, NULL);
        }
        else
        {
            rtsp__send_describe_reply(session);
        }
    }        
    session->status = erss_status_describe;
    /* parse the url */
    if ( hmsg && hmsg->headers.finished 
        && (full_uri = http_msg_get_full_uri(hmsg)) 
        && (0 == url_parse(full_uri->data, full_uri->len, &session->url.desc)) )
    {
        if(NULL == (session->url.data = (char*)malloc(full_uri->len + 1 )))
        {
            print_log0(err, "failed when save url.");
            rtsp__send_err_reply(session, 500, NULL);
            return -1;
        }
        memcpy(session->url.data, full_uri->data, full_uri->len);
        session->url.data[full_uri->len] = 0;       
        session->url.len = full_uri->len;
    }
    else
    {        
        print_log0(err, "missing url.");
        return rtsp__send_err_reply(session, 500, NULL );      
    }
    
    /* check cseq field */
    if ( (NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq)))
        || (1 != sscanf(p_cseq->data, "%d", &int_v)) )
    {       
        print_log0(err, "missing valid CSeq.");
        return  rtsp__send_err_reply(session, 400, NULL);            /* bad request */
    }
    session->cseq = (unsigned long)int_v;
   
    /* check accept field */ 
    if ( (NULL == (p_accept = http_msg_get_known_header(hmsg, ehmht_accept))) 
        ||  (0 != len_str_cmp_const(p_accept, "application/sdp") ))
    {        
        print_log0(err, "missing accept: application/sdp.");
        return rtsp__send_err_reply(session, 551, NULL);
    }

    session->session_type = erst_server_play;       /* is play request */   
    session->on_req = mod->params.events.server_play_req;
    session->on_close = mod->params.events.server_play_close;
    session->on_ctrl = mod->params.events.server_play_ctrl;
    session->on_media_data = NULL;   

    req_params.full_url.data = session->url.desc.path.data;
    req_params.full_url.len = session->url.desc.file_name.len + session->url.desc.path.len;  
    if(NULL == session->on_req)
    {
        print_log0(err, "missing on_req callback.");
        rtsp__send_err_reply(session, 500, NULL);
        return -1;
    }
    else
    {
        long ret;

        ++session->callback_times;
        mlock_simple_release(mod->lock);
        ret = session->on_req(session, &req_params);
        mlock_simple_wait(mod->lock);
        --session->callback_times;

        if(ret)
        {
            print_log0(err, "failed when invoke on_req() callback.");
            rtsp__send_err_reply(session, 500, NULL);
            return -1;        
        }
    }    
    return ERR_NO_ERROR;
}

long rtsp__send_describe_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_describe_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long            ret;
    unsigned long   len = 0, size;
    char            *response;

 char *p_test = "v=0\r\n\
o=shenzhenmining\r\n\
s=sample\r\n\
m=video 0 RTP/AVP 96\r\n\
a=rtpmap:96 H264/90000\r\n\
a=framerate:1\r\n\
a=fmtp:96 profile-level-id=4D0028; packetization-mode=1; sprop-parameter-sets=Z00AKPQCgC3I,aO48gA==\r\n\
a=control:trackID=1\r\n\r\n";


#if 0 //chenyong
	char * test_sdp= "v=0\r\n\
o=shenzhenmining\r\n\
s=sample\r\n\
m=video 0 RTP/AVP 96\r\n\
a=rtpmap:96 H264/90000\r\n\
a=framerate:60\r\n\
a=fmtp:96 profile-level-id=4D000C; packetization-mode=1; sprop-parameter-sets=Z00AKPQCgC3I,aO48gA==\r\n\
a=control:trackID=1\r\n\
m=audio 0 RTP/AVP 97\r\n\
a=rtpmap:97 mpeg4-generic/16000/1\r\n\
a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;constantDuration=1024;config=1408\r\n\
a=control:trackID=2\r\n\r\n";

	if (session->sdp.data)
	{
		free(session->sdp.data);
		session->sdp.data = (char *)malloc(strlen(test_sdp)+1);
	}

	strcpy(session->sdp.data, test_sdp);
	session->sdp.len = strlen(test_sdp);

	//strcpy(session->url.data, "rtsp://192.168.42.118:554/live/");
	//session->url.len = strlen("rtsp://192.168.42.118:554/live/");

#endif

#define TB_DEBUG 0
#if TB_DEBUG
	char *tempSdp = 
		"v=0"RTSP_EL
		"o=shenzhenmining"RTSP_EL
		"s=sample"RTSP_EL
		"m=video 0 RTP/AVP/TCP 96"RTSP_EL
		"a=rtpmap:96 H264/90000"RTSP_EL
		"a=framerate:10"RTSP_EL
		"a=fmtp:96 profile-level-id=4D000C; packetization-mode=1; sprop-parameter-sets=Z00ADI2NQKDPzwgAADhAAAr8gCA=,aO44gA=="RTSP_EL
		"a=control:trackID=1"RTSP_EL
		"m=audio 0 RTP/AVP/TCP 97"RTSP_EL
		"a=rtpmap:97 mpeg4-generic/16000/1"RTSP_EL
		"a=fmtp:97 profile-level-id=15;mode=AAC-lbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1408"RTSP_EL
		"a=control:trackID=2"RTSP_EL;

	printf("\n*****%s:%d*****\n",__FILE__,__LINE__);
	printf("%s",session->sdp.data);
	printf("\n************************\n");
	printf("%s",tempSdp);
	printf("\n************************\n");
#endif
	size = session->sdp.len + 1024;
	response = malloc(size);
    print_log0(debug, ".");
    if(NULL == response)
    {
        print_log0(err, "failed when malloc() respone message.");
        return -1;
    }

    len = sprintf( response, "%s %d %s"RTSP_EL
                             "Server: %s/%s"RTSP_EL
                             "Cseq: %ld"RTSP_EL
                             "Date: %s"RTSP_EL
                             "Content-Length: %ld"RTSP_EL
                             "Content-Type: application/sdp"RTSP_EL
                             /*
                             "x-Accept-Retransmit: our-retransmit"RTSP_EL
                             "x-Accept-Dynamic-Rate: 1"RTSP_EL
                             */
                             "Content-Base: %s/"RTSP_EL
                             RTSP_EL
                             "%s",
                             RTSP_VERSION, 
                             200, 
                             rtsp__get_status(200), 
                             RTSP_PACKAGE_NAME, 
                             RTSP_PACKAGE_VERSION, 
                             session->cseq,
                             rtsp__add_time_stamp(),
#if TB_DEBUG
							 strlen(tempSdp),
							 session->url.data,
							 tempSdp);
#else
                             session->sdp.len,
                             session->url.data,
                             session->sdp.data);
#endif

	//print_log1(err,"%s", session->sdp.data);//chenyong debug

    session->status = erss_status_describe;
    ret = rtsp__add_rtsp_package(session, (unsigned char*)response, len );
    free(response);
    return ret;
}

long rtsp__on_announce(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_announce(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module  *mod = session->in_list.mod;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    unsigned long       content_len = hmsg?http_msg_get_content_len(hmsg):0;
    struct len_str     *p_cseq, *full_uri, *p_content_type; 
    struct rtsp_req_params req_params;  
    
    print_log0(debug, ".");

    /* parse the url */
    if ( hmsg && (full_uri = http_msg_get_full_uri(hmsg))
        && (0 == url_parse(full_uri->data, full_uri->len, &session->url.desc)))
    {       
        if (session->url.data) free(session->url.data);
        if ( (NULL == (session->url.data = malloc(full_uri->len + 1 ))))
        {
            print_log0(err, "failed when save url.");
            return rtsp__send_err_reply(session, 500, NULL);    
        }
        memcpy(session->url.data, full_uri->data, full_uri->len);
        session->url.data[full_uri->len] = 0;      
        session->url.len = full_uri->len;
    }
    else
    {
        print_log0(err, "missing url.");
        return rtsp__send_err_reply(session, 500, NULL);          
    }
    
    /* check cseq field */
    if ( (NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq)))
        || (1 != sscanf(p_cseq->data, "%ld", &session->cseq)) )
    {
        print_log0(err, "missing valid Cseq.");
        return rtsp__send_err_reply(session, 400, NULL);            /* bad request */
    }
   
    /* check content type field */ 
    if ( (NULL == (p_content_type = http_msg_get_known_header(hmsg, ehmht_content_type))) 
        ||  (0 != len_str_cmp_const(p_content_type, "application/sdp")) )
    {
        print_log0(err, "just support content-type: application/sdp.");
        return rtsp__send_err_reply(session, 551, NULL);
    }
    
    /* check sdp information */
    if(0 >= (long)content_len)
    { /* lack of sdp information */
        print_log0(err, "missing content.");
        return rtsp__send_err_reply(session, 400, NULL);
    }
    else
    {
        session->sdp.len = content_len;
        if (session->sdp.data) { free(session->sdp.data);  sdp_destroy(session->sdp.desc);};
        if (((NULL == (session->sdp.data = (char*)malloc(content_len+ 1))))
            ||(content_len != http_msg_get_content(hmsg, content_len + 1, session->sdp.data)))
        {
            print_log0(err, "faile when save sdp.");
            return rtsp__send_err_reply(session, 500, NULL);    
        }
        session->sdp.data[content_len] = 0;       
        if (NULL == (session->sdp.desc = sdp_create(session->sdp.data, content_len)))
        {
            print_log0(err, "failed when sdp_create().");
            return rtsp__send_err_reply(session, 500, NULL); 
        }
    }
    
    if(rtsp__on_sdp(session))
    {
        print_log0(err, "failed when rtsp_do_sdp().");
        return rtsp__send_err_reply(session, 500, NULL);
    }  
    
    session->session_type = erst_server_record;       /* is record request */   
    session->on_req = mod->params.events.server_record_req;
    session->on_close = mod->params.events.server_record_close;
    session->on_media_data = mod->params.events.server_record_data;   
    
    req_params.full_url.data = session->url.desc.path.data;
    req_params.full_url.len = session->url.desc.file_name.len + session->url.desc.path.len;  
    if(NULL == session->on_req)
    {
        print_log0(err, "missing on_req callback.");
        rtsp__send_err_reply(session, 500, NULL);
        return -1;
    }
    else
    {
        long ret;

        ++session->callback_times;
        mlock_simple_release(mod->lock);
        ret = session->on_req(session, &req_params);
        mlock_simple_wait(mod->lock);
        --session->callback_times;

        if(ret)
        {
            print_log0(err, "failed when invoke on_req() callback.");
            rtsp__send_err_reply(session, 500, NULL);
            return -1;        
        }
    }    

    return rtsp__send_announce_reply(session);
}

long rtsp__send_announce_reply(struct rtsp_session *session)
{ 
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_announce_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char            response[200] ={0};    
    unsigned long   len;

    print_log0(debug, ".");

    len = sprintf(response, "%s %d %s"RTSP_EL
                  "Server: %s/%s"RTSP_EL
                  "CSeq: %ld"RTSP_EL                                            
                  RTSP_EL,
                  RTSP_VERSION, 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION, 
                  session->cseq);
    session->status = erss_status_announce;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);      
}

long rtsp__on_setup( struct rtsp_session *session )
{   
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_setup(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_transport_header transport = {0};
    struct rtsp_module  *mod = session->in_list.mod;
    struct rtp_channel  *channel = NULL;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str      *p_cseq, *p_transport, *p_session, *full_uri = NULL;
    struct url_scheme   url;    
    struct sdp_media    *media;
    struct sdp_xfield   *control_id = NULL;
    int                 int_v = 0;

    print_log0(debug, ".");

    /* check have deal with the describe or announce */
    if((NULL == session->url.data) || (NULL == session->sdp.data))
    {
        print_log1(err, "session missing %s.", session->url.data?"sdp":"url");
        return rtsp__send_err_reply(session, 400, NULL);
    }
    
    /* check url */
    if ( (NULL == hmsg)
         ||(NULL == (full_uri = http_msg_get_full_uri(hmsg)))
         || url_parse(full_uri->data, full_uri->len, &url))
    {
        print_log1(err, "invalid set url["len_str_format_s"]", len_str_format(full_uri));
        return rtsp__send_err_reply(session, 400, NULL);       
    } 

    /* check cseq field */
    if ( (NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq)))
        || (1 != sscanf(p_cseq->data, "%d", &int_v)))
    {
        print_log0(err, "missing valid Cseq.");
        return rtsp__send_err_reply(session, 400, NULL );       
    }
    session->cseq = (unsigned long)int_v;
    
    /* check transport field */ 
    if ( (NULL == (p_transport = http_msg_get_known_header(hmsg, ehmht_transport)))
        || ( 0 != rtsp__parse_transport_field(p_transport, &transport))) 
    {
        print_log0(err, "missing valid tarnsport field.");
        return rtsp__send_err_reply(session, 400, NULL);          /* bad request */
    }    

    if (NULL == (media = session->sdp.desc->medias.list))
    {
        print_log0(err, "missing sdp media info.");
        return rtsp__send_err_reply(session, 400, NULL);          /* bad request */
        return -1;
    }    
    do 
    {
        control_id = sdp_get_media_attr(media, "control", sizeof("control") - 1);       
        
       if ( control_id 
           && (0 == len_str_cmp(&(url.file_name), &(control_id->value))))
       {
           if (0 == len_str_cmp_const(&media->media_layer.announce.media_descr, "video"))
           {
               channel = session->video_channel;              
           }
           else if (0 == len_str_cmp_const(&media->media_layer.announce.media_descr, "audio"))
           {
               channel = session->audio_channel;
           }                 
           break;
       }        
    } while ( (media = media->out_list.next) && (media != session->sdp.desc->medias.list));
   
    if ( NULL == channel ) 
    {
        print_log0(err, "have not matched rtp-channel.");
        return rtsp__send_err_reply(session, 400, NULL);          /* bad request */
        return -1;
    }
    
    memcpy(channel->media_url, full_uri->data, full_uri->len);
    channel->media_url[full_uri->len] = 0;  
       
    if (( p_session = http_msg_get_known_header(hmsg, ehmht_session) ))
    {   
        unsigned long session_len = (p_session->len >= sizeof(session->session_id))?(sizeof(session->session_id) - 1):p_session->len;
        memcpy(&(session->session_id[0]), p_session->data, session_len);   
        session->session_id[session_len] = 0;
    }
    else
    { /* no session header */
        unsigned long len = ((unsigned long) rand()) % 12 + 8;      
        rtsp__get_session_id(&session->session_id[0], len);
      //  sprintf(&session->session_id[0], "%s", rtsp_get_session_id(&len));
    }  

    channel->ssrc = (unsigned long) rand();
    channel->start_seq = (unsigned long) rand();  
    channel->seq = channel->start_seq;
    channel->start_rtptime = (unsigned long) rand();    
    channel->rtptime = channel->start_rtptime;
    
    channel->transport_type = transport.type;
    if ( ertt_rtp_avp_udp ==  transport.type )
    {  
        struct  sockaddr_in addr = session->remote_addr;         
       
        addr.sin_port = htons( (unsigned short) transport.client_port.rtp);
        channel->transport.udp.rtp_addr = addr;
        addr.sin_port = htons( (unsigned short) transport.client_port.rtcp);   
        channel->transport.udp.rtcp_addr = addr;     
    }     
    else if (ertt_rtp_avp_tcp == transport.type)
    {        
        channel->transport.tcp.interleaved_id = transport.channel_nld;       
    } 
    return rtsp__send_setup_reply(session, channel);
}

long rtsp__send_setup_reply( struct rtsp_session *session, struct rtp_channel *channel)
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_setup_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_fd_desc rtp, rtcp;
    struct rtsp_module *mod = rtsp_session_get_mod(session);
    char    response[1024], trans[200];
    long    len = 0;
    char    *mode_str = NULL;

    print_log0(debug, ".");

    switch(channel->transport_type)
    {
    case ertt_rtp_avp_udp :
        {
            if(erst_server_record == session->session_type)
            {
                if(rtsp__rtp_get_port_pair(mod, &rtp, &rtcp ))
                {
                    print_log0(err, "failed when rtsp_rtp_get_port_pair()");
                    rtsp__send_err_reply(session, 500, NULL);
                    return -1;
                }           
                if((channel->transport.udp.rtp_sock = (struct rtsp_sock *) calloc(1, sizeof(struct rtsp_sock)))
                    && (channel->transport.udp.rtcp_sock = (struct rtsp_sock *) calloc(1, sizeof(struct rtsp_sock))))
                {
                    channel->transport.udp.rtp_sock->fd = rtp.fd;    
                    channel->transport.udp.rtcp_sock->fd = rtcp.fd;                  
                }        
                channel->transport.udp.local_port.rtp = rtp.port;
                channel->transport.udp.local_port.rtcp = rtcp.port;
                mode_str = "record";
            }            
            else if(erst_server_play == session->session_type)
            {
                rtp.port = mod->rtp_server.port;
                rtcp.port = mod->rtcp_server.port;     
                mode_str = "play";
            }
            
            len += sprintf(trans + len, "RTP/AVP;UNICAST;mode=%s;source=%s;client_port=%d-%d;server_port=%ld-%ld",
                mode_str, inet_ntoa( session->in_list.mod->local_addr), 
                ntohs(channel->transport.udp.rtp_addr.sin_port), ntohs(channel->transport.udp.rtcp_addr.sin_port),               	     
                rtp.port, rtcp.port); 
            if (erst_server_play == session->session_type)
            {
                len += sprintf(trans + len, ";ssrc=%08lX", channel->ssrc);
            }
        }
        break;        
    case ertt_rtp_avp_tcp:
        {
            unsigned long maxnum;
            maxnum = ( channel == session->rtp_sessions.list)?0:channel->in_list.prev->transport.tcp.interleaved_id.rtcp + 1;             
            channel->transport.tcp.interleaved_id.rtp = maxnum;
            channel->transport.tcp.interleaved_id.rtcp = maxnum + 1;
            if ( erst_server_play == session->session_type)
            {  
                mode_str = "play";
            }
            else if ( erst_server_record == session->session_type ) 
            {
                mode_str = "record";
            }
            len += sprintf(trans + len, "RTP/AVP/TCP;UNICAST;mode=%s;interleaved=%ld-%ld", 
                mode_str, channel->transport.tcp.interleaved_id.rtp, channel->transport.tcp.interleaved_id.rtcp);
            if (erst_server_play == session->session_type)
            {
                len += sprintf(trans + len, ";ssrc=%08lX", channel->ssrc);
            }          
        }
        break;
    default:        
        break;        
    }       
    
    len = sprintf(response, "%s %d %s"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Cseq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL
                            "Date: %s"RTSP_EL
                            "Expires: %s"RTSP_EL
                            "Transport: %s"RTSP_EL
                            //"x-Dynamic-Rate: 1"RTSP_EL
                            RTSP_EL,
                            RTSP_VERSION, 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION, 
                            session->cseq, session->session_id, 
                            rtsp__add_time_stamp(), rtsp__add_time_stamp(), trans);      
    
    session->machine_state = ersms_state_ready;
    session->status = erss_status_setup;    
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);    
}

long rtsp__send_play_reply(struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_play_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtp_channel  *channel;
    struct rtsp_module  *mod = rtsp_session_get_mod(session);
    char                response[1024], rtp_info[200];
    unsigned long       len = 0; 
    
    print_log0(debug, ".");

    if( NULL == (channel = session->rtp_sessions.list))
    {
        rtsp__send_err_reply(session, 500, NULL);
        print_log0(err, "missing rtp-session.");
        return -1;
    }

    if (channel->transport_type == ertt_rtp_avp_tcp)
    {        
        long size = 0x100000;         
        setsockopt(session->sock->fd, SOL_SOCKET, SO_SNDBUF, (const char *)&size, sizeof(int));  
    }

    do
    {
        len += sprintf(rtp_info + len, "url=%s", channel->media_url/*, channel->start_seq*/);
        channel = channel->in_list.next;
        if ( channel != session->rtp_sessions.list )
            rtp_info[len++] = ',';           
        else
            break;
    } while(channel);
    
    len = sprintf( response, "%s %d %s"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Cseq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL
                            "Date: %s"RTSP_EL
                            "Expires: %s"RTSP_EL
                            "Range: npt=now-"RTSP_EL
                            "RTP-Info: %s"RTSP_EL
                            RTSP_EL,
                            RTSP_VERSION, 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION, session->cseq,
                            session->session_id,  rtsp__add_time_stamp(), 
                            rtsp__add_time_stamp(), 
                            rtp_info);
    
    session->rtp_sessions.rtp_started = 1;          
    session->machine_state = ersms_state_playing;

    if(NULL == session->on_ctrl)
    {
        print_log0(warn, "missing on ctrl callback.");
    }
    else
    {
        long                    ret;
        struct rtsp_req_params  req_params = {0};
        len_str_set_const(&req_params.method, "idr");

        ++session->callback_times;
        mlock_simple_release(mod->lock);
        ret = session->on_ctrl(session, &req_params);
        mlock_simple_wait(mod->lock);
        --session->callback_times;

        if(ret)
        {
            print_log1(warn, "on_ctrl ret[%ld].", ret);
        }
    }
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len); 
}

long rtsp__on_play( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_play_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module  *mod = session->in_list.mod;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str      *p_cseq, *p_session, *full_uri = NULL;
    int                 int_v = 0;
    
    print_log0(debug, ".");

    if ( ersms_state_ready > session->machine_state)
    {
        print_log0(err, "session state is not ready.");
        return rtsp__send_err_reply(session, 400, NULL);
    }
    session->status = erss_status_play;

    /* parse the url */
    if ( (NULL == hmsg) || (NULL == (full_uri = http_msg_get_full_uri(hmsg)))
        || (0 != url_parse(full_uri->data, full_uri->len, &session->url.desc)))
    { 
        print_log1(err, "invalid play url["len_str_format_s"].", len_str_format(full_uri));
        return rtsp__send_err_reply(session, 400, NULL );     
    }   
    
    /* check cseq field */
    if ( (NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq)))
        || (1 != sscanf(p_cseq->data, "%d", &int_v)))
    {       
        print_log0(err, "missing valid Cseq.");
        return rtsp__send_err_reply(session, 400, NULL);            /* bad request */
    }   
    session->cseq = (unsigned long)int_v;

    /* check session field */
    if ((p_session = http_msg_get_known_header(hmsg, ehmht_session)))
    {
        memcpy(&(session->session_id[0]), p_session->data, p_session->len);      
        session->session_id[p_session->len] = 0;
    }
    else
    {
        print_log0(err, "reqeust missing session field.");
        rtsp__send_err_reply(session, 454, NULL);  
        return ERR_NO_ERROR;     
    }                
    return rtsp__send_play_reply(session);
}

long rtsp__send_record_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_record_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module *mod = rtsp_session_get_mod(session);    
    struct rtp_channel *channel = NULL;
    char               response[1024], rtp_info[200];
    unsigned long      len = 0;  
    
    print_log0(debug, ".");

    if(NULL == (channel = session->rtp_sessions.list))
    {
        print_log0(err, "missing rtp-session.");
        rtsp__send_err_reply(session, 500, NULL);
        return -1;
    }
    do
    {
        len += sprintf(rtp_info + len, "url=%s", channel->media_url);
        channel = channel->in_list.next;
        if ( channel != session->rtp_sessions.list )
            rtp_info[len++] = ',';           
        else
            break;
    } while(channel);

    if ( session->video_channel ) { rtsp__add_channel_sock(session->in_list.mod, session->video_channel);};
    if ( session->audio_channel ) { rtsp__add_channel_sock(session->in_list.mod, session->audio_channel);};
    
    len = sprintf(response, "%s %d %s"RTSP_EL
                            "Server: %s/%s"RTSP_EL
                            "Cseq: %ld"RTSP_EL
                            "Session: %s"RTSP_EL  
                            "RTP-Info: %s"RTSP_EL
                            RTSP_EL,
                            RTSP_VERSION, 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION,
                            session->cseq, session->session_id, rtp_info);    
    session->rtp_sessions.rtp_started = 1;   
    session->machine_state = ersms_state_recording;

    return rtsp__add_rtsp_package(session, (unsigned char*)response, len ); 
}

long rtsp__on_record( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_record(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module  *mod = session->in_list.mod;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str      *p_cseq, *p_session, *full_uri = NULL;
    int                 int_v = 0;

    print_log0(debug, ".");

    if(erss_status_setup > session->status)
    {
        print_log0(err, "session state not match, already over setup status.");
        return rtsp__send_err_reply(session, 400, NULL);
    }
    session->status = erss_status_record;
    
    /* parse the url */
    if ( (NULL == hmsg) || (NULL == (full_uri = http_msg_get_full_uri(hmsg)))
        || (0 != url_parse(full_uri->data, full_uri->len, &session->url.desc)))
    {
        print_log1(err, "requst with invalid url["len_str_format_s"]", len_str_format(full_uri));
        return rtsp__send_err_reply(session, 500, NULL );     
    }   
    
    /* check cseq field */
    if ( NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq))
        || (1 != sscanf(p_cseq->data, "%d", &int_v)))
    {              
        print_log0(err, "missing valid Cseq.");
        return rtsp__send_err_reply(session, 400, NULL);          /* bad request */
    }   
    session->cseq = (unsigned long)int_v;
    
    /* check session field */
    if ((p_session = http_msg_get_known_header(hmsg, ehmht_session)))
    {
        memcpy(&(session->session_id[0]), p_session->data, p_session->len);      
        session->session_id[p_session->len] = 0;
    }
    else
    {
        print_log0(err, "missing session field.");
        return rtsp__send_err_reply(session, 454, NULL);  
    }            
    return rtsp__send_record_reply(session);
}

long rtsp__on_pause( struct rtsp_session *session )
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_pause(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");
    print_log0(err, "current not support pause now.");
    return rtsp__send_pause_reply(session);
}
long rtsp__send_pause_reply( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_pause_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    print_log0(debug, ".");
    return rtsp__send_err_reply(session, 405, NULL);
}

long rtsp__on_teardown( struct rtsp_session *session )
{        
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_teardown(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module  *mod = session->in_list.mod;
    struct http_msg     *hmsg = rtsp_session_get_recv_msg(session);
    struct len_str      *p_cseq, *p_session;
    int                 int_v = 0;

    print_log0(debug, ".");

    session->status = erss_status_teardown;
    /* check cseq field */
    if ( NULL == (p_cseq = http_msg_get_known_header(hmsg, ehmht_cseq))
        || (1 != sscanf(p_cseq->data, "%d", &int_v)))
    {
        print_log0(err, "missing valid Cseq.");
        return rtsp__send_err_reply(session, 400, NULL);            /* bad request */
    }
    session->cseq = (unsigned long)int_v;
    
    /* check session field */
    if ( NULL != (p_session = http_msg_get_known_header(hmsg, ehmht_session)))
    {
        memcpy(&(session->session_id[0]), p_session->data, p_session->len);      
        session->session_id[p_session->len] = 0;
    }
    else
    {
        print_log0(err, "missing session field.");
        return rtsp__send_err_reply(session, 454, NULL);   
    }      
    
    return rtsp__send_teardown_reply(session);
}

long rtsp__send_teardown_reply( struct rtsp_session *session )
{ 
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__send_teardown_reply(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    char          response[200];                            
    unsigned long len;

    print_log0(debug, ".");

    len = sprintf(response, 
                  "%s %d %s"RTSP_EL
                  "Server: %s/%s"RTSP_EL
                  "CSeq: %ld"RTSP_EL
                  "Session: %s"RTSP_EL
                  "Connection: Close"RTSP_EL
                  RTSP_EL,
                  RTSP_VERSION, 200, rtsp__get_status(200), RTSP_PACKAGE_NAME, RTSP_PACKAGE_VERSION,
                  session->cseq, session->session_id);
    session->machine_state = ersms_state_init;
    return rtsp__add_rtsp_package(session, (unsigned char*)response, len);
}
