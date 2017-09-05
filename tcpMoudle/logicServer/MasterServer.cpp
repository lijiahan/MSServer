#include "MasterServer.h"
#include "MessageMoudle.hpp"
#include "LogicMoudle.hpp"

MasterServer::MasterServer()
{
    //ctor
}

MasterServer::~MasterServer()
{
    delete asynThread;
    close( asynFd );
}

void MasterServer::initialServer( std::string ip, int port)
{
    svrConnCtxMgr = new ServerConnCtxMgr();

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
    initMsgMoudle();
    //
    printf("Hello MasterServer!\n");
    startServer();
}

void MasterServer::listenEventHandler(ConnCtx * conn)
{
    ServerDataCtx * extraData = (ServerDataCtx *) malloc(sizeof(ServerDataCtx));
    //
    extraData->writeInd = asynThread->addWriteConn(conn);
    extraData->ctxIndex = 0;
    //
    conn->eDataCtx = (void *) extraData;
}

void MasterServer::writeEventHandler(ConnCtx * conn)
{
    //
}

//
 void MasterServer::connError(int errCode, ConnCtx * conn)
 {
    ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
    if( extraData != NULL )
    {
        switch( conn->connType )
        {
            case ConnGateWay:
            {
                printf("del ConnGateWay ind : %d\n", extraData->ctxIndex);
                svrConnCtxMgr->delGateWayCtx(conn);

                break;
            }

            case ConnSlave:
            {
                svrConnCtxMgr->delSServerCtx(conn);
                break;
            }
        }

        asynThread->delWriteConn(extraData->writeInd);
        free(extraData);
    }
 }

int MasterServer::addTimer(ConnCtx * conn, TimerEventHandler * handler)
{
    return 0;
}

int MasterServer::delTimer(TimerEventHandler * handler)
{
    return 0;
}

int MasterServer::resetTimer(TimerEventHandler * handler)
{
    return 0;
}

//
void MasterServer::initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData)
{
    switch(msg->handleType)
    {
        case ReqGWConnectMS:
        {
            GateWayConnectMST * res = (GateWayConnectMST *) resData;
            //
            res->logicSvrIndex = svrConnCtxMgr->addGateWayCtx(conn, res->serverId);
            //
            conn->connType = ConnGateWay;
            break;
        }

        case ReqSSConnectMS:
        {
            SlaveSvrConnectMST * res = (SlaveSvrConnectMST *) resData;
            //
            res->logicSvrIndex = svrConnCtxMgr->addSServerCtx(conn, res->serverId, res->ip, res->prot);
            //
            conn->connType = ConnSlave;
            break;
        }
    }
}

//
void MasterServer::initMsgMoudle()
{
    //
    MsgMoudleInterface * initMoudle = new MasterInitMsgMoudle();
    registerMsgMoudle(MainInitMouldeType, initMoudle);
    //
    MsgMoudleInterface * LogicMoudle = new MasterLogicMsgMoudle();
    registerMsgMoudle(LogicMouldeType, LogicMoudle);
    //
}

//
void MasterServer::initLogicMoudle()
{
    LogicMoudleInterface * shareMoudle = new MasterShareBlockLogic();
    registerLogicMoudle(ShareBlockMoulde, shareMoudle);

    LogicMoudleInterface * independMoudle = new MasterIndenpentBlockLogic();
    registerLogicMoudle(IndenpentBlockMoulde, independMoudle);
}

int MasterServer::diapatchServerInd(int blockId)
{
    return 0;
}

void MasterServer::addServerInd(int ind)
{

}
//
void MasterServer::sendMsgToServer(SendMsgInfo * msgInfo)
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
            svrConnCtxMgr->sendMsgToAllGateWay(msgInfo, asynThread);
            break;
        }

        case SendAllSSMsgToConn:
        {
            svrConnCtxMgr->sendAllSSMsgToConn(msgInfo->writeInd, asynThread);
            break;
        }
    }

}
