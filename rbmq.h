/*
 * mqlib.h
 *
 *  Created on: 2018. 9. 29.
 *      Author: omegaman
 */

#ifndef _RBMQ_H_
#define _RBMQ_H_

#include <stdio.h>
#include <stdarg.h>
#ifdef _WINDOWS
#include <windows.h>
#endif

#include <amqp.h>
#include <amqp_tcp_socket.h>

#define MAX_MQ_ROUTKEY 5

enum EXCHANGE_TYPE {
    none = -1,
    direct = 0,
    fanout = 1,
    topic = 2,
    headers = 3
};

typedef struct _stMQMessage
{
    amqp_basic_properties_t  m_properties;  //message properties
    size_t                   len;          /**< length of the buffer in bytes */
    void                     *bytes;       /**< pointer to the beginning of the buffer */
} MQMESSAGE;

////////////////////////////////////////////////////////////////////////////////////////////
// Rabbit MQ Base Wrapper Class Implement
//
class CRbMq
{
public:
    CRbMq();
    virtual ~CRbMq();

    virtual bool isConnected();
    void Init();
    virtual void Destroy();
    //Connection Factory 를 생성한다.
    void Create(char         *host     /*     ="127.0.0.1"*/,
        char         *username /* ="guest"*/,
        char         *password /*="guest"*/,
        char         *vhost    /*="/"*/,
        unsigned int port   /*=5672*/,
        int          frame_max  /*=131072*/,
        int          channel_max = 0
        );

    // Connect to MQ
    bool OpenConnect();
    // Make a channel to opened connection
    bool OpenChannel();
    // Open Connection And Channel
    bool OpenAll();

    //Producer를 위한 Exchange를 설정한다. Producer class 전용함수
    virtual void SetExchange(char  *exchange, EXCHANGE_TYPE exchange_type);
    virtual void SetExchange(char* exchange, char  *exchange_type);
    virtual void SetRoutingKey(char *routingkey);

    virtual bool DeclareExchange();
    bool DeclareQueue(char *queuename, char *routingkey);
    bool CreateQueue( char *exchange, char *queuename, char *routing_key);
    bool BindQueue  (char *exchange, char *queuename, char *routingkey );

    //Producer에서   data를 exchange 에 send한다. Producer class 전용함수
    virtual bool PushData(void *data) = 0;
    virtual bool PushData(const char *data) = 0;
    virtual bool PushData(const char *data, int len) = 0;
    virtual bool PushData(void *data, int len, char *routingkey) = 0;
    virtual bool PushData(MQMESSAGE *mqmsg) = 0;

    // Consumer를 위한  Queue setting
    virtual void SetQueue(char *exchange, char *queuename, char *routingkey);
    virtual bool DeclareQueue();
    virtual bool BindQueue();
    virtual bool ReadytoReceive();
    // Q 의 데이타를 Pop Up 한다.
    virtual bool PopData(MQMESSAGE *received) = 0;


    // channel을 close 한다.
    bool CloseChannel();
    // connection 을 close 한다.
    bool CloseConnect();

    // Close Connection And channnel
    bool CloseAll();

    bool IsError(amqp_rpc_reply_t x);

    void SetErrMsg(const char *msg);
    char *GetLastErrMsg();

    char *GetExchangeTypeStr(EXCHANGE_TYPE exchange_type);

    char *GetHost();
    char *GetExchangeName();
    EXCHANGE_TYPE GetExchangeType();
    EXCHANGE_TYPE GetExchangeType(char *exchange_type_str);
    char *GetRoutingKeyName();
    char *GetQueueName();
    char *GetUserName();
    char *GetPassword();
    char *GetVirtualHost();
    int  GetPort();
    int  GetFrameMax();
    int  GetChannelMax();
    int  GetAutoDelete();
    int  GetQueueSize();

    void SetAutoDelete(int autodelete);
    void SetQueueSize(int queuesize);

protected:
    char           *m_hostname;
    char           *m_exchange_name;
    EXCHANGE_TYPE   m_exchange_type;
    char           *m_routingkey;
    char           *m_queuename;
    char           *m_username;
    char           *m_password;
    char           *m_vhost;
    int             m_port;
    int             m_frame_max;
    int             m_channel_max;
    int m_autodelete;
    int m_queuesize;//갯수
    amqp_socket_t            *m_socket;
    amqp_connection_state_t  m_conn;
    amqp_basic_properties_t  m_props;        // default message properties
    amqp_bytes_t             m_queuehandle;  // queue handle

    char *m_error_string;
    static const char *m_EXCHANGE_TYPE_STR[4];

protected:
    int m_thr_id;//pthread_t type thread id
};

////////////////////////////////////////////////////////////////////////////////////////////
// Publish
//
//Producer Class
class CRbMqPub : public CRbMq
{
public:
    CRbMqPub();
    virtual ~CRbMqPub();

    //Message properties
    void SetDefaultMsgProp();
    void SetMsgProp(amqp_basic_properties_t prop);

    //Producer를 위한 Exchange를 설정한다.
    void SetExchange(char *exchange, EXCHANGE_TYPE exchange_type);
    void SetExchange(char* exchange, char  *exchange_type);
    // Producer가  data를 exchange 에 send한다.
    void SetRoutingKey(char *routingkey);
    bool DeclareExchange();

    bool PushData(void *data);
    bool PushData(const char *data);
    bool PushData(const char *data, int len);
    bool PushData(void *data, int len, char *routingkey);
    bool PushData(MQMESSAGE *mqmsg);

    // Producer class 에서는 사용 X 함수
    void SetQueue(char *exchange, char *queuename, char *routingkey);
    bool DeclareQueue();
    bool BindQueue();
    bool ReadytoReceive();
    
    // Producer class 에서는 사용 X 함수
    bool PopData(MQMESSAGE *received);
};

////////////////////////////////////////////////////////////////////////////////////////////
// Subscribe
//
//Consumer Class
class CRbMqSub : public CRbMq
{
public:
    CRbMqSub();
    virtual ~CRbMqSub();

    // Consumer를 위한  Queue setting
    void SetQueue(char *exchange,char *queuename,char *routingkey);

    bool DeclareQueue();
    // Q 의 데이타를 Pop Up 한다.
    bool PopData(MQMESSAGE *received);

    //CRbMqSub에서는 사용 X 안함.
    void SetExchange(char *exchange, EXCHANGE_TYPE exchange_type);
    void SetExchange(char* exchange, char  *exchange_type);
    void SetRoutingKey(char *routingkey);
    bool DeclareExchange();
    bool BindQueue();
    bool ReadytoReceive();

    bool PushData(void *data);
    bool PushData(const char *data);
    bool PushData(const char *data, int len);
    bool PushData(void *data, int len, char *routingkey);
    bool PushData(MQMESSAGE *mqmsg);

};


#endif /* _RBMQ_H_ */
