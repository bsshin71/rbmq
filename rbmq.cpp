/*
 * mqlib.cpp
 *
 *  Created on: 2018. 9. 29.
 *      Author: omegaman
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include <pthread.h>

#include "rbmq.h"

#include "kxtracelog.h"
#include "kxlib.h"

//#define USE_QUEUESIZE //에러나면 주석처리
#ifdef USE_QUEUESIZE
#include "amqp_table.h"
#endif

const char *CRbMq::m_EXCHANGE_TYPE_STR[4] = { "direct", "fanout", "topic", "headers" };

////////////////////////////////////////////////////////////////////////////////////////////
// Rabbit MQ Base Wrapper Class Implement
//
CRbMq::CRbMq()
{
    Init();
}

CRbMq::~CRbMq()
{
    Destroy();
}

void CRbMq::Init()
{
    m_thr_id = 0;
    //외부변수 pointing 말고 자체할당사용
    m_error_string = NULL;
    m_hostname = NULL;
    m_exchange_name = NULL;
    m_exchange_type = EXCHANGE_TYPE::none;
    m_routingkey = NULL;
    m_queuename = NULL;
    m_username = NULL;
    m_password = NULL;
    m_vhost = NULL;
    m_port = 5672;
    m_frame_max = 131072;
    m_channel_max = 0;
    m_autodelete = 0;
    m_queuesize = -1;
    m_socket = NULL;
    m_conn = NULL;
    //TODO::아래는 포인트 형이면 초기값 찾아서 NULL셋팅
    //m_conn;
    //m_props;        // default message properties
    //m_queuehandle;  // queue handle
}

void CRbMq::Destroy()
{
    if( m_conn != NULL ) {
        amqp_destroy_connection(m_conn);
        m_conn = NULL;
    }
    // just useless thing..

    if (m_error_string) free(this->m_error_string);
    if (m_hostname)       free(m_hostname);
    if (m_exchange_name) free(m_exchange_name);
    if (m_routingkey) free(m_routingkey);
    if (m_queuename) free(m_queuename);
    if (m_username) free(m_username);
    if (m_password) free(m_password);
    if (m_vhost) free(m_vhost);
    Init();
}

bool CRbMq::isConnected() {
    //if (m_socket == NULL) return false;
    if (m_conn == NULL) return false;
    return true;
}

char *CRbMq::GetHost()             { return m_hostname; }
char *CRbMq::GetExchangeName()     { return m_exchange_name; }
EXCHANGE_TYPE   CRbMq::GetExchangeType()     { return m_exchange_type; }
char *CRbMq::GetRoutingKeyName()   { return m_routingkey; }
char *CRbMq::GetQueueName()        { return m_queuename; }
char *CRbMq::GetUserName()         { return m_username; }
char *CRbMq::GetPassword()         { return m_password; }
char *CRbMq::GetVirtualHost()      { return m_vhost; }
int   CRbMq::GetPort()             { return m_port; }
int   CRbMq::GetFrameMax()         { return m_frame_max; }
int   CRbMq::GetChannelMax()       { return m_channel_max; }
int   CRbMq::GetAutoDelete()       { return m_autodelete; }
int   CRbMq::GetQueueSize()        { return m_queuesize; }

void CRbMq::SetAutoDelete(int autodelete)        { m_autodelete = autodelete; }
void CRbMq::SetQueueSize(int queuesize)        { m_queuesize = queuesize; }

bool CRbMq::IsError(amqp_rpc_reply_t x)
{
    char msgbuff[4096] = { 0, };
    char *msg;

    switch (x.reply_type)
    {
    case AMQP_RESPONSE_NORMAL:
        this->SetErrMsg("");
        return false;
    case AMQP_RESPONSE_NONE:
        this->SetErrMsg("Missing RPC reply type");
        return true;
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        strcpy(msgbuff, amqp_error_string2(x.library_error));
        this->SetErrMsg(msgbuff);
        return true;
    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id)
        {
        case AMQP_CONNECTION_CLOSE_METHOD:
        {
                                             amqp_connection_close_t *m = (amqp_connection_close_t *)x.reply.decoded;
                                             snprintf(msgbuff, (unsigned int)m->reply_text.len
                                                 , "server connection error msg: %s"
                                                 , (char *)m->reply_text.bytes);
                                             this->SetErrMsg(msgbuff);
                                             return true;
        }
        case AMQP_CHANNEL_CLOSE_METHOD:
        {
                                          amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
                                          snprintf(msgbuff, (unsigned int)m->reply_text.len
                                              , "server channel error msg: %s "
                                              , (char *)m->reply_text.bytes);
                                          this->SetErrMsg(msgbuff);
                                          return true;
        }
        default:
            sprintf(msgbuff, "unknown server error, method id 0x%08X", x.reply.id);
            this->SetErrMsg(msgbuff);
            return true;
        }
        break;
    }
    return false;
}

// Connect to MQ
bool CRbMq::OpenConnect()
{
    int status;
    amqp_rpc_reply_t ret;

    m_thr_id = (int) pthread_self();//thread safe check!

    m_conn = amqp_new_connection();
    m_socket = amqp_tcp_socket_new(m_conn);
    if (!m_socket) {
        this->SetErrMsg("creating TCP socket error ");
        return false;
    }

    status = amqp_socket_open(m_socket, this->m_hostname, this->m_port);
    if (status) {
        this->SetErrMsg("opening TCP socket error ");
        return false;
    }

    ret = amqp_login(m_conn,              /* connection */
        m_vhost,       /* virtual host */
        m_channel_max, /* channel_max */
        m_frame_max,            /* frame_max */
        0,                 /* heartbeat */
        AMQP_SASL_METHOD_PLAIN,
        m_username,   /* user */
        m_password);  /* password... */
    if (IsError(ret)) {
        return false;
    }
    return true;
}

