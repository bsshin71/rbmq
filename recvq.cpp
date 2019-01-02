/*
 * recvq.cpp
 *
 *  Created on: 2018. 9. 30.
 *      Author: omegaman
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "rbmq.h"
#include "kxtracelog.h"


void RecvSimpleTest()
{
    printf("MQ library Recv Simple Test\n");

    CRbMqSub *mq = new CRbMqSub();

    mq->Create(  "127.0.0.1",  "guest", "guest", "/", 5672 , 131072, 0);

    //mq->InitConfig();

    if ( !mq->OpenConnect() ) {
          printf(" Error while doing OpenConnect message=%s\n", mq->GetLastErrMsg() );
    }

    if ( !mq->OpenChannel()) {
          printf(" Error while doing OpenChannel message=%s\n", mq->GetLastErrMsg() );
          return;
    }


    if ( !mq->CreateQueue("Q_PUB_ORDERPRICE", "ORDERPRICE_GROUP1", "ORDERPRICE_GROUP1") )
    {
        printf(" Error while doing CreateQueue message=%s\n", mq->GetLastErrMsg() );
    }

    if ( !mq->ReadytoReceive() ) {
              printf(" Error while doing ReadytoReceive message=%s\n", mq->GetLastErrMsg() );
              return;
      }

    for(;;)
    {
          MQMESSAGE *received;

          received = ( MQMESSAGE *) malloc( sizeof( MQMESSAGE) );
          memset( received, 0x00, sizeof(received) );

          if( !mq->PopData( received ) ) {
            printf(" Error while doing PopData message=%s\n", mq->GetLastErrMsg() );
          }
          printf("Received data=[%.*s]\n", received->len, (char *) received->bytes );

          usleep(500000);

          free( received->bytes );
          free( received );
    }

    if( !mq->CloseAll() ) {
        printf(" Error while doing CloseChannel or CloseConnect message=%s\n", mq->GetLastErrMsg() );
    }

//    mq->Destroy();

    delete( mq );
}


int main( int argc, char **argv )
{
    int rc = 0;


    // 간단하게 Recv 하는 예제
    RecvSimpleTest();

    return rc;
}
