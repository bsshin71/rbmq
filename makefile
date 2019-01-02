AMQP_LIBDIR=/usr/local/lib64/
AMQP_INCLUDE=/usr/local/include

INCLUDES+=-I${AMQP_INCLUDE}  -I.  -I/usr/include -I/usr/local/include/json-c -I../../../kx.framework/include
LIB_PATH= -L${AMQP_LIBDIR}  -L/usr/local/lib -L../../../../lib
DEF_INC:=-I. -I../../../../inc/struct

CFLAGS=-g -W -Wall 
TARGET=sendq recvq

all: ${TARGET} 

sendq: sendq.cpp rbmq.cpp rbmq.h 
        g++ -c ${CFLAGS}  ${INCLUDES} ${DEF_INC} rbmq.cpp
        g++ -c ${CFLAGS}  ${INCLUDES} ${DEF_INC} sendq.cpp 
        g++ rbmq.o sendq.o  -o sendq ${LIB_PATH} -ldl -lm -lrt -lrabbitmq -ljson-c -lkxcomn  -lkxutil
recvq: recvq.cpp rbmq.cpp rbmq.h
        g++ -c ${CFLAGS}  ${INCLUDES} ${DEF_INC} rbmq.cpp
        g++ -c ${CFLAGS}  ${INCLUDES} ${DEF_INC} recvq.cpp 
        g++ rbmq.o recvq.o  -o recvq ${LIB_PATH} -ldl -lm -lrt -lrabbitmq -ljson-c -lkxcomn -lkxutil 
clean: 
        rm -rf *.o sendq recvq