void CRbMq::SetErrMsg(const char* msg)
{
    if (this->m_error_string) free(this->m_error_string);
    this->m_error_string = NULL;
    this->m_error_string = strdup(msg);
}

void CRbMq::Create(char* host
    , char* username
    , char* password
    , char* vhost
    , unsigned int port
    , int frame_max
    , int channel_max)
{
    if (m_hostname){ free(m_hostname); }  m_hostname = strdup(host);
    if (m_username){ free(m_username); }  m_username = strdup(username);
    if (m_password){ free(m_password); }  m_password = strdup(password);
    if (m_vhost)   { free(m_vhost);    }  m_vhost = strdup(vhost);
    m_port = port;
    m_frame_max = frame_max;
    m_channel_max = channel_max;
}

char* CRbMq::GetLastErrMsg()
{
    return m_error_string;
}

bool CRbMq::OpenChannel()
{
    amqp_rpc_reply_t ret;

    amqp_channel_open(m_conn, 1);
    ret = amqp_get_rpc_reply(m_conn);

    if (IsError(ret)) {
        return false;
    }

    return true;
}

bool CRbMq::CloseChannel()
{
    amqp_rpc_reply_t ret;

    amqp_channel_close(m_conn, 1, AMQP_REPLY_SUCCESS);
    ret = amqp_get_rpc_reply(m_conn);

    if (IsError(ret)) {
        return false;
    }

    return true;
}

bool CRbMq::CloseConnect()
{
    amqp_rpc_reply_t ret;

    amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
    ret = amqp_get_rpc_reply(m_conn);

    if (IsError(ret)) {
        return false;
    }

    return true;
}

bool CRbMq::OpenAll()
{
    if (!OpenConnect()) return false;
    if (!OpenChannel()) return false;

    return true;
}

bool CRbMq::CloseAll()
{
    if (!CloseChannel()) return false;
    if (!CloseConnect()) return false;

    return true;
}

char* CRbMq::GetExchangeTypeStr(EXCHANGE_TYPE exchange_type)
{

    return (char*) this->m_EXCHANGE_TYPE_STR[exchange_type];
}

EXCHANGE_TYPE CRbMq::GetExchangeType(char *exchange_type_str)
{
    char tempStr[128] = { 0, };
    strncpy(tempStr, exchange_type_str, sizeof(tempStr)-1);
    kxTrim(tempStr);

    //속도업 if if => if else if
    if (!strcasecmp("direct", tempStr)) m_exchange_type = direct;
    else if (!strcasecmp("fanout", tempStr)) m_exchange_type = fanout;
    else if (!strcasecmp("topic", tempStr)) m_exchange_type = topic;
    else if (!strcasecmp("headers", tempStr)) m_exchange_type = headers;
    return m_exchange_type;
}

