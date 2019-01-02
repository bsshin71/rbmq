# rbmq
C++ wrapping class for convenient use of  rabbitmq-c

# rbmq C++ class

메세지 큐의 한 종류인 rabbitmq를 위한   C 언어 library 인   rabbitmq-c  library를  좀더 쉽게 사용할 수 있도록 C++ class 로 wrapping 하였다.



# 소스 구성

| 파일명       | 용도                                  |
| ------------ | ------------------------------------- |
| makefile     | sample 소스 컴파일을 위한 makefile    |
| makefile.lib | rbmq.a library 생성을 위한 makefile   |
| rbmq.h       | rbmq C++ class  header                |
| rbmq.cpp     | rbmq C++ class member function source |
| sendq.cpp    | queue publish( push ) 예제            |
| recvq.cpp    | queue subscribe(pop) 예제             |



# 설치 및 컴파일

rabbitmq-C library가 필요하다. 

https://github.com/alanxz/rabbitmq-c  의 설명에 따라서  library를 build한다.  build가 성공하면 /usr/local/lib64/librabbitmq.a 에   rabbitmq-c library가 생성된다.

생성된 rabbitmq-c library 경로에 맞게 makefile 을 수정한다.

makefile  수정이 끝나면  테스트 프로그램을 build 한다.

```
make sendq   # push 를 위한 테스트프로그램 생성
make recvq   # pop 을 위한 테스트 프로그램 생성
```



# 실행

1. 수신프로그램을 먼저 실행한다.

   ```
   $ ./recvq
   ```

2. 송신 프로그램을 실행한다.

   ```
   ./sendq 10
   Create Queue
   MQ library Send Test
   Sent data=[Test Queue Data, Sending number[0]]
   Sent data=[Test Queue Data, Sending number[1]]
   Sent data=[Test Queue Data, Sending number[2]]
   Sent data=[Test Queue Data, Sending number[3]]
   Sent data=[Test Queue Data, Sending number[4]]
   Sent data=[Test Queue Data, Sending number[5]]
   Sent data=[Test Queue Data, Sending number[6]]
   Sent data=[Test Queue Data, Sending number[7]]
   Sent data=[Test Queue Data, Sending number[8]]
   Sent data=[Test Queue Data, Sending number[9]]
   ```


