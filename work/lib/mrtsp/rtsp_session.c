/*!
\file       rtsp session.c
\brief      rtsp unit

----history----
\author     xionghuatao
\date       2009-11-15
\version    0.01
\desc       create

$Author: xionghuatao $
$Id: rtsp.c,v 1.14 2009-11-15 17:28:27 xionghuatao Exp $
*/

#define mmodule_name "lib.mlib.mrtsp.rtsp_session"
#include "mcore/mcore.h"
#include "msdp/msdp.h"
#include "rtsp_mod.h"
#include "rtsp__session.h"
#include "rtp__packet.h"
#include "rtp__mod.h"

#if defined(_MSC_VER) && !defined(snprintf)
#   define snprintf _snprintf
#endif

/* void RTP_Schd(); */
extern long     g_ssl_loaded;

void rtsp__print_media_data( unsigned char *data, unsigned long length)
{
    unsigned long temp_size = 0;
    if ( NULL == data || 0 == length )
        return;   
    while ( temp_size < length )
    {
        print_debug(" %2.2x ", (unsigned char ) data[temp_size]);        
        if ( 0 ==  (temp_size + 1) % 16 )
        {
            print_debug("\n");
        }
        temp_size++;
    }
    print_debug("\n\n");
}

void rtsp__dump_data(FILE *fp, unsigned char *data, unsigned long len)
{
    unsigned long i  = 0;
    unsigned char tmp = 0;
    
    if (NULL == fp)   return;
    
    fprintf(fp, "\ntime [%s] start dump data! size[%ld]\n", mtime2s(0), len);   
    while ( i < len )
    {
        tmp = (unsigned char )data [i];
        fprintf(fp, "%.2x ", tmp);        
        if ( (i + 1)% 16 == 0)
        {
            fprintf(fp, "\n");
        }
        i++;     
    }
    fprintf(fp,"\n");   
}


long rtsp__set_sdp(struct rtsp_session *session, char *sdp, unsigned long len)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__set_sdp(session["rtsp_session_format_s"], sdp[%p{%*.*s}], len[%ld])"
#define func_format()   rtsp_session_format(session), sdp, 0, (len>64)?64:len, sdp, len

    long                ret = 0;
    char                *save_sdp = NULL;
    struct sdp_block    *sdp_blk = NULL;

    print_log0(debug, ".");

    if((NULL == sdp) || (0 == sdp))
    {
        print_log0(err, "faield with invalid param.");
        ret = -1;
    }    
    else if(NULL == (save_sdp = (char *)malloc(len + 1)))
    {
        print_log0(err, "failed when save sdp content.");
        ret = -2;
    }
    else
    {
        memcpy(save_sdp, sdp, len);    
        save_sdp[len] = 0;
        if((NULL == (sdp_blk = sdp_create(save_sdp, len))))
        {
            free(save_sdp);
            print_log0(err, "failed when parse sdp content.");
            ret = -3;
        }
        else
        {
            if(session->sdp.data){free(session->sdp.data); session->sdp.data = NULL; };
            if(session->sdp.desc){sdp_destroy(session->sdp.desc); session->sdp.desc = NULL; };
            session->sdp.len  = len;
            session->sdp.data = save_sdp;
            session->sdp.desc = sdp_blk;
        }
    }

    if((0 == session->have_send_sdp)
        && (erst_server_play == session->session_type)
        && (erss_status_describe == session->status))
    {
        print_log0(debug, "sdp ready, send out describe reply.");
        if(ret || rtsp__on_sdp(session))
        {
            print_log0(err, "dealwith sdp failed.");
            rtsp__send_err_reply(session, 500, NULL);
            return ret?ret:-4;
        }
        return rtsp__send_describe_reply(session);
    }
    if((0 == ret)
        && (erst_client_record == session->session_type)
        && (0 == session->have_send_sdp))
    {
        print_log0(debug, "sdp ready, send out announce.");
        return rtsp__req_announce(session);
    }    
    return 0;
}

const char *rtsp_conn2s( struct rtsp_session *session )
{
    static char buf[256];
    long i = 0;
    if ( session )
    {
        i += snprintf(&buf[i], sizeof(buf) - 1 - i, "%p{sock[%ld{%s}]", session, session->sock->fd, netx_stoa(session->sock->fd));
        buf[i++] = '}';
        buf[i] = 0;
    }
    return (const char*)&buf;
}

long rtsp_is_x_session(struct http_msg *hmsg)
{
    struct len_str *value;
    if((value = http_msg_get_known_header(hmsg, ehmht_x_sessioncookie))
       && value->len 
       && (0 == hmsg->headers.is_rsp))
    {
        if((0 == len_str_casecmp_const(&hmsg->headers.start_line.desc.req.method, "GET"))
            && (value = http_msg_get_known_header(hmsg, ehmht_accept))
            && (0 == len_str_casecmp_const(value, "application/x-rtsp-tunnelled")))
        {/* is rtsp get from quick time */
            return 1;
        }
        else if((0 == len_str_casecmp_const(&hmsg->headers.start_line.desc.req.method, "POST"))
            && (value = http_msg_get_known_header(hmsg, ehmht_content_type))
            && (0 == len_str_casecmp_const(value, "application/x-rtsp-tunnelled")))
        {/* is rtsp post from quick time */
            return 1;
        }
    }
    return 0;
}

long rtsp__on_tcp_recv_data( struct rtsp_session *session )
{    
#undef func_format_s
#undef func_format
#define func_format_s   "rtsp__on_tcp_recv_data(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_session *ref_session;
    struct rtsp_sock    *sock = session->sock;
    char                *recv_buffer = sock->recv.buffer,  *buf = NULL;              
    unsigned long       data_size = 0, buf_size = 0, app_size = 0, in_size = (sock->recv.length);
    long                ret, tmp; 
    unsigned long       msg_size = 0;
    
    print_log0(detail, ".");

    /* interleaved data begin with  0x24 i.e. "$." */ 
    if ((char)0x24 == recv_buffer[0])  
    {
        if((4 > sock->recv.length)
            || (data_size = (unsigned long )ntohs( (*(unsigned short *)&recv_buffer[2]))) > (in_size - 4))           
        { 
            print_log3(detail, "invalid recv.length[%ld] size[%ld] in_size[%ld].", sock->recv.length, data_size, in_size);
            return 1;        
        }         
        sock->recv.index = 4 + data_size;        
        if (rtsp__on_rtp_interleaved_data(session, (unsigned char*)recv_buffer, sock->recv.index))
        {
            print_log0(warn, "failed deal with interleaved rtp data not success.");
            return -1;
        }
        return 0;
    }

    /* deal with rtsp protocol data */
    do 
    {   
        if(((NULL == session->hmsg) && (NULL == (session->hmsg = http_msg_create(NULL, NULL))))
           ||(NULL == (buf = http_msg_buf_prepare(session->hmsg, &buf_size))))
        {
            print_log0(err, "meet err when http_msg_buf_prepare().");
            if(session->hmsg)
            { 
                http_msg_destroy(session->hmsg); 
                session->hmsg = NULL;
            }
            return  ERR_ALLOC_FAILED;
        }          
        
        if (0 > (tmp = (long) (in_size - sock->recv.index)))
        {
            print_log1(err, "the msg index is beyond the total length . ret [%ld]", tmp);
            return ERR_GENERIC;
        }
        
        if(rtsp_x_session_type_post != session->x_session.type)
        {/* normal session */
            app_size = (buf_size > (unsigned long) tmp)? (unsigned long) tmp : buf_size;
            memcpy(buf, recv_buffer + sock->recv.index, app_size);
            sock->recv.index += app_size;
        }
        else
        {/* apple quick time x session */
            app_size = (buf_size > (unsigned long) tmp)? (unsigned long) tmp : buf_size;
            //app_size &= -4;
            //if(app_size)
            {
                if(0 > (ret = base64_decode((unsigned char*)recv_buffer + sock->recv.index, app_size, (unsigned char*)buf, app_size)))
                {
                    print_log1(err, "failed when base64_decode() with ret [%ld]", ret);
                    return ERR_GENERIC;
                }

                sock->recv.index += app_size;
                app_size = ret;
            }
        }
        ret = http_msg_append( session->hmsg, app_size );

        if ( 0 > ret )
        { /* err */
            print_log1(err, "failed when http_msg_append() with ret [%ld]", ret);
            return ERR_GENERIC;
        }
        else if(http_msg_header_finished(session->hmsg))
        {
            struct len_str  *req_method = http_msg_get_req_method(session->hmsg),
                            *version = http_msg_get_req_version(session->hmsg);
            if((erss_status_init == session->status)
                && (0 == len_str_case_begin_const(version, "HTTP"))
                && (0 == session->x_session.type))
            {/* x-session */
                struct len_str  *s_value;
                if((0 == len_str_case_begin_const(req_method, "GET"))
                   && (s_value = http_msg_get_known_header(session->hmsg, ehmht_accept))
                   && s_value->len
                   && (0 == len_str_casecmp_const(s_value, "application/x-rtsp-tunnelled")))
                {/* is x-session get */
                    if(session->x_session.cookie.len || session->x_session.ref)
                    {
                        print_log0(err, "x-sessioncookie allready bound.");
                        return ERR_GENERIC;                            
                    }
                    if(len_str_dup(&session->x_session.cookie, s_value->len, s_value->data))
                    {
                        print_log0(err, "save x-sessioncookie failed.");
                        return ERR_GENERIC;                            
                    }
                    session->x_session.type = rtsp_x_session_type_get;
                }
                else if((0 == len_str_case_begin_const(req_method, "POST"))
                    && (s_value = http_msg_get_known_header(session->hmsg, ehmht_content_type))
                    && s_value->len
                    && (0 == len_str_casecmp_const(s_value, "application/x-rtsp-tunnelled")))
                {/* is x-session post */
                    unsigned long       content_len;
                    struct rtsp_session *check_session = session->in_list.mod->useds.list;
                    if(len_str_dup(&session->x_session.cookie, s_value->len, s_value->data))
                    {
                        print_log0(err, "save x-sessioncookie failed.");
                        return ERR_GENERIC;                            
                    }
                    if(check_session)
                    {
                        do 
                        {
                            if(0 == len_str_cmp(s_value, &check_session->x_session.cookie))
                            {/* found */
                                ref_session = check_session;
                                break;
                            }
                        } while ((check_session = check_session->in_list.next) != session->in_list.mod->useds.list);
                    }

                    if(NULL == ref_session)
                    {
                        print_log0(err, "unknonw x-sessioncookie refer POST session(missing GET).");
                        return ERR_GENERIC;                            
                    }
                    if(ref_session->x_session.ref)
                    {
                        print_log0(err, "refer GET x-session allready bound.");
                        return ERR_GENERIC;                            
                    }
                    session->x_session.type     = rtsp_x_session_type_post;
                    ref_session->x_session.ref  = session;
                    session->x_session.ref      = ref_session;
                    
                    content_len = http_msg_get_content_len(session->hmsg);
                    if(content_len)
                    {/* copy remained content into recv buffer */
                        /* hack header->content.len */
                        http_msg_set_header_content_length(session->hmsg, 0xffffffff);
                        if(sock->recv.index < content_len)
                        {
                            recv_buffer = (char*)realloc(sock->recv.buffer, sock->recv.size + (content_len - sock->recv.index));
                            if(NULL == recv_buffer)
                            {
                                print_log0(err, "failed when realloc recv-buffer.");
                                return ERR_GENERIC;                            
                            }
                            sock->recv.buffer = recv_buffer;
                            sock->recv.size += (content_len - sock->recv.index);
                            sock->recv.length += (content_len - sock->recv.index);
                            sock->recv.index = 0;
                            in_size = sock->recv.length;
                        }
                        else
                        {
                            sock->recv.index -= content_len;
                        }
                        if(content_len != http_msg_get_content(session->hmsg, content_len, sock->recv.buffer + sock->recv.index))
                        {
                            print_log0(err, "failed when http_get_content().");
                            return ERR_GENERIC;                            
                        }
                    }
                    http_msg_destroy(session->hmsg);
                    session->hmsg = NULL;
                    continue;
                }
            }
            
            if ( http_msg_finished(session->hmsg) || (0xffffffff == http_msg_get_header_content_length(session->hmsg)))
            {   /* ok */
                if(rtsp_x_session_type_post != session->x_session.type)
                {
                    msg_size = (http_msg_get_headers_len(session->hmsg) 
                                    + ((0xffffffff == http_msg_get_header_content_length(session->hmsg))?0:http_msg_get_content_len(session->hmsg)));   
                    if ((sock->recv.index != msg_size) && (msg_size != 0) && (msg_size < sock->recv.length))
                    {
                        sock->recv.index = msg_size;
                    }
                }
                if(rtsp_x_session_type_post == session->x_session.type)
                {
                    if(NULL == session->x_session.ref)
                    {
                        print_log0(err, "failed with missing ref GET x-session.");
                        return ERR_GENERIC;
                    }
                    print_log3(detail, "recv rtsp message: \r\n%*.*s\r\n", 0, (int)sock->recv.index, recv_buffer);
                    return rtsp__on_rtsp_msg_data(session->x_session.ref);
                }
                else
                {
                    print_log3(detail, "recv rtsp message: \r\n%*.*s\r\n", 0, (int)sock->recv.index, recv_buffer);
                    return rtsp__on_rtsp_msg_data(session);
                }
            }        
        }
    } while ( sock->recv.index < in_size);    

    print_log0(detail, "still need recv data.");
    return 1;
}

