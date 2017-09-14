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
    initLogicMoudle();
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

void MasterServer::logicMoudle(ConnCtx * conn, InterComMsg * msg)
{
    CLIENTIDTYPE uid = msg->uId;
    ClientCtx * clientCtx = clientCtxMgr->getCtx(uid);

    //
    if( clientCtx != NULL )
    {
        HandlerCtx * handlerCtx = handlerCtxMgr->getCtx(msg->handlerInd, conn, msg, clientCtx, clientCtxMgr);
        MOUDLEID mid = msg->logicMoudleId;
        std::map<int, LogicMoudleInterface * >::iterator it = moudleInterfaceMap.find(mid);
        if( it != moudleInterfaceMap.end() )
        {
            printf("logicMoudle uid %d mid %d \n", uid, mid);
            LogicMoudleInterface * mif = it->second;
            //
            MoudleDataCheck dtCheck;
            memset(&dtCheck, 0, sizeof(MoudleDataCheck));
            dtCheck->logicId = mid;
            dtCheck.dispIndex = diapatchServerInd(mid);
            int sInd = mif->getDataCheck(clientCtx, &dtCheck);

            //
            if(sInd == -1)
            {
                handleDataConflict(clientCtx, &dtCheck);
            }
            else
            {
                handleDispacthServer(clientCtx, &dtCheck);
            }
        }
    }
}
//

void handleDataConflict(ClientCtx * clientCtx, MoudleDataCheck * dataCheck, ConnCtx * conn)
{
    //
}

void handleDispacthServer(ClientCtx * clientCtx, MoudleDataCheck * dataCheck,ConnCtx * conn)
{
    //
    for(int i=0; i<dataCheck->dataNum; i++)
    {
        DataMoudleCtx * dataCtx = clientCtx->getDataCtx(dataCheck->dataIndex[i]);
        dataCtx->dispIndex = dataCheck->dispIndex;
    }

    //
    char mbuf[256];
    memset(mbuf, 0, 256);
    InterComMsg * rm = MsgMgr::buildInterComMsg(mbuf, LogicMouldeType, ReqCliLogicMS,
                             0, client->clConnCtx.connIndex, 0, client->uid, 0);

    GateWayCtx * gateWayCtx = svrConnCtxMgr->getGateWayCtx(dataCheck->dispIndex);
    MsgMgr::buildCliLogicMST(rm->data, ReqCliLogicMS, clientCtx->uid, gateWayCtx->serverId, dataCheck->logicId);

    SlaveServerCtx * slaveCtx = svrConnCtxMgr->getSServerCtx(conn);
    ServerDataCtx * extraSlaveData = (ServerDataCtx *) (slaveCtx->conn->eDataCtx);
    asynThread->asynWriteMsg(rm, extraSlaveData->writeInd);
}

//
void MasterServer::sendMsgToServer(SendMsgInfo * msgInfo)
{
    switch(msgInfo->msgType)
    {
        case SendMsgToConn:
        {
            ServerDataCtx * extraData = (ServerDataCtx *) (msgInfo->conn->eDataCtx);
            asynThread->asynWriteMsg(msgInfo->sendMsg, extraData->writeInd);
            break;
        }

        case SendMsgToAllGWServer:
        {
            svrConnCtxMgr->sendMsgToAllGateWay(msgInfo, asynThread);
            break;
        }

        case SendAllSSMsgToConn:
        {
            ServerDataCtx * extraData = (ServerDataCtx *) (msgInfo->conn->eDataCtx);
            svrConnCtxMgr->sendAllSSMsgToConn(extraData->writeInd, asynThread);
            break;
        }
    }

}