// Producer 에서 Exchange 설정을 한다.
bool CRbMq::DeclareExchange() /* durable */
{
    int iDurable = 0;
    amqp_rpc_reply_t ret;
    char             *exchange_type_str;

    exchange_type_str = GetExchangeTypeStr(m_exchange_type);
    amqp_exchange_declare(m_conn,                            // connection state
        1,                                // the channel to the RPC on
        amqp_cstring_bytes(m_exchange_name),  // exchange name
        // exchnage type (fanout, direct, topic, headers )
        amqp_cstring_bytes(exchange_type_str),
        0,                                // passive
        1,                             // durable
        0,                               // auto_delete
        0,                               // internal
        amqp_empty_table                 // arguments
        );

    ret = amqp_get_rpc_reply(m_conn);
    if (IsError(ret)) {
        return false;
    }
    return true;
}

void CRbMq::SetQueue(char *exchange, char *queuename, char *routingkey)
{
    if (m_exchange_name){free(this->m_exchange_name); }  m_exchange_name = strdup(exchange);
    if (m_queuename)    {free(this->m_queuename);     }  m_queuename = strdup(queuename);
    if (m_routingkey)   {free(this->m_routingkey);    }  m_routingkey = strdup(routingkey);
}

bool CRbMq::CreateQueue( char *exchange, char *queuename, char *routing_key)
{
    if ( ! DeclareQueue(queuename, routing_key ) ) return false;
    if ( ! BindQueue(exchange, queuename, routing_key )    ) return false;

    return true;
}

bool CRbMq::DeclareQueue()
{
    return DeclareQueue(m_queuename ,m_routingkey );
}

bool CRbMq::DeclareQueue(char *queuename, char *routingkey)
{
    amqp_rpc_reply_t ret;
    amqp_bytes_t     queuename_bytes;
    amqp_bytes_t     routingkey_bytes;

    queuename_bytes = (queuename == NULL || queuename[0] == 0x00) ? amqp_empty_bytes : amqp_cstring_bytes(queuename);
    routingkey_bytes = (routingkey == NULL || routingkey[0] == 0x00) ? amqp_empty_bytes : amqp_cstring_bytes(routingkey);

    amqp_table_t props;
    amqp_table_entry_t sub_base_entries[1];

#ifdef USE_QUEUESIZE
    //사용안할경우 m_queuesize = -1 아니면 갯수설정
    if (m_queuesize != -1){
        char szqueuesize[30];
        sprintf(szqueuesize, "%d", m_queuesize);

        sub_base_entries[0] = amqp_table_construct_utf8_entry("x-max-length",szqueuesize);
        props.num_entries   =  sizeof(sub_base_entries) / sizeof(amqp_table_entry_t);
        props.entries = sub_base_entries;
    }
#endif

    amqp_queue_declare_ok_t *r = amqp_queue_declare(
        m_conn,
        1,                // the channel to do the RPC on
        queuename_bytes,  // empty_bytes 일 경우는  use system-generated queue name
        0,                // passive
        1,                // durable
        0,                // exclusive
        m_autodelete,     // auto_delete
        (m_queuesize == -1) ? amqp_empty_table:props);// arguments
    ret = amqp_get_rpc_reply(m_conn);
    if (IsError(ret)) {
        this->SetErrMsg("amqp_queue_declare error ");
        return false;
    }

    this->m_queuehandle = amqp_bytes_malloc_dup(r->queue);
    if (m_queuehandle.bytes == NULL) {
        this->SetErrMsg("Out of memory while copying queue name");
        return false;
    }

    return true;
}

bool CRbMq::BindQueue()
{
    return BindQueue(m_exchange_name ,m_queuename,m_routingkey );
}

bool CRbMq::BindQueue(char *exchange, char *queuename, char *routingkey )
{
    amqp_rpc_reply_t ret;
    amqp_bytes_t     queuename_bytes;
    amqp_bytes_t     routingkey_bytes;

    queuename_bytes  = ( queuename == NULL || queuename[0] == 0x00  ) ? amqp_empty_bytes : amqp_cstring_bytes( queuename );
    routingkey_bytes = ( routingkey == NULL || routingkey[0] == 0x00 ) ? amqp_empty_bytes : amqp_cstring_bytes( routingkey );

    // binding queue to the exchange with routing key
    amqp_queue_bind(m_conn,
        1,                  // the channel to do the RPC on
        m_queuehandle,  // queue handle
        amqp_cstring_bytes(exchange),
        routingkey_bytes,   // routing_key
        amqp_empty_table);  // arguments
    ret = amqp_get_rpc_reply(m_conn);
    if (IsError(ret)) {
        this->SetErrMsg("amqp_queue_bind error ");
        return false;
    }
    return true;
}

