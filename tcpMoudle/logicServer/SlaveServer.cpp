#include "SlaveServer.h"
#include "MessageMoudle.hpp"
#include "LogicMoudle.hpp"

SlaveServer::SlaveServer()
{
    //ctor
    masterCtx = (MasterServerCtx *) malloc(sizeof(MasterServerCtx));
    masterCtx->conn = NULL;
    masterCtx->slaveInd = 0;
}

SlaveServer::~SlaveServer()
{
    delete gWConnMgr;
    //dtor
    delete asynThread;
    close( asynFd );

    free(masterCtx);
}

void SlaveServer::initialServer(std::string ip, int port, std::string masterLgIp, int masterLgPort)
{
    gWConnMgr = new GWConnCtxMgr();

    initServer(ip, port);
    //
    asynThread = new AsynOperationThread();
    asynThread->initialThread();

    asynFd = asynThread->getSocketPairFd();

    asynConn = getSvrConnCtx();
    asynConn->fd = asynFd;
    asynConn->connType = ConnAsynThread;
    asynConn->serverCtx = (void *) this;
    //
    prepareRead(asynConn);
    //
    //
    int wlen = MsgMgr::SlaveSvrConnectMSTLen + MsgMgr::InterComMsgLen;
    char msgBuf[1024];
    memset(msgBuf, 0, 1024);

    InterComMsg * msg = (InterComMsg *) msgBuf;
    msg->msgMoudleType = MainInitMouldeType;
    msg->handleType = ReqSSConnectMS;
    //
    SlaveSvrConnectMST * msgSt = (SlaveSvrConnectMST *) (msg->data);
    msgSt->handleType = ReqSSConnectMS;
    msgSt->serverId = serverId;
    strcpy(msgSt->ip, ip.c_str());
    msgSt->prot = port;
    msg->msgLen = wlen;
    //
    connectMasterServer(masterLgIp, masterLgPort, msg);
    //
    initMsgMoudle();
    //
    startServer();
}

void SlaveServer::connectMasterServer(std::string masterLgIp, int masterLgPort, InterComMsg * msg)
{
    //
    int nfd = 0;
    if( (nfd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
    {
        perror(" can not create socket \n");
        exit(0);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(masterLgPort);

    if( inet_pton(AF_INET, masterLgIp.c_str(), &server_addr.sin_addr ) < 1)
    {
        perror(" inet_pton error \n");
        exit(0);
    }

    if( connect(nfd,  (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror(" connect logic master  server error \n");
        exit(0);
    }

    evutil_make_socket_nonblocking( nfd );

    printf(" connect logic master server successfully\n");

    //
    masterCtx->conn = getSvrConnCtx();
    masterCtx->conn->fd = nfd;
    masterCtx->conn->connType = ConnMaster;
    masterCtx->conn->serverCtx = (void *) this;

    prepareRead(masterCtx->conn);
    //
    //
    BufCtx * wbufCtx = writeData(masterCtx->conn, msg->msgLen);
    MsgMgr::addICMsgToBufCtx(msg, wbufCtx);
}

void SlaveServer::listenEventHandler(ConnCtx * conn)
{
    ServerDataCtx * extraData = (ServerDataCtx *) malloc(sizeof(ServerDataCtx));
    //
    extraData->writeInd = asynThread->addWriteConn(conn);
    extraData->ctxIndex = 0;
    conn->eDataCtx = (void *) extraData;
}

void SlaveServer::writeEventHandler(ConnCtx * conn)
{
    //
}

//
 void SlaveServer::connError(int errCode, ConnCtx * conn)
 {
    ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
    if( extraData != NULL )
    {
         switch( conn->connType )
        {
            case ConnGateWay:
            {
                printf("del ConnGateWay ind : %d\n", extraData->ctxIndex);
                gWConnMgr->delGateWayCtx(conn);
                break;
            }
        }

        asynThread->delWriteConn(extraData->writeInd);
        free(extraData);
    }
 }

int SlaveServer::addTimer(ConnCtx * conn, TimerEventHandler * handler)
{
    return 0;
}

int SlaveServer::delTimer(TimerEventHandler * handler)
{
    return 0;
}

int SlaveServer::resetTimer(TimerEventHandler * handler)
{
    return 0;
}

//
void SlaveServer::initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData)
{
    switch(msg->handleType)
    {
        case ReqGWConnectSS:
        {
            GateWayConnectMST * res = (GateWayConnectMST *) resData;
            //
            res->logicSvrIndex = gWConnMgr->addGateWayCtx(conn, res->serverId);
            //
            conn->connType = ConnGateWay;

            break;
        }

        case ResSSConnectMS:
        {
            printf("ResSSConnectMS!! severId : %d \n", serverId);
            break;
        }
    }
}

//
void SlaveServer::initMsgMoudle()
{
     //
    MsgMoudleInterface * initMoudle = new SlaveInitMsgMoudle();
    registerMsgMoudle(MainInitMouldeType, initMoudle);
    //
    MsgMoudleInterface * LogicMoudle = new SlaveLogicMsgMoudle();
    registerMsgMoudle(LogicMouldeType, LogicMoudle);
}

void SlaveServer::initLogicMoudle()
{

}

int SlaveServer::diapatchServerInd(int blockId)
{
    return 0;
}

void SlaveServer::addServerInd(int ind)
{

}
//
void SlaveServer::sendMsgToServer(SendMsgInfo * msgInfo)
{
    switch(msgInfo->msgType)
    {
        case SendMsgToConn:
        {
            asynThread->asynWriteMsg(msgInfo->sendMsg, msgInfo->writeInd);
            break;
        }

        case SendMsgToAllGWServer:
        {
            gWConnMgr->sendMsgToAllGateWay(msgInfo, asynThread);
            break;
        }
    }

}

