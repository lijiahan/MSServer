#ifndef BASESTRUCTHEADER_H_INCLUDED
#define BASESTRUCTHEADER_H_INCLUDED

#include <string>
#include <iostream>
#include <sstream>

#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include <event.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "ConnectMsg.pb.h"

#define BufSize 8192
#define ConnSize 20

extern int errno;

typedef unsigned int CLIENTIDTYPE;

enum LogicBlockMoulde
{
    ShareBlockMoulde = 1,
    IndenpentBlockMoulde = 2,
};

enum MsgMoudleType
{
    GateWayMsgType = 1,

    MainServerMsgType = 2,

    SlaveServerMsgType = 3,

    ClientLogicMsgType = 4,

    ClientGateWayMsgType = 5,
};

enum ClientLogicHandleType
{
    ReqCliConnectLS = 41001,

    ResCliConnectLS = 42001,
};

enum ClientGateWayHandlerType
{
    ReqCliConnectGw = 51001,

    ResCliConnectGw = 52001,
};


enum ConnType
{
    ConnGateWay =1,
    ConnLogicServer,
    ConnClient,
};

enum ConnState
{
    ConnReadState = 1,
    ConnWaitReadState,
    ConnWriteState,
    ConnWaitWriteState,
    ConnHandleState,
};

struct ClientToGateMsg
{
    int msgType;
    int logicBlockId;
    char codeKey[4];
    int handlerState;
    int msgLen;
    char data[];
};

class BufCtx
{
    public:
        BufCtx()
        {
            memset(buf, 0, BufSize);
            bufLen = 0;
            next = NULL;
        }
    public:
        char buf[BufSize];
        int bufLen;
        BufCtx * next;
};

class connCtx
{
    public:
        int index;
        int connType;
        int fd;
        int state;
        BufCtx * readBuf;
        BufCtx * writeBuf;
        struct event readEevnt;
        struct event writeEevnt;
        struct event timeEevnt;
        void * ctx;
};

class reqHandlerCtx
{
    public:
        int handleState;
        int dataLen;
        connCtx * conn;
        char dataValue[BufSize];
};

static int ClientToGateMsgLen = sizeof(ClientToGateMsg);



#endif // BASESTRUCTHEADER_H_INCLUDED