long rtsp__valid_response_msg( struct rtsp_session *session, unsigned long *stat)
{       
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__valid_response_msg(session["rtsp_session_format_s"], stat[%p{%ld}])"
#define func_format()   rtsp_session_format(session), stat, stat?*stat:0

    struct http_msg     *hmsg = ((rtsp_x_session_type_get == session->x_session.type) && (session->x_session.ref))?session->x_session.ref->hmsg:session->hmsg;
    struct len_str      *p_ceq, *version;
    int                 seq = 0;

    print_log0(debug, ".");

    if ( (NULL == (p_ceq = http_msg_get_known_header(hmsg, ehmht_cseq))) 
        || (1 != sscanf(p_ceq->data, "%d", &seq)))
    {
        struct len_str  *req_method = http_msg_get_req_method(hmsg);
        if((rtsp_x_session_type_get == session->x_session.type)
           && http_msg_is_req(hmsg)
           && (0 == len_str_casecmp_const(req_method, "GET")))
        {/* quick time x session */
            return 0;
        }
        print_log0(err, "missing Creq field.");
        return -1;
    }   

    if(http_msg_is_req(hmsg))
    {/* is request */
        return 0;
    }
    else if((version = http_msg_get_rsp_version(hmsg)) && version->len)
    {
        if ((session->cseq != (unsigned long)seq) && ((seq + 1) != session->cseq))           
        {
            print_log2(err, "invalid sequence session cseq [%d], need seq [%d].", session->cseq, seq);
            return -1;
        }
        *stat = http_msg_get_rsp_code_value(hmsg);
        print_log0(debug, "got a response.");
        return 1;
    }

    print_log0(err, "invalid message.");
    return -1;
}

/*!
func    rtsp_mod_create
\brief  create net module
\param  params[in]              callback routines, must be not null, if null, rtsp module create failed
\return the pointer to net module
        #NULL                   failed
        #other                  net module
*/
struct rtsp_module *rtsp_mod_create(struct rtsp_params  *params)
{         
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_mod_create(params[%p{port[%ld]}])"
#define func_format()   params, params?params->net.rtsp_port:0

    unsigned long max_conns = (params && 0 < params->net.max_conns) ? params->net.max_conns : RTSP_MAX_CONN_SIZE;
    unsigned long i;
    unsigned long size =  sizeof(struct rtsp_module) + sizeof(mlock_simple) +
                          (sizeof(struct rtsp_session) * max_conns) + 
                          (sizeof(struct netx_event) * (5 * max_conns + 1)) + (sizeof(struct rtsp_sock) * 3);       
    struct rtsp_module  *mod = NULL; 
    struct netx_event   evt;  
    
    print_log0(debug, ".");

    if( (NULL == params )
        || ( !(params->events.client_play_close && params->events.client_play_data && params->events.client_record_close 
            && params->events.server_play_close && params->events.server_play_req && params->events.server_record_close 
            && params->events.server_record_data && params->events.server_record_req)) ) 
    {
        print_log0(err, "invalid param.");
        return NULL;
    } 
    if(NULL == (mod = ( struct rtsp_module *) calloc(size, 1)))
    {
        print_log1(err, "failed when malloc(%ld) module.", size);
        return NULL;
    }
    mod->lock = (mlock_simple*)&mod[1];
    mlock_simple_init(mod->lock);
    mod->params = *params;
    mod->local_addr.s_addr = 0;
    mod->rtsp_server.fd = -1;
    mod->rtp_server.fd = -1;
    mod->rtcp_server.fd = -1;   
    mod->rtsp_server.port = params->net.rtsp_port;
    
    mod->time_out = RTSP_SESSION_TIMEOUT * 1000;
    mod->max_conn_counts = max_conns;
    mod->now_tick = mod->start_tick = mtime_tick(); 
    
    mod->udp_ports.min_udp_port = (0 < params->net.udp_min)? params->net.udp_min : 10000;
    mod->udp_ports.max_udp_port = ((0 < params->net.udp_max) && (params->net.udp_min + 4 < params->net.udp_max )) ? params->net.udp_max : 20000;
    mod->udp_ports.used_index = (mod->udp_ports.min_udp_port & 0X0001) ? mod->udp_ports.min_udp_port + 1 : mod->udp_ports.min_udp_port;
    
    mod->rtsp_sock = (struct rtsp_sock *) (((char*)mod->lock) + sizeof(mlock_simple));
    mod->rtp_sock = (struct rtsp_sock *) &mod->rtsp_sock[1];
    mod->rtcp_sock = (struct rtsp_sock *) &mod->rtsp_sock[2];   
    mod->first_conn = (struct rtsp_session *) &mod->rtcp_sock[1];
    mod->events = (struct netx_event *) &mod->first_conn[max_conns];
    