bool CRbMq::ReadytoReceive()
{
    amqp_rpc_reply_t ret;

    amqp_basic_consume(m_conn,
        1,                 //the channel to do the RPC on
        m_queuehandle,
        amqp_empty_bytes,  // consumer_tag
        0,                 // no_local
        1,                 // no_ack
        0,                 // exclusive
        amqp_empty_table);  // arguments

    ret = amqp_get_rpc_reply(m_conn);
    if (IsError(ret)) {
        this->SetErrMsg("amqp_basic_consume error ");
        return false;
    }
    return true;
}

void CRbMq::SetExchange(char* exchange, EXCHANGE_TYPE exchange_type)
{
    if (m_exchange_name) free(m_exchange_name);
    m_exchange_name = strdup(exchange);
    m_exchange_type = exchange_type;
}

void CRbMq::SetExchange(char* exchange, char  *exchange_type)
{
    if (m_exchange_name) free(m_exchange_name);
    m_exchange_name = strdup(exchange);
    m_exchange_type = GetExchangeType(exchange_type);
}

void CRbMq::SetRoutingKey(char* routingkey)
{
    if (m_routingkey){ free(m_routingkey); }
    m_routingkey = strdup(routingkey);
}
////////////////////////////////////////////////////////////////////////////////////////////
// Publish
//

CRbMqPub::CRbMqPub()
{
    SetDefaultMsgProp();
    SetRoutingKey("");
}

CRbMqPub::~CRbMqPub()
{

}

void CRbMqPub::SetDefaultMsgProp()
{
    m_props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    m_props.content_type = amqp_cstring_bytes("text/plain");
    m_props.delivery_mode = 2; /* persistent delivery mode */
}

void CRbMqPub::SetRoutingKey(char* routingkey)
{
    if (m_routingkey)
        free(m_routingkey);
    m_routingkey = NULL;
    m_routingkey = strdup(routingkey);
}

void CRbMqPub::SetMsgProp(amqp_basic_properties_t prop)
{
    this->m_props = prop;
}

void CRbMqPub::SetExchange(char* exchange  /* Name of exchange */, EXCHANGE_TYPE exchange_type /* Exchange type */)
{
    CRbMq::SetExchange(exchange, exchange_type);
}

void CRbMqPub::SetExchange(char* exchange, char* exchange_type)
{
    CRbMq::SetExchange(exchange, exchange_type);
}

bool CRbMqPub::PushData(void* data)
{
    return CRbMqPub::PushData((char*)data);
}

bool CRbMqPub::PushData(const char* data)
{
    return CRbMqPub::PushData((void *)data, (int)strlen(data), m_routingkey);
}

bool CRbMqPub::PushData(const char* data, int len)
{

    return CRbMqPub::PushData((void *)data, len, m_routingkey);
}

bool CRbMqPub::PushData(MQMESSAGE* mqmsg)
{
    return CRbMqPub::PushData((void *)mqmsg->bytes, mqmsg->len, m_routingkey);
}

bool CRbMqPub::PushData(void *data, int len, char *routingkey)
{
    amqp_bytes_t     message_bytes;
    amqp_rpc_reply_t ret;
    amqp_bytes_t     routingkey_bytes;

    message_bytes.len = len;
    message_bytes.bytes = (void*)data;

    if (m_thr_id != 0 && m_thr_id != (int)pthread_self()){
        this->SetErrMsg(" CRbMqPub::Warning! Thread Unsafe , Please use same thread inside ");
        perror("CRbMqPub::PushData Warning! Thread Unsafe , Please use same thread inside\n");
    }
    routingkey_bytes = (routingkey[0] == 0x00) ? amqp_empty_bytes : amqp_cstring_bytes(routingkey);

    amqp_basic_publish(m_conn,
        1,
        amqp_cstring_bytes(m_exchange_name),
        routingkey_bytes,
        0,
        0,
        &m_props,
        message_bytes);
    ret = amqp_get_rpc_reply(m_conn);
    if (IsError(ret)) {
        return false;
    }
    return true;
}

