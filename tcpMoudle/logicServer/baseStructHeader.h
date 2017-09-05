#ifndef BASESTRUCTHEADER_H_INCLUDED
#define BASESTRUCTHEADER_H_INCLUDED

#include<string>
#include<iostream>
#include<sstream>

#include<netinet/in.h>
#include<netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include<event.h>
#include<event2/util.h>
#include<event2/event.h>
#include <event2/thread.h>

#include "ConnectMsg.pb.h"

#define BufSize 8192
#define ConnSize 20

extern int errno;

typedef unsigned int CLIENTIDTYPE;

struct commandCtx
{
    int cmdType;
    int blockId;
    int connIndex;
    CLIENTIDTYPE uId;
    int handlerState;
    int cmdLen;
    char data[];
};

enum CommandType
{
     //gate to logic
    MsgGateToLogic = 1,
    //client to gate
    MsgClientToGate = 2,
    //client to logic
    MsgClientToLogic = 3,
    //logic to client
    MsgLogicToClient = 4,
    //
    MsgLogicToAsyn = 5,
};

enum ConnType
{
    ConnGateWay =1,
    ConnLogicServer,
    ConnClient,
    ConnAsyn,
};

enum ConnState
{
    ConnReadState = 1,
    ConnWaitReadState,
    ConnWriteState,
    ConnWaitWriteState,
    ConnHandleState,
};

enum HandlerState
{
    HandlerLogicState = 1,
};

class bufCtx
{
    public:
        bufCtx()
        {
            memset(buf, 0, BufSize);
            bufLen = 0;
            next = NULL;
        }
    public:
        char buf[BufSize];
        int bufLen;
        bufCtx * next;
};

class connCtx
{
    public:
        int index;
        int connType;
        int fd;
        int state;
        bufCtx * readBuf;
        bufCtx * writeBuf;
        struct event readEevnt;
        struct event writeEevnt;
        struct event timeEevnt;
        void * ctx;
};

class reqHandlerCtx
{
    public:
        int handleState;
        connCtx * conn;
        int dataLen;
        char dataValue[BufSize];
};

static int CommandHeadLen = sizeof(commandCtx);

#endif // BASESTRUCTHEADER_H_INCLUDED
