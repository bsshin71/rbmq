/*
 * sendq.cpp
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
#include <json.h>

void createQueue()
{

   printf("Create Queue\n");

        CRbMqPub *pro = new CRbMqPub();

        pro->Create(  "127.0.0.1",  "guest", "guest", "/", 5672 , 131072, 0);

        if ( !pro->OpenConnect() ) {
            printf(" Error while doing OpenConnect message=%s\n", pro->GetLastErrMsg() );
        }

        if ( !pro->OpenChannel()) {
            printf(" Error while doing OpenChannel message=%s\n", pro->GetLastErrMsg() );
        }

         pro->SetExchange("Q_PUB_ORDERPRICE", direct );
        if ( !pro->DeclareExchange()) {
            printf(" Error while doing DeclareExchange message=%s\n", pro->GetLastErrMsg() );
        }

        if ( !pro->CreateQueue("Q_PUB_ORDERPRICE", "ORDERPRICE_GROUP1", "ORDERPRICE_GROUP1") )
        {
            printf(" Error while doing CreateQueue message=%s\n", pro->GetLastErrMsg() );
        }

        if( !pro->CloseChannel() ) {
            printf(" Error while doing CloseChannel message=%s\n", pro->GetLastErrMsg() );
        }

        if( !pro->CloseConnect() ) {
            printf(" Error while doing CloseConnect message=%s\n", pro->GetLastErrMsg() );
        }


        pro->Destroy() ;

        delete( pro );
}

void SendDetailTest(int count)
{
    printf("MQ library Send Test\n");

    CRbMqPub *pro = new CRbMqPub();

    pro->Create(  "127.0.0.1",  "guest", "guest", "/", 5672 , 131072, 0);

    if ( !pro->OpenConnect() ) {
        printf(" Error while doing OpenConnect message=%s\n", pro->GetLastErrMsg() );
    }

    if ( !pro->OpenChannel()) {
        printf(" Error while doing OpenChannel message=%s\n", pro->GetLastErrMsg() );
    }

    pro->SetExchange("Q_PUB_ORDERPRICE", direct );
    if ( !pro->DeclareExchange()) {
        printf(" Error while doing DeclareExchange message=%s\n", pro->GetLastErrMsg() );
    }

    for(int i=0; i < count; i++ )
    {
      char data[1024] = { 0, };

      pro->SetRoutingKey( "ORDERPRICE_GROUP1" );
      sprintf( data, "Test Queue Data, Sending number[%d]", i);
      if( !pro->PushData( data, strlen(data) ) ) {
        printf(" Error while doing PushData1 message=%s\n", pro->GetLastErrMsg() );
      }
      printf("Sent data=[%s]\n", data );

      usleep(500000);
    }

    if( !pro->CloseChannel() ) {
        printf(" Error while doing CloseChannel message=%s\n", pro->GetLastErrMsg() );
    }

    if( !pro->CloseConnect() ) {
        printf(" Error while doing CloseConnect message=%s\n", pro->GetLastErrMsg() );
    }

    pro->Destroy() ;

    delete( pro );

}


int main( int argc, char **argv )
{
    int rc = 0;

    int count=0;
    if( argc < 2 ) {
        printf("Usage %s count \n", argv[0]);
        exit(1);
    }

    count = atoi(argv[1]);
    // Detail 하게 제어하는 함수사용 예제
    //SendDetailTest();

    //ConvPackToJson();
    // 간단하게 전송하는 예제
    //SendSimpleTest();

    //getTestData();

    //setTestData();

    createQueue();
    SendDetailTest( count );

    return rc;
}
