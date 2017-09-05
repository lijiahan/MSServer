#ifndef SLAVESERVER_H
#define SLAVESERVER_H

#include "AsynOperationThread.h"
#include "basicHeader/ServerEventImplSt.hpp"

struct MasterServerCtx
{
    int slaveInd;
    ConnCtx * conn;
};

class GWConnCtxMgr
{
    public:
        GWConnCtxMgr()
        {
            gwIndex = 1;
        }

        ~GWConnCtxMgr()
        {
            std::map<int, GateWayCtx *>::iterator it2;
            for( it2 = gateWayMap.begin(); it2 != gateWayMap.end(); it2++ )
            {
                GateWayCtx * gwCtx = it2->second;
                if( gwCtx != NULL )
                {
                    free(gwCtx);
                }
            }
            gateWayMap.clear();
        }

        int addGateWayCtx( ConnCtx * conn, int sid )
        {
            GateWayCtx * gwCtx = (GateWayCtx *) malloc(sizeof(GateWayCtx));
            gwCtx->conn = conn;
            gwCtx->serverId = sid;
            gwCtx->gateWayIndex = gwIndex;
            gateWayMap[gwIndex] = gwCtx;

            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            extraData->ctxIndex = gwIndex;

            gwIndex++;
            return gwCtx->gateWayIndex;
        }

        GateWayCtx * getGateWayCtx( int ind )
        {
            GateWayCtx * ctx = NULL;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(ind);
            if(iter != gateWayMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        GateWayCtx * getGateWayCtx( ConnCtx * conn )
        {
            GateWayCtx * ctx = NULL;
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(extraData->ctxIndex);
            if(iter != gateWayMap.end())
            {
               ctx = iter->second;
            }

            return ctx;
        }

        void delGateWayCtx( ConnCtx * conn )
        {
            ServerDataCtx * extraData = (ServerDataCtx *) conn->eDataCtx;
            std::map<int, GateWayCtx *>::iterator iter = gateWayMap.find(extraData->ctxIndex);
            if(iter != gateWayMap.end())
            {
                GateWayCtx * ctx = iter->second;
                free(ctx);
                gateWayMap.erase(iter);
            }

            extraData->ctxIndex = 0;
        }

        void sendMsgToAllGateWay(SendMsgInfo * msgInfo, AsynOperationThread * asynThread)
        {
            std::map<int, GateWayCtx *>::iterator iter;
            for( iter = gateWayMap.begin(); iter != gateWayMap.end(); iter++ )
            {
                GateWayCtx * gwCtx = iter->second;
                if( gwCtx != NULL )
                {
                    ConnCtx * conn = gwCtx->conn;
                    ServerDataCtx * extraData = (ServerDataCtx *) (conn->eDataCtx);
                    asynThread->asynWriteMsg(msgInfo->sendMsg, extraData->writeInd);
                }
            }
        }

    private:
        int gwIndex;
        std::map<int, GateWayCtx *>  gateWayMap;
};

class SlaveServer : public ServerExtraInterface
{
    public:
        SlaveServer();
        virtual ~SlaveServer();
        void initialServer(std::string ip, int port, std::string masterLgIp, int masterLgPort);
        void connectMasterServer(std::string masterLgIp, int masterLgPort, InterComMsg * msg);
        //
        virtual void listenEventHandler(ConnCtx * conn);
        virtual void writeEventHandler(ConnCtx * conn);
        virtual void connError(int errCode, ConnCtx * conn);
        //
        virtual int addTimer(ConnCtx * conn, TimerEventHandler * handler);
        virtual int delTimer(TimerEventHandler * handler);
        virtual int resetTimer(TimerEventHandler * handler);
        //
        virtual void initMsgMoudle();
        virtual void initServerByMsg(InterComMsg *msg, ConnCtx * conn, void * resData);
        //
        virtual void initLogicMoudle();
        //
        virtual int diapatchServerInd(int blockId);
        virtual void addServerInd(int ind);
        //
        virtual void sendMsgToServer(SendMsgInfo * sendMsg);

    private:
        //
        int serverId;
        int asynFd;
        AsynOperationThread * asynThread;
        MasterServerCtx * masterCtx;
        ConnCtx * asynConn;
        GWConnCtxMgr * gWConnMgr;
};

#endif // SLAVESERVER_H