    /* init the free connections */
    for ( i = 0; i < max_conns; ++i )
    {  
        rtsp_session_set_mod(&(mod->first_conn[i]), mod);            
        mlist_add(mod->frees, &(mod->first_conn[i]), in_list);
    }            
    if ( 0 > (mod->epoll_fd = netx_create(max_conns * 5)))
    {
        print_log0(err, "failed when rtsp_mod_create().");
        rtsp_mod_destroy(mod);
        return NULL;    
    }         
    if ( (0 < mod->rtsp_server.port)
        && (0 < (mod->rtsp_server.fd = netx_open(SOCK_STREAM, &mod->local_addr, mod->rtsp_server.port, netx_open_flag_reuse_addr))))
    {  
        mod->rtsp_sock->type = erst_rtsp;
        mod->rtsp_sock->owner_refer = mod;
        mod->rtsp_sock->fd = mod->rtsp_server.fd;

        evt.data.ptr = mod->rtsp_sock;
        evt.events = netx_event_et | netx_event_in;
        if ( netx_ctl(mod->epoll_fd, netx_ctl_add, mod->rtsp_server.fd, &evt))
        {
            print_log2(err, "faield when add module fd [%ld] on tcp port [: %ld] to epoll polling.",
                       mod->rtsp_server.fd, mod->rtsp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
    }
    
    if (rtsp__rtp_get_port_pair(mod, &mod->rtp_server, &mod->rtcp_server))
    {
        print_log1(err, "failed when open rtp and rtcp port[%ld].", mod->rtcp_server.port);
        rtsp_mod_destroy(mod);
        return NULL; 
    }
    
    if((0 < mod->rtp_server.port) && (0 < mod->rtp_server.fd))
    { /* rtp */  
        long size = 0x200000; 
        mod->rtp_sock->type = erst_rtp;
        mod->rtp_sock->fd = mod->rtp_server.fd;
        mod->rtp_sock->owner_refer = mod;
        
        evt.data.ptr = mod->rtp_sock;
        evt.events = netx_event_et;
        if ( netx_ctl(mod->epoll_fd, netx_ctl_add, mod->rtp_server.fd, &evt))
        {
            print_log2(err, "failed  when add server rtp fd [%ld] on udp port [: %ld] to epoll polling.",
                       mod->rtp_server.fd, mod->rtp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
        setsockopt(mod->rtp_server.fd, SOL_SOCKET, SO_SNDBUF, (const char *)&size, sizeof(int)); 
    }        
    if((0 < mod->rtcp_server.port) && (0 < mod->rtcp_server.fd ))
    {/* rtcp */
        long size = 0x200000; 
        mod->rtcp_sock->type = erst_rtcp;
        mod->rtcp_sock->fd = mod->rtcp_server.fd;
        mod->rtcp_sock->owner_refer = mod;
        
        evt.data.ptr = mod->rtcp_sock;
        evt.events = netx_event_et;
        if(netx_ctl(mod->epoll_fd, netx_ctl_add, mod->rtcp_server.fd, &evt))
        {
            print_log2(err, "failed when add server rtcp fd [%ld] on udp port [: %ld] to epoll polling.",
                       mod->rtcp_server.fd, mod->rtcp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
        setsockopt(mod->rtcp_server.fd, SOL_SOCKET, SO_SNDBUF, (const char *)&size, sizeof(int)); 
    }

#if defined(mrtsp_ssl_enable) && mrtsp_ssl_enable
	mod->ssl_enable = params->ssl.cert 
						&& params->ssl.cert->len 
						&& params->ssl.chain
						&& params->ssl.chain->len
						&& params->ssl.prik
						&& params->ssl.prik->len
						&& params->ssl.ssl_enable;
    if(mod->ssl_enable){
        print_log0(warn, "RTSP is based on SSL");
        mod->cert = params->ssl.cert->data;
        mod->chain = params->ssl.chain->data;
        mod->prik = params->ssl.prik->data;
		
		//The code below is thread-unsafe, make sure there is only one thread invoking them at same time.
		if( g_ssl_loaded == 0 )
        {
			g_ssl_loaded++;
    		SSL_library_init();
            SSL_load_error_strings();
            ERR_load_BIO_strings();
        }      
		
        mod->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
        if(mod->ssl_ctx == NULL){
            ERR_print_errors_fp(stderr);
            print_log2(err, "failed when crate ssl ctx for rtsp server [%s:%d].",
                inet_ntoa(mod->local_addr), mod->rtsp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
        else if(0 >= SSL_CTX_use_certificate_file(mod->ssl_ctx, mod->cert, SSL_FILETYPE_PEM))
        {
            ERR_print_errors_fp(stderr);
            print_log3(err, "faild when load cert file[%s] for rtsp server [%s:%d].",
                mod->cert, inet_ntoa(mod->local_addr), mod->rtsp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
        else if(0 >= SSL_CTX_use_certificate_chain_file(mod->ssl_ctx, mod->chain)){
            ERR_print_errors_fp(stderr);
            print_log3(err, "faild when load chain file[%s] for rtsp server [%s:%d].",
                mod->chain, inet_ntoa(mod->local_addr), mod->rtsp_server.port);
            rtsp_mod_destroy(mod);
            return NULL; 
        }
        else if(0 >= SSL_CTX_use_PrivateKey_file(mod->ssl_ctx, mod->prik, SSL_FILETYPE_PEM)){
            ERR_print_errors_fp(stderr);
            print_log3(err, "faild when load prik file[%s] for rtsp server [%s:%d].",
                mod->prik, inet_ntoa(mod->local_addr), mod->rtsp_server.port); 
            rtsp_mod_destroy(mod);
            return NULL; 		
        }
        else if(0 >= SSL_CTX_check_private_key(mod->ssl_ctx)){
            ERR_print_errors_fp(stderr);
            print_log3(err, "faild when check prik file[%s] for rtsp server [%s:%d].",
                mod->prik, inet_ntoa(mod->local_addr), mod->rtsp_server.port); 
            rtsp_mod_destroy(mod);
            return NULL; 		
        }
    }
    else{
        print_log0(warn, "RTSP isn't based on SSL");
    }
	#endif

    srand(mtime_us());

    /* start rtp block */
/*    RTP_Start(); */
    print_log4(info, " ret[%p{port{tcp[%ld], udp[%ld - %ld]}}].",
               mod, mod->rtsp_server.port, mod->rtp_server.port, mod->rtcp_server.port);
    return mod;        
}

/*!
func    rtsp_mod_destroy
\brief  destroy net module, terminate the rtsp server
\param  mod [in]                the net module created by rtsp_mod_create()
\return the result of destroying the module 
        #0                      succeed
        #other                  error code
*/
long rtsp_mod_destroy(struct rtsp_module *mod)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_mod_destroy(mod["rtsp_module_format_s"])"
#define func_format()   rtsp_module_format(mod)

    struct rtsp_session     *conn;    
    struct netx_event       evt ={0};
    
    if(NULL == mod)
    {
        print_log0(err, "failed with invalid param.");
        return -1;
    }

    print_log0(info, ".");
    /* stop rtp block */
/*    RTP_Stop(); */

    mlock_simple_wait(mod->lock);    
    /* close the connections being linked */
    while((conn = mod->useds.list))
    {
        rtsp__close_session(conn);
    }
    /* close server listen socket */
    if( 0 < mod->rtsp_server.fd )
    {
        if( ( 0 > mod->epoll_fd ) || netx_ctl(mod->epoll_fd, netx_ctl_del, mod->rtsp_server.fd, &evt))
        {         
            print_log1(err, "meet err when delete rtsp module fd[%ld].", mod->epoll_fd);
        }
        netx_close(mod->rtsp_server.fd );       
    }

    /* close server rtp socket */
    if (0 < mod->rtp_server.fd)
    {        
        if( (0 > mod->epoll_fd) || netx_ctl(mod->epoll_fd, netx_ctl_del, mod->rtp_server.fd, &evt) )     
        {
            print_log0(err, "meet err when netx_ctl() with op. [del].");
        }
        netx_close(mod->rtp_server.fd );            
    } 

    /* close server rtcp */
    if ( 0 < mod->rtcp_server.fd )
    {
        if( (0 > mod->epoll_fd) || netx_ctl(mod->epoll_fd, netx_ctl_del, mod->rtcp_server.fd , &evt)) 
        {
            print_log0(err, "meet err when netx_ctl() with op. [del].");          
            netx_close(mod->rtcp_server.fd );
        }
        netx_close(mod->rtcp_server.fd);         
    }    
    if( (0 > mod->epoll_fd) || netx_destroy(mod->epoll_fd))
    {
        print_log0(err, "meet err close epoll fd failed.");
    }    

#if defined(mrtsp_ssl_enable) && mrtsp_ssl_enable
    if(mod->ssl_ctx){
        SSL_CTX_free(mod->ssl_ctx);
    }
#endif

    mlock_simple_release(mod->lock);
    mlock_simple_uninit(mod->lock);
    free(mod);
    return 0;
}

long rtsp_mod_wait( struct rtsp_module *mod, unsigned long timeout)
{      
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_mod_wait(mod["rtsp_module_format_s"], timeout[%ld])"
#define func_format()   rtsp_module_format(mod), timeout

    struct netx_event   *epoll_events = NULL;
    long                result = 0, ret = 0;   
    struct rtsp_sock    *sock;
    unsigned long       event_tmp, umax_count = 0;
    struct rtsp_session *session = NULL;

    print_log0 (detail, ".");
    
    if((NULL == mod) || (0 > mod->epoll_fd))
    {
        print_log0(err, "failed with invalid param.");
        return ERR_INPUT_PARAM;
    }

    mod->now_tick = mtime_tick();
    
    /* waiting for the net event , return the count */
    epoll_events = mod->events;
    umax_count = mod->max_conn_counts * 5;    
    result = netx_wait(mod->epoll_fd, epoll_events, umax_count, timeout);        
    if ((result < 0) && (netx_eintr != netx_errno))
    { 
        print_log1(warn, "failed with netx_wait() result[%ld].", result);
        return 0;/* don't return -1 here */
    }
    
    mlock_simple_wait(mod->lock);
/*    RTP_Schd();    */
    /* deal with the net event here */
    while ( 0 < (result--))
    {        
        sock = (struct rtsp_sock *) epoll_events[result].data.ptr;
        event_tmp = epoll_events[result].events;
        if ( erst_rtsp == sock->type)
        {
           ret = rtsp__on_rtsp_event(mod, sock, event_tmp);         
        }
        else if( erst_rtp == sock->type)
        {
            ret = rtsp__on_rtp_event(mod, sock, event_tmp);                   
        }
        else if ( erst_rtcp == sock->type)
        {
           ret = rtsp__on_rtcp_event(mod, sock, event_tmp);
        }   
        
        if ( 0 > ret)
        {
            print_log1(warn, "meet something not perfect well done when deal with sock index [%ld] event.", result + 1);
        }       
    }    

	
    while( (NULL != (session = mod->useds.list)) && ((mod->now_tick - session->tick) > mod->time_out))
    {
        print_log3(warn, "meet timeout session["rtsp_session_format_s"] server time[%u] session time [%u] . then close it.",
                   rtsp_session_format(session), mod->now_tick, session->tick);		
		
        rtsp__close_session(session);
    }       
    mlock_simple_release(mod->lock);    
    return ERR_NO_ERROR;    
}

long rtsp__on_rtsp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtsp_event(mod["rtsp_module_format_s"], sock["rtsp_sock_format_s"], timeout[%ld])"
#define func_format()   rtsp_module_format(mod), rtsp_sock_format(sock), events

    print_log0(detail, ".");

    if(mod == sock->owner_refer)
    {  
        struct rtsp_sock    *tmp_sock;
        struct sockaddr_in  addr;                
        struct rtsp_session *session = NULL;
        struct netx_event   evt;
        long                socket, accept_counts = 0;
        
        while(0 <= (socket = netx_accept(mod->rtsp_server.fd, &addr)))
        {        
            ++accept_counts;
            if( (0 >= socket) || (NULL == (session = mod->frees.list))                    
                || (NULL == (tmp_sock = calloc(1, sizeof(struct rtsp_sock)))
                /* || NULL == (session->sort_channel = RTP_CreateDecodeChl(RTP_STRM_RTP, 2000))*/))
            { 
                print_log0(err, "failed when create rtsp session.");
                netx_close(socket);
                if ( tmp_sock )  {free(tmp_sock);}; 
                return ERR_GENERIC;
            }             
        
            session->sock = tmp_sock;
            session->remote_addr = addr;
            session->status = erss_status_init;
            session->machine_state = ersms_state_init;
            session->session_type = erst_server_play;
            session->tick = mod->now_tick;
			session->rtp_audio_count = 0;
			session->rtp_video_count = 0;
			session->rtp_audio_octet = 0;
			session->rtp_video_octet = 0;
			session->rtp_audio_ssrc = 0;
			session->rtp_video_ssrc = 0;
			session->audio_sample_buff.list = NULL;
			session->audio_sample_buff.counts = 0;
			session->rtp_allowed_latency = 500;
            rtsp_session_set_mod(session, mod);                                   
        
            tmp_sock->fd = socket;
            tmp_sock->type = erst_rtsp;
            tmp_sock->owner_refer = session;                                      
    
            evt.data.ptr = tmp_sock;
            evt.events = netx_event_et | netx_event_in;        
            if (netx_ctl(mod->epoll_fd, netx_ctl_add, tmp_sock->fd, &evt))
            {            
                print_log2(err, "failed when netx_ctl(sock[%ld{%s}], operation[add]).", socket, netx_stoa(socket));
                free(sock);
                netx_close(socket);                 
            }
            else{
				#if defined(mrtsp_ssl_enable) && mrtsp_ssl_enable
                if(mod->ssl_enable && mod->ssl_ctx)
                {
                    long ssl_ret, set_fd_ret;
                    session->ssl = SSL_new(mod->ssl_ctx);
                    if(session->ssl == NULL
                        || ((0 >= (set_fd_ret = SSL_set_fd(session->ssl, session->sock->fd))
                            || 0 >= (ssl_ret = SSL_accept(session->ssl)))
                            && ( 0 > ssl_ret) 
                            && (SSL_ERROR_WANT_READ != SSL_get_error(session->ssl, ssl_ret)))){
                        print_log4(err, "err when %s(sock[%ld{%s}], op[add]) ret[%ld].",
                            session->ssl?(set_fd_ret?"ssl-set-fd" : "ssl-accept") : "ssl-new",
                            session->sock->fd, netx_stoa(session->sock->fd), (long)session->ssl?SSL_get_error(session->ssl, ssl_ret) : 0);
                        if(session->ssl){
                            SSL_free(session->ssl);
							session->ssl = NULL;
                        }
                    }
                }
				#endif
            }        

            mlist_del(mod->frees, session, in_list);  
            mlist_add(mod->useds, session, in_list);
            mlist_move_to_last(mod->useds, session, in_list);
            print_log2(debug, "accept a connection sock[%ld{%s}])", socket, netx_stoa(socket));
        }
        if((0 == accept_counts) || (netx_errno && (netx_ewouldblock != netx_errno)))
        {
            print_log2(err, "accept with net-errno[%d] with accept-counts[%ld].", netx_errno, accept_counts);
        }
        return 0;
    }
    else
    { 
        long ret = 0;
        struct rtsp_session *session = (struct rtsp_session *)sock->owner_refer;
        session->tick = mod->now_tick; 
        mlist_move_to_last(mod->useds, session, in_list);
        if (((erst_client_play == session->session_type )
            || (erst_client_record == session->session_type ))
            && (session->tick - session->prev_set_tick) > 3000) //chenyong add
        {
            rtsp__req_set_parameter(session);
            session->prev_set_tick = session->tick;
        }
        if ( (ret = (events & netx_event_in)?rtsp__on_tcp_recv(session):0)
            || ((0 == ret) && (events & netx_event_out) && (ret = rtsp__on_tcp_send(session))) )
        {
            if((ERR_CONN_CLOSED == ret) || (ERR_GENERIC == ret) || session->close_reason == rtsp_session_close_reason_by_user)
            {
                print_log1(debug, "meet close connection or server err when rtsp_on_tcp_%s().",
                           (events & netx_event_in)?"recv":"send");
                rtsp__close_session(session);
            }
            return 0;
        }
        else if(events & netx_event_et)
        {
            print_log0(debug, "meet NETX-EVENT-ET close it.");
            rtsp__close_session(session);               
            return 0;
        }
    }            
    return 0;
}

long rtsp__on_rtp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events)
{        
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtp_event(mod["rtsp_module_format_s"], sock["rtsp_sock_format_s"], timeout[%ld])"
#define func_format()   rtsp_module_format(mod), rtsp_sock_format(sock), events

    struct sockaddr_in      addr = {0};     
    long                    ret = 0;  
    unsigned int            addrlen = sizeof(struct sockaddr_in);

    print_log0(detail, ".");

    if( sock && (0 < sock->fd) && ((events & netx_event_in )))
    {  
        struct rtp_channel *channel = (struct rtp_channel *) sock->owner_refer;
        struct rtsp_session *session = channel->in_list.session;
        addr  = channel->transport.udp.rtp_addr;
        
        if (NULL == sock->recv.buffer)
        {
            sock->recv.size = RTSP_RTP_BUFFERSIZE;
            if (NULL == (sock->recv.buffer = malloc(sock->recv.size)))
            {
                print_log0(err, "meet err when alloc memory for recv buffer.");
                return ERR_GENERIC;
            }
            sock->recv.index = sock->recv.length = 0; 
        }
        ret = recvfrom(sock->fd, sock->recv.buffer, sock->recv.size, 0, (struct sockaddr *)&addr, &addrlen);        
        if (ret > 0 )
        {    
            print_log3(detail, "session["rtsp_session_format_s"] recv a rtp message on port[%ld], len[%ld].",
                       rtsp_session_format(session), channel->transport.udp.local_port.rtp, ret);
            sock->recv.index = 0;
            sock->recv.length = ret;
        }
        else if((ret < 0) || (ret == 0))
        {
            print_log4(err, "session["rtsp_session_format_s"] recv err on port[%ld], ret[%ld] with net-errno[%ld].",
                       rtsp_session_format(session), channel->transport.udp.local_port.rtp, ret, netx_errno);
            return  ERR_GENERIC;
        }           
        session->tick = mod->now_tick;
        mlist_move_to_last(mod->useds, session, in_list);
        rtsp__on_rtp_recv_data(channel, (unsigned char*)sock->recv.buffer, sock->recv.length);
        return ERR_NO_ERROR;
    }
    
    /* event out */ 
    if( sock &&  (0 < sock->fd) && ( events & netx_event_out ))  
    {   
         return rtsp__on_udp_send(sock);
    }
    return ERR_NO_ERROR;  
}

long rtsp__on_rtcp_event(struct rtsp_module *mod, struct rtsp_sock *sock, unsigned long events)
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtcp_event(mod["rtsp_module_format_s"], sock["rtsp_sock_format_s"], timeout[%ld])"
#define func_format()   rtsp_module_format(mod), rtsp_sock_format(sock), events

    struct sockaddr_in      addr = {0};     
    long                    ret = 0;  
    unsigned int            addrlen = sizeof(struct sockaddr_in);
    
    print_log0(detail, ".");
    if( sock && ( 0 < sock->fd) && (events & netx_event_in ))
    {  
        struct rtp_channel *channel = (struct rtp_channel *) sock->owner_refer;
        struct rtsp_session *session = channel->in_list.session;
        addr  = channel->transport.udp.rtcp_addr;
        
        if (NULL == sock->recv.buffer)
        {
            sock->recv.size = RTSP_RTCP_BUFFERSIZE;
            if (NULL == (sock->recv.buffer = (char*)malloc(sock->recv.size)))
            {
                print_log0(err, "meet err when alloc memory for recv buffer.");
                return ERR_GENERIC;
            }
            sock->recv.index = sock->recv.length = 0; 
        }
        
        ret = recvfrom(sock->fd, sock->recv.buffer, sock->recv.size, 0, (struct sockaddr *)&addr, &addrlen);        
        if (ret > 0 )
        {    
            print_log3(detail, "session["rtsp_session_format_s"] recv a rtp message on port[%ld], len[%ld].",
                       rtsp_session_format(session), channel->transport.udp.local_port.rtcp, ret);
            sock->recv.index = 0;
            sock->recv.length = ret;
        }
        else if (ret < 0 || ret == 0)
        {
            print_log4(err, "session["rtsp_session_format_s"] recv err on port[%ld], ret[%ld] with net-errno[%ld].",
                       rtsp_session_format(session), channel->transport.udp.local_port.rtcp, ret, netx_errno);
            return  ERR_GENERIC;
        }           
        session->tick = mod->now_tick;
        mlist_move_to_last(mod->useds, session, in_list);
        rtsp__on_rtcp_recv_data(channel, (unsigned char*)sock->recv.buffer, sock->recv.length);
        return ERR_GENERIC;
    }
    
    /* event out */ 
    if( sock && (0 < sock->fd) && ( events & netx_event_out ))  
    {   
        return rtsp__on_udp_send(sock);
    }
    return ERR_NO_ERROR;  
}

long rtsp__on_udp_send(struct rtsp_sock *sock)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_udp_send(sock["rtsp_sock_format_s"])"
#define func_format()   rtsp_sock_format(sock)

    struct rtsp_data    *sdata;
    long                len, ret;
    struct rtsp_module  *mod = (struct rtsp_module *)sock->owner_refer;
    struct netx_event   evt;

    print_log0(detail, ".");

    while ( NULL != ( sdata = sock->send.list))
    {
        len = (sdata->len -  sock->send.index );                                    
        ret = sendto(sock->fd, sdata->data + sock->send.index, len, 0, (struct sockaddr *)&sdata->addr, sizeof(struct sockaddr_in));
        sock->send.index += ret;     
        
        print_log4(detail, "send rtp data with send ret [%ld] from local port {%ld} to remote{%s [:%d]}.",
                   ret, mod->rtp_server.port, inet_ntoa( sdata->addr.sin_addr ), ntohs(sdata->addr.sin_port));
    
        /* deal with result of send data */
        if( ((0 < ret)  && (len > ret)) || ( (0 > ret) && (netx_ewouldblock == netx_errno)) )
        { /*  block */
            print_log4(debug, "send rtp data with send ret [%ld] from local port {%ld} to remote{%s [:%d]} blocking.",
                       ret, mod->rtp_server.port, inet_ntoa( sdata->addr.sin_addr ), ntohs(sdata->addr.sin_port));
            return ERR_NO_ERROR;
        }
        if( (0 == ret) || ( (0 > ret) &&  (netx_econnreset == netx_errno) ))
        { /* remote closed */
            print_log4(info, "send rtp data with send ret [%ld] from local port {%ld} to remote{%s [:%d]} closed.",
                       ret, mod->rtp_server.port, inet_ntoa( sdata->addr.sin_addr ), ntohs(sdata->addr.sin_port));
            return ERR_CONN_CLOSED;
        }
        if( 0 > ret )
        { /* error */
            print_log5(err, "send rtp data with send ret [%ld] from local port {%ld} to remote{%s [:%d]} with net-errno[%ld].",
                       ret, mod->rtp_server.port, inet_ntoa( sdata->addr.sin_addr ), ntohs(sdata->addr.sin_port), netx_errno);
            return ERR_GENERIC;
        }            
        
        /* deal with next message */
        if ( sock->send.index >= sdata->len )
        { 
            mlist_del(sock->send, sdata, in_list);
            free(sdata->data);
            free(sdata);    
            sock->send.index = 0;                                       
        }  
    }

    evt.data.ptr = sock;
    evt.events = netx_event_et;
    if ( mod->epoll_fd && netx_ctl(mod->epoll_fd, netx_ctl_mod, sock->fd, &evt))
    {             
        print_log0(err, "meet err when mod epoll fd.");   
        return ERR_GENERIC;
    }    
    return 0;
}

long rtsp__on_tcp_send( struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_tcp_send(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_sock    *sock = session->sock;
    struct rtsp_data    *sdata = NULL;   
    long                ret = 0, len = 0;    
    unsigned long       i = 0;
    struct netx_event   evt = {0};       
    
    print_log0(detail, ".");

    while(( sdata = sock->send.list ))
    {
        len = (sdata->len - sock->send.index);
		#if mrtsp_ssl_enable
        if(session->in_list.mod->ssl_enable && session->ssl){
            ret = SSL_write(session->ssl, sdata->data + sock->send.index, len);
        }
        else{
            ret = send(sock->fd, sdata->data + sock->send.index, len, 0);
        }
		#else
		ret = send(sock->fd, sdata->data + sock->send.index, len, 0);
		#endif
        
#if 0 /* todo:xxxxxxxxxxxx just for debug */
        do 
        {/*xxxxxxxxxxxxxxxxxxxx */
            if(0 < ret)
            {
                static FILE *fp;
                if(NULL == fp)
                {
                    fp = fopen("1.rtsp.send.txt", "wb");
                }
                fwrite(sdata->data + sock->send.index, 1, len, fp);
                fflush(fp);
            }
        } while (0);
#endif
        sock->send.index += (ret >= 0)?ret:0;

        print_log2(debug, "[tcp] send(%ld) ret[%ld].", len, ret);

        if( (ret > 0 && len > ret) || ( (0 > ret) && (netx_ewouldblock == netx_errno) ))
        { /* block */
            print_log2(debug, "[tcp] blocking len[%ld] ret[%ld].", len, ret);
            return ERR_NO_ERROR;
        }
        if( (0 == ret) || ( (0 > ret) &&  (netx_econnreset == netx_errno) ))
        { /* remote closed */
            print_log0(info, "[tcp] meet remote closed.");
            return ERR_CONN_CLOSED;
        }
        if( 0 > ret )
        { /* error */
            print_log1(err, "meet err when send() with sys errno[%d].", netx_errno); 
            return ERR_GENERIC;                         
        }
        if(sock->send.index >= sdata->len)
        { /* release the messages sent */        
            sock->send.cache_size -= ret;
			mlist_del(sock->send, sdata, in_list);
            free(sdata->data);
            free(sdata);
            sock->send.index = 0;                  
			
        }
    } 
   
    evt.events = netx_event_et | netx_event_in;
    evt.data.ptr = sock;
    if( netx_ctl(session->in_list.mod->epoll_fd, netx_ctl_mod, sock->fd, &evt) < 0 )
    {
        print_log0(err, "failed when netx_ctl().");
        return ERR_GENERIC;
    }
    return ERR_NO_ERROR;
}

long rtsp__on_tcp_recv( struct rtsp_session *session )
{  
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_tcp_recv(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    long                buf_size,recv_ret, deal_ret;
    struct rtsp_sock    *sock = session->sock;

    print_log0(detail, ".");

    if (NULL == sock->recv.buffer)
    {  
        if (NULL == (sock->recv.buffer = malloc(RTSP_BUFFERSIZE)))
        {
            print_log1(err, "failed when malloc(%d).", RTSP_BUFFERSIZE);
            return ERR_GENERIC;
        }
        sock->recv.index = 0;
        sock->recv.length = 0;
        sock->recv.size = RTSP_BUFFERSIZE;
    }

    buf_size = sock->recv.size - sock->recv.length;
	#if mrtsp_ssl_enable
    if(session->in_list.mod->ssl_enable && session->ssl)
    {
        recv_ret = SSL_read(session->ssl, sock->recv.buffer + sock->recv.length, buf_size);
    }
    else{
        recv_ret = recv(sock->fd, sock->recv.buffer + sock->recv.length, buf_size, 0);
    }
	#else
	recv_ret = recv(sock->fd, sock->recv.buffer + sock->recv.length, buf_size, 0);
	#endif
#if 0 /* todo:xxxxxxxxxxxx just for debug */
    do 
    {/*xxxxxxxxxxxxxxxxxxxx */
        if(0 < recv_ret)
        {
            static FILE *fp;
            if(NULL == fp)
            {
                fp = fopen("1.rtsp.recv.txt", "wb");
            }
            fwrite(sock->recv.buffer + sock->recv.length, 1, recv_ret, fp);
            fflush(fp);
        }
    } while (0);
#endif
    sock->recv.length += ((0 < recv_ret)?recv_ret:0);

    if((0 == recv_ret) || ((0 > recv_ret) && (netx_econnreset == netx_errno)))
    {/* sock be closed. */        
        print_log0(info, "meet the remote closing the connection.");
        return ERR_CONN_CLOSED;
    }
    else if(0 > recv_ret)
    {/* recv meet error */
        if ( netx_ewouldblock == netx_errno )
        {
            print_log0(detail, "blocking.");
            return ERR_NO_ERROR;
        }        
        print_log1(err, "failed when recv() with sys errno[%d].", netx_errno);
        return ERR_GENERIC;    
    }
    
    /* handle with the recv message */
    while(0 < sock->recv.length)
    {        
        /* distinguish the message type, rtsp message or rtp/rtcp message */
        deal_ret = rtsp__on_tcp_recv_data(session);      
        if(ERR_NO_ERROR > deal_ret)
        {
            print_log0(warn, "deal with recv data meet something error.");        
            return ERR_GENERIC;
        }  
        else if(0 < deal_ret)
        { /* the message not full, waiting for next package */
            return ERR_NO_ERROR;
        }            
        if(sock->recv.length > sock->recv.index)
        {
            sock->recv.length -= sock->recv.index; 
            memmove(sock->recv.buffer, sock->recv.buffer + sock->recv.index, sock->recv.length);
        }
        else
        {
            /* comment by kugle 2012-04-27
            memset(sock->recv.buffer, 0, sock->recv.index); */
            sock->recv.length = 0;     
        }        
        sock->recv.index = 0;            
    }
    print_log0(detail, "handle recv message succeed.");
    return ERR_NO_ERROR;  
}

long rtsp__on_rtsp_msg_data(struct rtsp_session *session)
{           
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtsp_msg_data(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    unsigned long       is_response = 0, stat;
    struct http_msg     *new_hmsg, **old_hmsg_pt = rtsp_session_get_recv_msg_pt(session);

    print_log0(detail, ".");

    is_response = rtsp__valid_response_msg(session, &stat);                 
    if(0 == is_response)
    { /* rtsp request */
        if (rtsp__on_request(session))
        {
            print_log0(warn,"the server response not { RTSP/1.0 200 OK } or deal with reply meet err.");
            return ERR_GENERIC;            
        }       
    }            
    else if(1 == is_response)
    { /* rtsp response */
        if ( (0 != rtsp__on_response(session)))
        {
            print_log0(warn, "the server response not { RTSP/1.0 200 OK } or deal with reply meet err.");
            return ERR_GENERIC;
        }                    
    }
       
    if (NULL == (new_hmsg = http_msg_create(session->hmsg, NULL)) 
        || http_msg_destroy(*old_hmsg_pt))
    {
        if(new_hmsg){ http_msg_destroy(new_hmsg); };
        print_log0(err, "meet err when create http header in rtsp_handle_recv.");
        return ERR_GENERIC;
    }    
    *old_hmsg_pt = new_hmsg;  
    return ERR_NO_ERROR;        
}

long rtsp__on_rtp_interleaved_data(struct rtsp_session *session, unsigned char *data, unsigned long len)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtp_interleaved_data(session["rtsp_session_format_s"], data[%p], len[%ld])"
#define func_format()   rtsp_session_format(session), data, len

    struct rtp_channel  *channel;
    unsigned channel_id = (unsigned int)data[1];     /*channel id code */      

    if ((channel = session->rtp_sessions.list))        /* be careful here, is '=', not '==' */
    {
        do 
        {
            if ( (channel->transport.tcp.interleaved_id.rtp == channel_id) 
                ||( channel->transport.tcp.interleaved_id.rtcp == channel_id )) 
            { 
                unsigned is_rtp = (channel->transport.tcp.interleaved_id.rtp == channel_id);
                
                print_detail("[%s] info : Interleaved %s package arrived on channel %d.datasize[%d].%s:%d\r\n",
                    mtime2s(0), is_rtp?"rtp":"rtcp", channel_id, len - 4, __FILE__, __LINE__);    
                                    
                is_rtp?rtsp__on_rtp_recv_data(channel, data + 4, len - 4):rtsp__on_rtcp_recv_data(channel, data + 4, len - 4);
                break;
            }            
            else if ( (channel = channel->in_list.next) == session->rtp_sessions.list)
            {
                print_warn("[%s] info : Interleaved package arrived unknown rtp data on channel %d.datasize[%d].%s:%d\r\n",
                    mtime2s(0),  channel_id, len - 4, __FILE__, __LINE__);   
                return 0;
            }
        } while (channel && (channel != session->rtp_sessions.list));
    }
    else
    {
        print_err("[%s] err: rtsp_on_rtp_interleaved_data() can not find the rtp session.%s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

struct rtp_channel *rtsp__rtp_channel_create( struct rtsp_session *session )
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__rtp_channel_create(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtp_channel *channel;

    print_log0(debug, ".");

    if ( ( NULL ==  (channel = (struct rtp_channel *) calloc(1, sizeof(struct rtp_channel) ) ))        
        || NULL == (channel->media_url = malloc(session->url.len + 128/*xxxxxxxxxxxx todo:modify it, or maybe overflow */)))
    {
        print_log0(err, "failed when malloc().");
        if (channel) free(channel);        
        return NULL;
    }

//	//Add by tangbo
//	channel->rtp_r_timestamp = 0;
//	channel->rtp_timestamp_pre = 0;
    rtp_channel_set_session(channel, session);   
    mlist_add(session->rtp_sessions, channel, in_list);
    return channel;
}

long rtsp__rtp_channel_destroy( struct rtp_channel *channel )
{       
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__rtp_channel_destroy(channel["rtp_channel_format_s"])"
#define func_format()   rtp_channel_format(session)

    struct rtsp_session     *session = rtp_channel_get_session(channel);
    struct rtsp_module      *mod = rtsp_session_get_mod(session);   
    struct rtsp_sock        *sock = NULL;   
  
    print_log0(debug, ".");

    if(ertt_rtp_avp_udp == channel->transport_type)
    {
        switch(session->session_type)
        {
        case erst_client_play:            
        case erst_server_record:
            {
                unsigned long step = 2;
                struct rtsp_data *sdata;
                struct netx_event evt;
                while (step--)
                {
                    sock = step?channel->transport.udp.rtp_sock:channel->transport.udp.rtcp_sock;

                    if ( NULL == sock ) continue;
                    netx_ctl(mod->epoll_fd, netx_ctl_del, sock->fd, &evt);
                    netx_close(sock->fd);
                    if (sock->recv.buffer) free(sock->recv.buffer);
                    while ((sdata = sock->send.list))
                    {
                        mlist_del(sock->send, sdata, in_list);
                        free(sdata->data);
                        free(sdata);
                    }
                    free(sock);
                }
                break;
            }      
        default:
            break;        
        }
    }  

    if (channel->media_url) free(channel->media_url);   
    if (channel->pack)  rtp_destroy_packet(channel->pack);    
    free(channel);
    return ERR_NO_ERROR;
}

void rtsp__on_rtp_video_out(struct rtsp_session *session, unsigned char *data, long len, long errind )
{       
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtp_video_out(session["rtsp_session_format_s"], data[%p], len[%ld], errind[%ld])"
#define func_format()   rtsp_session_format(session), data, len, errind

    struct rtsp_module          *mod = NULL;    
    struct rtp_channel          *channel;
    unsigned long               timestamp = 0, data_type = 0;
    struct len_str              video_format = {len_str_def_const("video/nal")};   
    unsigned long delta = 0;
    unsigned long new_timestamp = 0;
    unsigned long max_time = 0;
   
#ifndef UINT_MAX
#define UINT_MAX  0xFFFFFFFF
#endif
    print_log0(detail, ".");

    if(NULL == session)
    {
        print_log0(err, "failed with invalid param.");
        return;
    }      
    mod = rtsp_session_get_mod(session);   
    channel = session->video_channel;
 
    if ( ((data[12]&0x1F) < 28) || (((data[12]&0x1F) == 28) && (data[13]&0x40)))
        data[1] |= 0x80;       

    if(0 == rtp_decode_append(channel->pack, data, len, errind))
    {/* get a data packet */      
        new_timestamp = channel->pack->time_stamp / 90;
        max_time = UINT_MAX / 90;
        if (!session->prev_timestamp)
        {
            session->prev_timestamp = new_timestamp;
        }
        if (!session->time_stamp)
        {
            session->time_stamp = session->time_stamp;
        }
        delta = (new_timestamp >= session->prev_timestamp) ?
            (new_timestamp - session->prev_timestamp) : (max_time - session->prev_timestamp + new_timestamp);
        session->time_stamp += delta;
        session->prev_timestamp = new_timestamp;
        timestamp = session->time_stamp;
        if(NULL == session->on_media_data)
        {
            print_log0(warn, "missing on_media_data().");
        }
        else
        {
            long  ret;

            ++session->callback_times;
            mlock_simple_release(mod->lock);
            ret = session->on_media_data(session, channel->pack->data, channel->pack->len, timestamp, &video_format);
            mlock_simple_wait(mod->lock);
            --session->callback_times;
            
            if(ret)
            {
                print_log0(err, "invoke on media data failed.");      
            }
        }          
        rtp_decode_delete(channel->pack);        
    }    
}

void rtsp__on_rtp_audio_out(struct rtsp_session *session, unsigned char *data, long len)
{      
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtp_audio_out(session["rtsp_session_format_s"], data[%p], len[%ld])"
#define func_format()   rtsp_session_format(session), data, len

    struct rtsp_module          *mod = NULL;    
    struct rtp_channel          *channel = NULL;
    unsigned long               timestamp = 0, data_type = 0;
    struct len_str              audio_format = {len_str_def_const("audio")};      
    
    print_log0(detail, ".");
    if(NULL == session)
    {
        print_log0(err, "failed with invalid param.");
        return;
    }      
    return;     
}

void rtsp__on_rtp_recv_data(struct rtp_channel *channel, unsigned char *data, unsigned long len)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtp_recv_data(channel["rtp_channel_format_s"], data[%p], len[%ld])"
#define func_format()   rtp_channel_format(channel), data, len

    long                ret;
    struct rtsp_session *session = rtp_channel_get_session(channel);  
    struct rtsp_module  *mod = rtsp_session_get_mod(session);   
    struct len_str      videoformat = {len_str_def_const("video/rtp")};
    struct len_str      audioformat = {len_str_def_const("audio")};     

    print_log0(detail, ".");

    //RTP_DecodeInput(session->sort_channel, data, len);   
    if((ermt_video == channel->media_type)
       && ((unsigned int)(data[1] & 0x7F) == channel->payload_type))
    {        
        rtsp__on_rtp_video_out(session, data, len , 0);
        if(NULL == session->on_media_data)
        {
            print_log0(warn, "missing on_media_data().");
        }
        else
        {
            ++session->callback_times;
            mlock_simple_release(mod->lock);
            ret = session->on_media_data(session, data, len, ntohl(*(unsigned long *)&data[4])/90, &videoformat);
            mlock_simple_wait(mod->lock);
            --session->callback_times;

            if(ret)
            {
                print_log0(err, "invoke on media data failed.");      
            }
        }
    }
    else if((ermt_audio == channel->media_type)
            && ((unsigned int)(data[1] & 0x7F) == channel->payload_type) )
    { /* change to adts */
        /* unsigned char   *tmp_data = data + 9;
        unsigned long   tmp_len = len - 9;

        tmp_data[0] = 0xFF;
        tmp_data[1] = 0xF9;
        tmp_data[2] = 0x60;
        tmp_data[3] = 0x40;
        tmp_data[3] |= (unsigned char)((tmp_len & 0x1800) >> 11);
        tmp_data[4] = (unsigned char)((tmp_len & 0x1FF8) >> 3);    
        tmp_data[5] = (unsigned char)((tmp_len & 0x7) << 5);
        tmp_data[5] |= 0x1F;
        tmp_data[6] = 0xFC; this is fixed header, move to better encode method in rtsp channel */
        
        if(NULL == session->on_media_data)
        {
            print_log0(warn, "missing on_media_data().");
        }
        else
        {
            ++session->callback_times;
            mlock_simple_release(mod->lock);
            ret = session->on_media_data(session, (unsigned char*)data + 16/* tmp_data */, len - 16 /* tmp_len */, ntohl(*(unsigned long *)&data[4])/16, &audioformat);
            mlock_simple_wait(mod->lock);
            --session->callback_times;

            if(ret)
            {
                print_log0(tag, "invoke on media data failed.");
            }
        }
    }
    else
    {
        print_log0(err, "meet unknown media type or the rtp data not match the payload.");
        return;
    }
}

void rtsp__on_rtcp_recv_data(struct rtp_channel *channel, unsigned char *data, unsigned long len)
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__on_rtcp_recv_data(channel["rtp_channel_format_s"], data[%p], len[%ld])"
#define func_format()   rtp_channel_format(channel), data, len

    print_log0(detail, "handle the rtcp message being received.");
}

struct rtsp_session *rtsp__session_connect(struct rtsp_module *mod, struct len_str *url)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__session_connect(mod["rtsp_module_format_s"], url["len_str_format_s"])"
#define func_format()   rtsp_module_format(mod), len_str_format(url)

    struct rtsp_session     *session;
    //struct sockaddr_in      sock_addr;
    char                    host[64];
    unsigned short          port = 0;  
    struct rtsp_sock        *sock;
    struct netx_event       evt;
    
    print_log0(debug, ".");

    if ((  NULL == (session = mod->frees.list) ) 
        || (NULL == (sock = (struct rtsp_sock *)calloc(1,sizeof(struct rtsp_sock))))
        /*|| NULL == (session->sort_channel = RTP_CreateDecodeChl(RTP_STRM_RTP, 2000))*/)
    {
        print_log0(err, "failed alloc connection.");
        if (sock) free(sock);
        return NULL;
    }    
    
    /* parse rtsp url */
    if(url_parse(url->data, url->len, &session->url.desc)
       ||(0 == session->url.desc.host.len))
    {
        print_log0(err, "faile with invalid url.");
        free(sock);
        /* RTP_DeleteDecodeChl(session->sort_channel); */
        return NULL;
    }
    
    /* url for example rtsp://192.168.2.104:554/sample_100kbit.mp4 */
    host[0] = 0;
    if(session->url.desc.host.len)
    {
        unsigned long host_len = (session->url.desc.host.len>=sizeof(host))?(sizeof(host) - 1):session->url.desc.host.len;
        memcpy(host, session->url.desc.host.data, host_len);
        host[host_len] = 0;
    }
    if( session->url.desc.port.len != 0)    
        port = (unsigned short)session->url.desc.port_value;      
    else    
        port = RTSP_SERVER_DEFAULT_PORT;    
    
    /* sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = inet_addr(host);     */
    
    session->sock = sock;    
    session->in_list.mod = mod;
    //session->remote_addr = sock_addr;    
    
    if(0 >= (sock->fd = netx_connect(host, port, NULL, 0, &session->remote_addr)))
    {
        print_log2(err, "connect to {ip[%s]:port[%d]} failed.", host, port);
        free(sock);
        /* RTP_DeleteDecodeChl(session->sort_channel); */
        return NULL;
    }
    
    session->url.data = malloc(url->len + 1 );
    memcpy(session->url.data, url->data, url->len);
    session->url.data[url->len] = 0;  
    session->url.len = url->len;
    session->sock = sock; 
    rtsp_session_set_mod(session, mod);
   
    sock->type = erst_rtsp;
    sock->owner_refer = session;         
    evt.data.ptr = sock;
    evt.events = netx_event_et | netx_event_out | netx_event_in;    
    if(netx_ctl(mod->epoll_fd, netx_ctl_add, sock->fd, &evt))
    {
        print_log0(err, "failed when netx_ctl( netx_event_add).");
        free(sock);
        /* RTP_DeleteDecodeChl(session->sort_channel); */
        return NULL;
    }

    mlist_del(mod->frees, session, in_list);
    mlist_add(mod->useds, session, in_list); 

    print_log3(info, "connect to ip[%s], port[%d] ret["rtsp_session_format_s"].",
              host, port, rtsp_session_format(session));
    return session;
}

/*!
func    rtsp_session_create
\brief  connect to rtsp server, to play|record media
\param  mod [in]                the net module create by rtsp_mod_create()
\param  url [in]                the rtsp url , such as  rtsp://192.168.2.104:554/sample_100kbit.mp4
\return create result 
        #NULL                   failed
        #the pointer            succeed
*/
struct rtsp_session *rtsp_session_create(struct rtsp_module *mod, struct rtsp_session_desc *desc)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_session_create(mod["rtsp_module_format_s"], desc["rtsp_session_desc_format_s"])"
#define func_format()   rtsp_module_format(mod), rtsp_session_desc_format(desc)

    struct rtsp_session *session;
    struct url_scheme   scheme;
    struct len_str      url;
    unsigned long       tcp_priority = 0, url_malloc_here = 0;
	
    if((NULL == mod) || (NULL == desc) || (0 == desc->flag.url) || (0 == desc->url.len) || (0 == desc->url.data))
    {
        print_log0(err, "failed with invalid param.");
        return NULL;
    }
    tcp_priority = desc->url.len && (('T' == *desc->url.data) || ('t' == *desc->url.data) || ('r' == *desc->url.data) || ('R' == *desc->url.data));

    if((0 == url_parse(desc->url.data, desc->url.len, &scheme))
        && (scheme.username.len || scheme.password.len))
    {/* have user name and password */
        char            *user_pass_start = scheme.username.data;
        unsigned long   user_pass_prev_part_len = user_pass_start - desc->url.data - tcp_priority,
                        user_pass_len = scheme.username.len + (scheme.password.len?(scheme.password.len+1):0) + 1;
        url.data = (char*)malloc(desc->url.len - user_pass_len + 1);
        if(NULL == url.data)
        {
            print_log0(err, "failed when malloc url fix buffer.");
            return NULL;
        }
        url.len = desc->url.len - tcp_priority - user_pass_len;
        memcpy(url.data, desc->url.data + tcp_priority, user_pass_prev_part_len);
        memcpy(&url.data[user_pass_prev_part_len], &user_pass_start[user_pass_len], url.len - user_pass_prev_part_len);
        url.data[url.len] = 0;
        url_malloc_here = 1;
    }
    else
    {
        //url.data = desc->url.data + tcp_priority;
        //url.len  = desc->url.len - tcp_priority;
		url.data = desc->url.data; //chenyong onvif
        url.len  = desc->url.len;  //chenyong 
    }

    mlock_simple_wait(mod->lock);
    if(NULL == (session = rtsp__session_connect(mod, &url)))
    {
        mlock_simple_release(mod->lock);
        if(url_malloc_here){ free(url.data); };
        print_log0(err, "failed when rtsp__session_connect().");
        return NULL;
    }
    if(url_malloc_here){ free(url.data); url.data = NULL; url.len = 0; };

    if(desc->flag.refer)
    { 
        session->refer = desc->refer;
    }
    session->rtp_sessions.tcp_priority = tcp_priority;
    if(scheme.username.len)
    {
        len_str_dup(&session->authorzation.user, scheme.username.len, scheme.username.data);
    }
    if(scheme.password.len)
    {
        len_str_dup(&session->authorzation.password, scheme.password.len, scheme.password.data);
    }
    if(desc->flag.user && desc->user.len && desc->user.data)
    {
        len_str_dup(&session->authorzation.user, desc->user.len, desc->user.data);
    }
    if(desc->flag.password && desc->password.len && desc->password.data)
    {
        len_str_dup(&session->authorzation.password, desc->password.len, desc->password.data);
    }

    session->machine_state = ersms_state_init;
    session->cseq = 0;        
    session->on_req = NULL;
    session->tick = mod->now_tick;
    session->prev_set_tick = mod->now_tick;//chenyong add

    if(0 == len_str_casecmp_const(&desc->type, "record"))
    {
        session->session_type = erst_client_record;                  
        session->on_close = mod->params.events.client_record_close;
        session->on_media_data = NULL;   
        session->on_ctrl = mod->params.events.client_record_ctrl;
    }
    else
    {
        session->session_type = erst_client_play;       /*!< notify that it's a client play request */
        session->on_close = mod->params.events.client_play_close;
        session->on_media_data = mod->params.events.client_play_data;
        session->on_ctrl = NULL;
    }

    //if(rtsp__req_options(session))
    if(rtsp__req_describe(session))    //chenyong
    {        
        print_log0(err, "failed when rtsp__req_options().");
        rtsp__close_session(session);
        mlock_simple_release(mod->lock);
        return NULL;
    }
    mlock_simple_release(mod->lock);

    print_log1(info, "ret["rtsp_session_format_s"].", rtsp_session_format(session));
    return session;
}

#if 0
/*!
func    rtsp_client_play
\brief  connect to rtsp server, to play media
\param  mod [in]                the net module create by rtsp_mod_create()
\param  url [in]                the rtsp url, such as rtsp://192.168.2.104:554/sample_100kbit.mp4
\return create result 
        #NULL                   failed
        #the pointer            succeed
*/
struct rtsp_session *rtsp_client_play (struct rtsp_module *mod, char *url)
{
    struct rtsp_session *session;    

    if ( (NULL == mod) || (NULL == url) )
    {
        print_err("[%s] err: rtsp_client_play(mod[%p], url[%p]) meet invalid parameters. %s:%d\r\n",
            mtime2s(0), mod, url, __FILE__, __LINE__);
        return NULL;
    }
    mlock_simple_wait(mod->lock);
    if ( NULL == (session = rtsp_session_connect (mod, url)))
    {
        mlock_simple_release(mod->lock);
        print_err("[%s] err: rtsp_client_play() meet invalid parameter.%s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        return NULL;
    }

    session->machine_state = ersms_state_init;
    session->session_type = erst_client_play;       /*!< notify that it's a client play request */
    session->on_close = mod->params.events.client_play_close;
    session->on_media_data = mod->params.events.client_play_data;
    session->on_idr = NULL;
    session->on_req = NULL;   
    session->cseq = 0;        
    session->tick = mod->now_tick;
    
    if (rtsp_req_options(session))
    {        
        print_err("[%s] err: rtsp_client_play() invoke meet err .%s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        rtsp_close_session(session);
        mlock_simple_release(mod->lock);
        return NULL;
    }
    mlock_simple_release(mod->lock);
    return session;    
}

struct rtsp_session *rtsp_client_record (struct rtsp_module *mod, char *url )
{
    struct rtsp_session *session;

    if ( (NULL == mod) || (NULL == url) )
    {
        print_err("[%s] err: rtsp_client_record(mod[%p], url[%p]) meet invalid parameters. %s:%d\r\n",
            mtime2s(0), mod, url, __FILE__, __LINE__);
        return NULL;
    }

    mlock_simple_wait(mod->lock);
    if ( NULL == (session = rtsp_session_connect (mod, url)))
    {
        mlock_simple_release(mod->lock);
        print_err("[%s] err: invalid parameter ,when rtsp_cli_play %s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        return NULL;
    }

    session->machine_state = ersms_state_init;
    session->session_type = erst_client_record;                  
    session->cseq = 0;     
    session->on_close = mod->params.events.client_record_close;
    session->on_req = NULL;
    session->on_media_data = NULL;   
    session->on_idr = mod->params.events.client_record_idr;
    session->tick = mod->now_tick;

    if (rtsp_req_options(session))
    {
        print_err("[%s] err: rtsp_client_play() invoke meet err .%s:%d\r\n",
            mtime2s(0), __FILE__, __LINE__);
        rtsp_close_session(session);
        mlock_simple_release(mod->lock);        
        return  NULL;
    }
    mlock_simple_release(mod->lock);
    return session;
}
#endif

long rtsp__close_session(struct rtsp_session *session)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp__close_session(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_session     *ref_session;
    struct rtsp_data        *sdata = NULL;
    struct rtsp_module      *mod = rtsp_session_get_mod(session);
    struct rtsp_sock        *sock = session->sock;
    struct rtp_channel      *channel;
    long                    ret = 0;    
    long                    i = 0;      

	//origin code
    if(session->callback_times)
    {
        print_log0(err, "still in callback.");
        return -1;
    }
	
    if((ref_session = session->x_session.ref))
    {/* break session refer */
        ref_session->x_session.ref = NULL;
        rtsp__close_session(ref_session);
        session->x_session.ref = NULL;
    }

    if (session->refer )
    {
        struct rtsp_req_params  params;
        params.full_url.data = session->url.data;
        params.full_url.len  = session->url.len; 
        if(session->on_close 
           && (rtsp_session_close_reason_by_user != session->close_reason))
        {
            ++session->callback_times;
            mlock_simple_release(mod->lock);
            session->on_close(session, &params); 
            mlock_simple_wait(mod->lock);
            --session->callback_times;

            session->refer = NULL;          
        }
    }

    if ( sock )
    {
        struct netx_event evt = {0};  
        while ((sdata = sock->send.list))
        {
            mlist_del(sock->send, sdata, in_list);
            free(sdata->data);
            free(sdata);
        }     
        if (sock->recv.buffer) free(sock->recv.buffer);                       
        if(0 < sock->fd)
        {
            if ( 0 != netx_ctl(mod->epoll_fd, netx_ctl_del, sock->fd, &evt))
            {
                print_log0(err, "del rtsp session from epoll polling meet err.");
            }          
            netx_close(sock->fd);        
            sock->fd = -1;
        }
        free(sock);
    }        

	#if mrtsp_ssl_enable
    if(session->ssl){
        SSL_free(session->ssl);
		session->ssl = NULL;
    }
	#endif

	while(session->audio_sample_buff.list){
		struct sample_buff_node *item = session->audio_sample_buff.list;
		if(item->data){
			free(item->data);
			item->data = NULL;
		}

		mlist_del(session->audio_sample_buff, item, node);
	}

    /* recovery memory when close rtsp session */
    if ( session->hmsg )
    {
        print_log0(detail, "free the http_header for parse the recv message.");
        http_msg_destroy(session->hmsg);
        session->hmsg = NULL;       
    }         
    
    /* url or sdp information */
    if ( session->url.data )   {free(session->url.data); session->url.data = NULL;};
    memset(&session->url, 0, sizeof(session->url));
    if ( session->sdp.data)
    {
        free(session->sdp.data);
        session->sdp.data = 0; 
        sdp_destroy(session->sdp.desc);
    }    
    memset(&session->sdp, 0, sizeof(session->sdp));
    /* close the rtp_sessions which created in rtsp_do_setup method */ 
    while (( channel = session->rtp_sessions.list ))
    {
        mlist_del(session->rtp_sessions, channel,in_list);
        rtsp__rtp_channel_destroy( channel);
        session->video_channel = session->audio_channel = NULL;
    }                 
    /* destroy rtp decode channel */
    /*
    if ( session->sort_channel)
    {  
        RTP_DeleteDecodeChl(session->sort_channel);
        session->sort_channel = NULL;
    } */     
    
    len_str_clear(&session->x_session.cookie);
    len_str_clear(&session->authorzation.user);
    len_str_clear(&session->authorzation.password);
    mlist_del(mod->useds, session, in_list);
    /*
    memset(session, 0, sizeof(struct rtsp_session));
    rtsp_session_set_mod(session, mod);
    */
    mlist_add(session->in_list.mod->frees, session, in_list);

    return ERR_NO_ERROR;
}

long rtsp_session_destroy(struct rtsp_session *session)
{  
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_session_destroy(session["rtsp_session_format_s"])"
#define func_format()   rtsp_session_format(session)

    struct rtsp_module *mod = NULL;
    if((NULL == session)
        || (NULL == (mod  = rtsp_session_get_mod(session))))
    {
        print_log0(err, "failed with invalid param.");
        return -1;
    }
    print_log0(debug, ".");

    mlock_simple_wait(mod->lock);
    session->close_reason  = rtsp_session_close_reason_by_user;
    if(session->callback_times)
    {
        print_log0(warn, "session try close in callbacking.");
    }
    else
    {
        rtsp__close_session(session);
    }
    mlock_simple_release(mod->lock);    
    return 0;
}

/*!
func    rtsp_session_send_media_data
\brief  send media data, only for client record and server play
\param  session[in]             rtsp session  
\param  data[in]                the media data need to be sent
\param  length[in]              media data size
\param  timestamp[in]           data timestamp
\param  data_format[in]         video or audio , create it by len_str_def_const ("video") and vice versa       
\return send result 
        # 0                     succeed
        # other                 error code
*/
long rtsp_session_send_media_data( struct rtsp_session *session, unsigned char *data, unsigned long length, unsigned long timestamp, struct len_str  *format, int has_video_is)
{    
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_session_send_media_data(session["rtsp_session_format_s"], data[%p], length[%ld], timestamp[%ld], format["len_str_format_s"], has_video_is[%ld])"
#define func_format()   rtsp_session_format(session), data, length, timestamp, len_str_format(format), has_video_is

    struct rtp_channel      *channel;
    struct rtp_packet       *video_pack = NULL;
    struct rtp_data         *rtp_data;       
    unsigned long           seq, rtp_timestamp, payload_type, ssrc, mtu; 
    struct rtsp_module      *mod = rtsp_session_get_mod(session);
    struct rtsp_sock        *sock = mod->rtp_sock;
	long long jitter = 0;

    if((NULL == session) || (NULL == mod) || (NULL == data) || (0 == length) || (NULL == format) || (0 == format->len))
    {
        print_log0(err, "failed with invalid param.");
        return -1;
		
    }

    print_log0(detail, ".");

    mlock_simple_wait(mod->lock);      
    if ( (ersms_state_playing != session->machine_state) && (ersms_state_recording != session->machine_state))
    {             
        print_log0(warn, "is not playing or recording state , so not to send any media data.");
        mlock_simple_release(mod->lock);  
        return 0;
    }  
   
    channel = (0 == len_str_cmp_const(format, "video")) ? session->video_channel : session->audio_channel; 
    if ( NULL == channel ) 
    {
        mlock_simple_release(mod->lock);   
        print_log0(warn, "meet unknown medie channel, it haven't create the media session, maybe because of the wrong sps pps data.");
        return 0;
    }  

    session->tick = mod->now_tick;
    mlist_move_to_last(mod->useds, session, in_list);

    payload_type = channel->payload_type;  
    seq = channel->seq;
    ssrc = channel->ssrc;
    mtu = 1012;     

	/* buffer the audio sample */
    if (ermt_audio == channel->media_type)
    {
		struct sample_buff_node *item;
		unsigned char *temp;
		/* the buffer's length is no more than 20 */
		if(session->audio_sample_buff.counts >= 20){
			item = session->audio_sample_buff.list;
			jitter = (long long)(item->timestamp - session->rtp_prior_audio_timestamp) - 64;
			if(jitter > 64)
				jitter = 64;
			else if(jitter < -64)
				jitter = -64;
			session->rtp_fiducial_audio_timestamp += jitter;
			session->rtp_prior_audio_timestamp = item->timestamp;
			mlist_del(session->audio_sample_buff, item, node);
			free(item->data);
			free(item);
			item = NULL;
		}
		
		if((item = (struct sample_buff_node *)malloc(sizeof(struct sample_buff_node)))
			&& (temp = malloc(length))){
			memcpy(temp, data, length);
			item->data = temp;
			item->len = length;
			item->timestamp = timestamp;
			item->node.prev = NULL;
			item->node.next = NULL;
			mlist_add(session->audio_sample_buff, item, node);
		}
		else{
			if(item != NULL){
				free(item);
				item = NULL;
			}

			print_log0(err, "alloc memery failed");
		}
		
		session->rtp_audio_ssrc = ssrc;
    }
    else if ( ermt_video == channel->media_type)
    {
		unsigned char   temp[2000];
        switch(data[4] & 0x1f)
        {
        case 5:
        case 7:
        case 8:
            {
                print_log0(debug, "key-data.");
                break;
            }
        }

		if(session->rtp_video_ssrc != ssrc){
			session->rtp_video_count = 0;
			session->rtp_video_octet = 0;
		}
		
        /* is rtp */
        if ( 0x80 == data[0] )
        {        
            unsigned char   m = data[1];
            unsigned long   mark = ((unsigned char )(m & (unsigned char)0x80) == (unsigned char)0x80)? 1:0; 
            
            if ((7 == (data[12] & 0x1F )) || (8 == (data[12] & 0x1f)))
            {
                print_log0(warn, "ignore sps|pps.");
                return 0;
            }

            seq = ntohs(*(unsigned short *)&data[2]);
            rtp_timestamp = ntohl(*(unsigned long *)&data[4]);
			session->last_video_sample_time = timestamp;					//this timestamp may not be the sample time
            rtp_create_packet_header(temp, (unsigned char)payload_type, rtp_timestamp, ssrc, mark, (unsigned short)seq);
            memcpy(temp + 12, data + 12, length - 12 ); 
            rtsp__add_rtp_package(channel, temp, length, erst_rtp);
			session->rtp_video_octet += length -12;
			/* Send a sr type rtcp every 25 video pack */
			if(mark == 1 && session->rtp_video_count % (session->rtp_video_count < 6 ? 2 : 25) == 0){
				rtcp_create_packet_header(temp, 200, rtp_timestamp, timestamp, 1, ssrc, session->rtp_video_count + 1, session->rtp_video_octet);
				rtsp__add_rtp_package(channel, temp, 28, erst_rtcp);	
			}

			session->rtp_video_count++;
//            mlock_simple_release(mod->lock);   
//            return 0;
        }    
        else{
	        /* is nal */
			rtp_timestamp = timestamp * 90;
	        if(NULL == (video_pack = rtp_encode_create_packet( (unsigned short) seq, rtp_timestamp,(unsigned char) payload_type, ssrc, mtu,data, length)))
	        {/* create packet failed */
	            mlock_simple_release(mod->lock);  
	            print_log0(err, "meet err when invoke rtp_get_video_package.");
	            return -1;
	        }     
	        channel->seq += video_pack->subs.counts;
	        channel->rtptime = rtp_timestamp;
			session->last_video_sample_time=timestamp;
	        if (( rtp_data = video_pack->subs.list ))
	        {
	            do 
	            {					
	                rtsp__add_rtp_package(channel, rtp_data->data, rtp_data->len, erst_rtp);
					session->rtp_video_octet += rtp_data->len - 12;
					/* Send a sr type rtcp every 25 video pack */
					if(rtp_data->data[1]&0x80 && session->rtp_video_count % (session->rtp_video_count < 5 ? 1 : 25) == 0){
						rtcp_create_packet_header(temp, 200, rtp_timestamp, timestamp, 1, ssrc, session->rtp_video_count + 1, 0);
						rtsp__add_rtp_package(channel, temp, 28, erst_rtcp);
					}

					session->rtp_video_count++;
	                rtp_data = rtp_data->out_list.next;					
	            } while ( rtp_data != video_pack->subs.list );       
	        }
			
	        if(rtp_destroy_packet( video_pack ))
	        {
	            mlock_simple_release(mod->lock);   
	            print_log0(err, "rtsp_send_media_data() meet err when destroy rtp_rtp_video_package.");
	            return -1;
	        }
			
//	        mlock_simple_release(mod->lock); 
//	        return 0;
        }
    }

	if(ermt_audio == channel->media_type || ermt_video == channel->media_type){
		channel = session->audio_channel;
	    if ( NULL == channel ) 
	    {
	        mlock_simple_release(mod->lock);   
	       	return 0;
	    }
		
		payload_type = channel->payload_type;
		ssrc = channel->ssrc;
		while(session->audio_sample_buff.counts){
			unsigned char   temp[500];               
	        unsigned long   mark = 1;    
	        long            tmp_len;
			struct timeval tv;
			long long timeDelta;			
			
			seq=channel->seq;
			struct sample_buff_node *item=session->audio_sample_buff.list;
			/* if the channel has video istream, then the sample time of audio should in a range base on sample time of last sended video */
			/* it makes sure the audio wouldn't precede video to much */
			if(has_video_is){
				if(session->last_video_sample_time - item->timestamp >= 100000000ul){
					if(item->timestamp - session->last_video_sample_time >= 120){
						//audio is ahead the video too much, buff the audio
						break;
					}
				}
				else if(session->last_video_sample_time - item->timestamp >= 120){
					//video is ahead the audio too much, drop the audio
					printf("video_time:%10lu\taudio_time:%10lu\tdelta:%10lu\tdrop\tlen:%10lu\n", 
						session->last_video_sample_time, 
						item->timestamp, 
						session->last_video_sample_time - item->timestamp, 
						session->audio_sample_buff.counts - 1);
					item = session->audio_sample_buff.list;
					jitter = (long long)(item->timestamp - session->rtp_prior_audio_timestamp) - 64;
					if(jitter > 64)
						jitter = 64;
					else if(jitter < -64)
						jitter = -64;					
					session->rtp_fiducial_audio_timestamp += jitter;					
					session->rtp_prior_audio_timestamp = item->timestamp;
					mlist_del(session->audio_sample_buff, item, node);
					free(item->data);
					free(item);
					item = NULL;
					continue;				
				}
			}

			gettimeofday(&tv, NULL);
			int latency = 0;
			jitter = (long long)(item->timestamp - session->rtp_prior_audio_timestamp) - 64;
			if(jitter > 64)
				jitter = 64;
			else if(jitter < -64)
				jitter = -64;			
			session->rtp_fiducial_audio_timestamp += jitter;
			timeDelta = ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000) - ((long long)session->rtp_fiducial_audio_time.tv_sec * 1000 + session->rtp_fiducial_audio_time.tv_usec / 1000) & 0xFFFFFFFF;
			if((latency = timeDelta - (item->timestamp - session->rtp_fiducial_audio_timestamp)) > session->rtp_allowed_latency){
				//the latency of audio is two high, so drop the audio
				printf("The latency[%5d] of audio beyond %3d, so drop the audio\n", latency, session->rtp_allowed_latency);
				session->rtp_prior_audio_timestamp = item->timestamp;
				mlist_del(session->audio_sample_buff, item, node);
				free(item->data);
				free(item);
				item = NULL;
				continue;	
			}else if(latency < 0){
				//This issue:
				//sample time		: 	00 | 40 | 80  | 120 | 160
				//server time		: 	20 | 60 | 70  | 110 | 180
				//relate latency 	:	   |  0	|-30  | -30 | 0
				//but the real latency between 4th and 5th is 30
				//also,
				//sample time		: 	00 | 40 | 110 | 150 | 190
				//server time		: 	20 | 60 | 100 | 140 | 210
				//relate latency 	:	   |  0	| -30 | -30 | 0
				//but the real latency between 4th and 5th is 70
				//so, we need to adjust the base sample time and server time
				printf("The latency[%5d] of audio is less than 0, so adjust the factors\n", latency);
				session->rtp_fiducial_audio_time = tv;
				session->rtp_fiducial_audio_timestamp = item->timestamp;
			}

			session->rtp_prior_audio_timestamp = item->timestamp;
			rtp_timestamp = item->timestamp * 16;
	        rtp_create_packet_header(temp, payload_type, rtp_timestamp, ssrc, mark, (unsigned short)seq);
	        
	        tmp_len = item->len - 7;
			if(ssrc != session->rtp_audio_ssrc){
				session->rtp_audio_ssrc = ssrc;
				session->rtp_audio_count = 0;
				session->rtp_audio_octet = 0;
			}

			session->rtp_audio_octet += tmp_len;
	        temp[12] = 0;
	        temp[13] = 16;
	        temp[14] = (tmp_len >> 5) & 0xFF;
	        temp[15] = (tmp_len << 3) & 0xFF;
			
	        memcpy(temp + 16, item->data + 7, tmp_len);		
	        
	        tmp_len = tmp_len + 16;      
	        rtsp__add_rtp_package(channel, temp, tmp_len, erst_rtp);
			/* Send a sr type rtcp every 25 audio frame */
			if(session->rtp_audio_count % (session->rtp_audio_count < 5 ? 1 : 25) == 0){
				rtcp_create_packet_header(temp, 200, rtp_timestamp, item->timestamp, 1, ssrc, session->rtp_audio_count + 1, session->rtp_audio_octet);
				rtsp__add_rtp_package(channel, temp, 28, erst_rtcp);
			}
			
	        channel->seq++;
			mlist_del(session->audio_sample_buff, item, node);
			free(item->data);
			free(item);
			item=NULL;
			session->rtp_audio_count++;
		}		
	}
    mlock_simple_release(mod->lock);
    return 0;
}

/*!
func    rtsp_session_update
\brief  set session data, such as user, password, refer, sdp
\param  session[in]             the session
\param  data[in]                the datas
\return create result 
        #NULL                   failed
        #the pointer            succeed
*/
long rtsp_session_update(struct rtsp_session *session, struct rtsp_session_desc *desc)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_session_update(session["rtsp_session_format_s"], desc[%p])"
#define func_format()   rtsp_session_format(session), desc

    long                ret = 0;
    struct rtsp_module  *mod;
    if((NULL == session) || (NULL == (mod = rtsp_session_get_mod(session))) || (NULL == desc))
    {
        print_log0(err, "invalid param.");
        return -1;
    }

    print_log0(debug, ".");
    mlock_simple_wait(mod->lock);
    /* set sdp */
    if(desc->flag.sdp && rtsp__set_sdp(session, desc->sdp.data, desc->sdp.len))
    {
        print_log0(err, "failed when rtsp__set_sdp().");
        ret = -1;
    }
    mlock_simple_release(mod->lock);
    return ret;
}

/*!
func    rtsp_session_ctrl
\brief  ctrl, such as idr, mbw
\param  session[in]             rtsp session  
\return invoke result 
        #0                      succeed
        #other                  error code
*/
long rtsp_session_ctrl(struct rtsp_session *session, struct len_str *method, struct len_str *params)
{
#undef  func_format_s
#undef  func_format
#define func_format_s   "rtsp_session_ctrl(session["rtsp_session_format_s"], method["len_str_format_s"], params["len_str_format_s"])"
#define func_format()   rtsp_session_format(session), len_str_format(method), len_str_format(params)

    struct rtsp_module  *mod;

    if((NULL == session) || (NULL == (mod = rtsp_session_get_mod(session)))
        || (NULL == method) || (0 == method->len))
    {
        print_log0(err, "invalid param.");
        return -1;
    }

    print_log0(debug, ".");
    mlock_simple_wait(mod->lock);
    if(session->machine_state != ersms_state_init)
    {
        rtsp__req_ctrl(session, method, params);
    }
    mlock_simple_release(mod->lock);
    return 0;
}