void CRbMqPub::SetQueue(char *exchange, char *queuename, char *routingkey)
{
    //this->SetErrMsg(" CRbMqPub::SetQueue Can't be called in CRbMqPub");
    CRbMq::SetQueue( exchange, queuename, routingkey );
}

bool CRbMqPub::DeclareExchange()
{
    return  CRbMq::DeclareExchange();
}

bool CRbMqPub::DeclareQueue()
{
    return CRbMq::DeclareQueue();
}

bool CRbMqPub::BindQueue()
{
    return CRbMq::BindQueue();
}

bool CRbMqPub::ReadytoReceive()
{
    this->SetErrMsg(" CRbMqPub::ReadytoReceive Can't be called in CRbMqPub");
    return false;
}

bool CRbMqPub::PopData(MQMESSAGE *received)
{
    this->SetErrMsg(" CRbMqPub::PopData Can't be called in CRbMqPub");
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Subscribe
//
CRbMqSub::CRbMqSub()
{

}

CRbMqSub::~CRbMqSub()
{

}

void CRbMqSub::SetQueue(char *exchange, char *queuename, char *routingkey)
{
    CRbMq::SetQueue(exchange, queuename, routingkey);
}

bool CRbMqSub::DeclareQueue()
{
    return CRbMq::DeclareQueue();
}

bool CRbMqSub::PopData(MQMESSAGE *received)
{
    amqp_rpc_reply_t    res;
    amqp_envelope_t     envelope;

    amqp_maybe_release_buffers(m_conn);
    if (m_thr_id != 0 && m_thr_id != (int)pthread_self()){
        this->SetErrMsg(" CRbMqSub::Warning! Thread Unsafe , Please use same thread inside ");
        perror("CRbMqPub::PopData Warning! Thread Unsafe , Please use same thread inside\n");
    }
    res = amqp_consume_message(m_conn,
        &envelope,
        NULL,     //timeout
        0);      // flags(unused)

    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        amqp_destroy_envelope(&envelope);
        return false;
    }

    received->m_properties = envelope.message.properties;
    received->len = envelope.message.body.len;

    received->bytes = (void *)malloc(envelope.message.body.len);
    if (received->bytes == NULL) {
        this->SetErrMsg(" Error while allocating memory ");
        amqp_destroy_envelope(&envelope);
        return false;
    }

    memcpy(received->bytes, envelope.message.body.bytes, envelope.message.body.len);
    amqp_destroy_envelope(&envelope);
    return true;
}

void CRbMqSub::SetExchange(char *exchange, EXCHANGE_TYPE exchange_type)
{
    CRbMq::SetExchange( exchange,  exchange_type );
}

void CRbMqSub::SetExchange(char* exchange, char* exchange_type)
{
    CRbMq::SetExchange( exchange, exchange_type );
}

void CRbMqSub::SetRoutingKey(char *routingkey)
{
    CRbMq::SetRoutingKey( routingkey );
}

bool CRbMqSub::BindQueue()
{
    return CRbMq::BindQueue();
}

bool CRbMqSub::ReadytoReceive()
{
    return CRbMq::ReadytoReceive();
}

bool CRbMqSub::PushData(void *data)
{
    this->SetErrMsg(" CRbMqSub::PushData Can't be called in CRbMqSub");
    return false;
}
bool CRbMqSub::PushData(const char *data)
{
    this->SetErrMsg(" CRbMqSub::PushData Can't be called in CRbMqSub");
    return false;
}
bool CRbMqSub::PushData(const char *data, int len)
{
    this->SetErrMsg(" CRbMqSub::PushData Can't be called in CRbMqSub");
    return false;
}
bool CRbMqSub::PushData(void *data, int len, char *routingkey)
{
    this->SetErrMsg(" CRbMqSub::PushData Can't be called in CRbMqSub");
    return false;
}

bool CRbMqSub::PushData(MQMESSAGE* mqmsg)
{
    this->SetErrMsg(" CRbMqSub::PushData Can't be called in CRbMqSub");
    return false;
}

bool CRbMqSub::DeclareExchange()
{
    this->SetErrMsg(" CRbMqSub::DeclareExchange  Can't be called in CRbMqSub");
    return false;
}
